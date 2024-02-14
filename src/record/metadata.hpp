#pragma once

#include <cstring>
#include <vector>
#include <memory>

#include "constants.h"
#include "field.hpp"

#include "../filesystem/bufmanager/BufPageManager.h"
#include "../filesystem/fileio/FileManager.h"
#include "../filesystem/utils/pagedef.h"
#include "../constants.h"

class FieldMetaData {
public:
    FieldType type;
    std::string name;
    FieldID id;
    bool not_null;
    FieldSize size; // fixed length for string
    bool has_default;
    std::shared_ptr<Field> default_value;
    
    explicit FieldMetaData() = default;
    explicit FieldMetaData(
        FieldType type, 
        std::string name, 
        FieldID id, 
        bool not_null = false,
        FieldSize size = 20, // default string length: 20 chars
        bool has_default = false, 
        std::shared_ptr<Field> default_value = nullptr
    ) {
        this->type = type;
        this->name = std::move(name);
        this->id = id;
        this->not_null = not_null;
        if (type == FieldType::INT) {
            this->size = sizeof(int); // 4 bytes
        } else if (type == FieldType::FLOAT) {
            this->size = sizeof(double); // 8 bytes
        } else if (type == FieldType::VARCHAR) {
            this->size = size; // 20 bytes by default
        }
        this->has_default = has_default;
        this->default_value = std::move(default_value);
    }

    void store(BufType &b, int &offset) const {
        // store type
        b[offset] = static_cast<int32_t>(type);
        offset++;

        // store name
        int32_t name_size = name.size();
        b[offset] = name_size;
        offset++;
        int block_size = (name_size + 3) / 4; // 4 char per unit
        memcpy(b + offset, name.c_str(), name_size);
        offset += block_size;

        // store id
        b[offset] = id;
        offset++;

        // store not_null
        b[offset] = not_null;
        offset++;

        // store size
        b[offset] = size;
        offset++;

        // store has_default
        b[offset] = has_default;
        offset++;
    }

    void load(BufType &b, int &offset) {
        // load type
        type = static_cast<FieldType>(b[offset]);
        offset++;

        // load name
        int32_t name_size = b[offset];
        offset++;
        int block_size = (name_size + 3) / 4; // 4 char per unit
        char name[name_size + 1]; // +1: for ending symbol '\0'
        memcpy(name, b + offset, name_size);
        name[name_size] = '\0';
        this->name = name;
        offset += block_size;

        // load id
        id = b[offset];
        offset++;

        // load not_null
        not_null = b[offset];
        offset++;

        // load size
        size = b[offset];
        offset++;

        // load has_default
        has_default = b[offset];
        offset++;
    }

    FieldSize field_size() {
        if (type == FieldType::INT) {
            return sizeof(int) / 4; // 4 bytes == 1
        } else if (type == FieldType::FLOAT) {
            return sizeof(double) / 4; // 8 bytes == 2
        } else if (type == FieldType::VARCHAR) {
            return (size + 3) / 4; // 20 bytes by default == 5
        }
        return 0;
    }

    std::string toString() {
        std::string str = "FieldMetaData: \n";
        str += "type: " + FieldType2String(type) + "\n";
        str += "name: " + name + "\n";
        str += "id: " + std::to_string(id) + "\n";
        str += "not_null: " + std::to_string(not_null) + "\n";
        str += "size: " + std::to_string(size) + "\n";
        str += "has_default: " + std::to_string(has_default) + "\n";
        return str;
    }

};

class TableMetaData {
public:
    TableID table_id;
    std::string name;
    std::vector<std::shared_ptr<FieldMetaData>> fields;
    std::optional<std::pair<std::string, std::vector<std::string>>> primary_key;
    std::vector<ForeignKey> foreign_keys;
    std::vector<std::pair<std::string,std::string>> indexs; //[idx_a, a]

    explicit TableMetaData() = default;
    explicit TableMetaData(
        TableID _table_id, 
        std::string _name, 
        std::vector<std::shared_ptr<FieldMetaData>> _fields,
        std::optional<std::pair<std::string, std::vector<std::string>>> _primary_key,
        std::vector<ForeignKey> &foreign_keys
    ) {
        table_id = _table_id;
        name = std::move(_name);
        fields = std::move(_fields);
        this->primary_key = std::move(_primary_key);
        this->foreign_keys = std::move(foreign_keys);

        if (primary_key) {
            // std::cerr << "PRIMARY KEY!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
            // std::cerr << "pk_name = " << primary_key.value().first << std::endl;
            for (const auto &field_name : primary_key.value().second) {
                // std::cerr << "field_name = " << field_name << std::endl;
                for (const auto &field : fields) {
                    if (field->name == field_name) {
                        // std::cerr << "NOT NULL !!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
                        field->not_null = true;
                        break;
                    }
                }
            }
        }
    }

    void store(int fileID, BufPageManager* &bpm) const {
        int pageID = 0;
        int index;
        // BufType: unsigned int*, i.e. 32 bits(4 bytes) per unit
        BufType b = bpm->allocPage(fileID, pageID, index, true);
        int offset = 0;

        // store table_id
        b[offset] = table_id;
        offset++;

        // store name
        int32_t name_size = name.size();
        b[offset] = name_size;
        offset++;
        int block_size = (name_size + 3) / 4; // 4 char per unit
        memcpy(b + offset, name.c_str(), name_size);
        offset += block_size;

        int32_t fields_size = fields.size();
        b[offset] = fields_size;
        offset++;

        // store field meta data
        for (const auto &field : fields) {
            field->store(b, offset);
        }

        // store primary key
        if (primary_key) {
            // store sign bit
            b[offset] = 1;
            offset++;
            // store pk name
            int32_t pk_name_size = primary_key.value().first.size();
            b[offset] = pk_name_size;
            offset++;
            int block_size = (pk_name_size + 3) / 4; // 4 char per unit
            memcpy(b + offset, primary_key.value().first.c_str(), pk_name_size);
            offset += block_size;

            // store pk fields
            int32_t pk_fields_size = primary_key.value().second.size();
            b[offset] = pk_fields_size;
            offset++;
            for (const auto &field_name : primary_key.value().second) {
                int32_t field_name_size = field_name.size();
                // store field_id
                for (const auto &field : fields) {
                    if (field->name == field_name) {
                        b[offset] = field->id;
                        offset++;
                        break;
                    }
                }
            }
        } else {
            // sign bit
            b[offset] = 0;
            offset++;
        }

        // store foreign keys
        int32_t foreign_keys_size = foreign_keys.size();
        b[offset] = foreign_keys_size;
        offset++;
        for (const auto &foreign_key : foreign_keys) {
            // store foreign_key_name
            int32_t foreign_key_name_size = std::get<0>(foreign_key).size();
            b[offset] = foreign_key_name_size;
            offset++;
            int block_size = (foreign_key_name_size + 3) / 4; // 4 char per unit
            memcpy(b + offset, std::get<0>(foreign_key).c_str(), foreign_key_name_size);
            offset += block_size;

            // store reference_table_name
            int32_t reference_table_name_size = std::get<1>(foreign_key).size();
            b[offset] = reference_table_name_size;
            offset++;
            block_size = (reference_table_name_size + 3) / 4; // 4 char per unit
            memcpy(b + offset, std::get<1>(foreign_key).c_str(), reference_table_name_size);
            offset += block_size;

            // store field_names
            int32_t field_names_size = std::get<2>(foreign_key).size();
            b[offset] = field_names_size;
            offset++;
            for (const auto &field_name : std::get<2>(foreign_key)) {
                int32_t field_name_size = field_name.size();
                b[offset] = field_name_size;
                offset++;
                block_size = (field_name_size + 3) / 4; // 4 char per unit
                memcpy(b + offset, field_name.c_str(), field_name_size);
                offset += block_size;
            }

            // store reference_field_names
            int32_t reference_field_names_size = std::get<3>(foreign_key).size();
            b[offset] = reference_field_names_size;
            offset++;
            for (const auto &reference_field_name : std::get<3>(foreign_key)) {
                int32_t reference_field_name_size = reference_field_name.size();
                b[offset] = reference_field_name_size;
                offset++;
                block_size = (reference_field_name_size + 3) / 4; // 4 char per unit
                memcpy(b + offset, reference_field_name.c_str(), reference_field_name_size);
                offset += block_size;
            }
        }

        // store indexs
        int32_t indexs_size = indexs.size();
        b[offset] = indexs_size;
        offset ++;
        for (const auto &index : indexs) {
            // store index_name
            int32_t index_name_size = index.first.size();
            b[offset] = index_name_size;
            offset++;
            int block_size = (index_name_size + 3) / 4; // 4 char per unit
            memcpy(b + offset, index.first.c_str(), index_name_size);
            offset += block_size;

            // store field_name
            int32_t field_name_size = index.second.size();
            b[offset] = field_name_size;
            offset++;
            block_size = (field_name_size + 3) / 4; // 4 char per unit
            memcpy(b + offset, index.second.c_str(), field_name_size);
            offset += block_size;
        }
        bpm->markDirty(index);
        bpm->writeBack(index);
    }

    void load(int fileID, BufPageManager* &bpm) {
        int pageID = 0;
        int index;
        BufType b = bpm->getPage(fileID, pageID, index);
        int offset = 0;

        // load table_id
        table_id = b[offset];
        offset++;

        // load name
        int32_t name_size = b[offset];
        offset++;
        int block_size = (name_size + 3) / 4; // 4 char per unit
        char name[name_size + 1]; // +1: for ending symbol '\0'
        memcpy(name, b + offset, name_size);
        name[name_size] = '\0';
        this->name = name;
        offset += block_size;

        // load fields_size
        int32_t fields_size = b[offset];
        offset++;
        for (int i = 0; i < fields_size; i++) {
            FieldMetaData field_meta;
            field_meta.load(b, offset);
            fields.push_back(std::make_shared<FieldMetaData>(field_meta));
        }

        // load primary key
        bool has_pk = b[offset];
        offset++;
        if (has_pk) {
            // load pk name
            int32_t pk_name_size = b[offset];
            offset++;
            int block_size = (pk_name_size + 3) / 4; // 4 char per unit
            char pk_name[pk_name_size + 1]; // +1: for ending symbol '\0'
            memcpy(pk_name, b + offset, pk_name_size);
            pk_name[pk_name_size] = '\0';
            std::string pk_name_str = pk_name;
            offset += block_size;

            // load pk fields
            int32_t pk_fields_size = b[offset];
            offset++;
            std::vector<std::string> pk_fields;
            for (int i = 0; i < pk_fields_size; i++) {
                FieldID field_id = b[offset];
                offset++;
                pk_fields.push_back(fields[field_id]->name);
            }
            primary_key = std::make_pair(pk_name_str, pk_fields);
        }

        // load foreign keys
        int32_t foreign_keys_size = b[offset];
        offset++;
        for (int i = 0; i < foreign_keys_size; i++) {
            // load foreign_key_name
            int32_t foreign_key_name_size = b[offset];
            offset++;
            int block_size = (foreign_key_name_size + 3) / 4; // 4 char per unit
            char foreign_key_name[foreign_key_name_size + 1]; // +1: for ending symbol '\0'
            memcpy(foreign_key_name, b + offset, foreign_key_name_size);
            foreign_key_name[foreign_key_name_size] = '\0';
            std::string foreign_key_name_str = foreign_key_name;
            offset += block_size;

            // load reference_table_name
            int32_t reference_table_name_size = b[offset];
            offset++;
            block_size = (reference_table_name_size + 3) / 4; // 4 char per unit
            char reference_table_name[reference_table_name_size + 1]; // +1: for ending symbol '\0'
            memcpy(reference_table_name, b + offset, reference_table_name_size);
            reference_table_name[reference_table_name_size] = '\0';
            std::string reference_table_name_str = reference_table_name;
            offset += block_size;

            // load field_names
            int32_t field_names_size = b[offset];
            offset++;
            std::vector<std::string> field_names;
            for (int j = 0; j < field_names_size; j++) {
                int32_t field_name_size = b[offset];
                offset++;
                block_size = (field_name_size + 3) / 4; // 4 char per unit
                char field_name[field_name_size + 1]; // +1: for ending symbol '\0'
                memcpy(field_name, b + offset, field_name_size);
                field_name[field_name_size] = '\0';
                std::string field_name_str = field_name;
                offset += block_size;
                field_names.push_back(field_name_str);
            }

            // load reference_field_names
            int32_t reference_field_names_size = b[offset];
            offset++;
            std::vector<std::string> reference_field_names;
            for (int j = 0; j < reference_field_names_size; j++) {
                int32_t reference_field_name_size = b[offset];
                offset++;
                block_size = (reference_field_name_size + 3) / 4;
                char reference_field_name[reference_field_name_size + 1];
                memcpy(reference_field_name, b + offset, reference_field_name_size);
                reference_field_name[reference_field_name_size] = '\0';
                std::string reference_field_name_str = reference_field_name;
                offset += block_size;
                reference_field_names.push_back(reference_field_name_str);
            }

            foreign_keys.push_back(std::make_tuple(
                foreign_key_name_str, 
                reference_table_name_str, 
                field_names, 
                reference_field_names
            ));
        }
        // load indexs
        int32_t indexs_size = b[offset];
        offset ++;
        for (int i = 0; i < indexs_size; i++) {
            // load index_name
            int32_t index_name_size = b[offset];
            offset++;
            int block_size = (index_name_size + 3) / 4; // 4 char per unit
            char index_name[index_name_size + 1]; // +1: for ending symbol '\0'
            memcpy(index_name, b + offset, index_name_size);
            index_name[index_name_size] = '\0';
            std::string index_name_str = index_name;
            offset += block_size;

            // load field_name
            int32_t field_name_size = b[offset];
            offset++;
            block_size = (field_name_size + 3) / 4; // 4 char per unit
            char field_name[field_name_size + 1]; // +1: for ending symbol '\0'
            memcpy(field_name, b + offset, field_name_size);
            field_name[field_name_size] = '\0';
            std::string field_name_str = field_name;
            offset += block_size;

            indexs.push_back(std::make_pair(index_name_str, field_name_str));
        }
        bpm->access(index);
        bpm->release(index);
    }

    std::string toString() const {
        std::string str = "TableMetaData: \n";
        str += "table_id: " + std::to_string(table_id) + "\n";
        str += "name: " + name + "\n";
        str += "fields: \n";
        for (const auto &field : fields) {
            str += FieldType2String(field.get()->type) + " " 
                + field.get()->name + " " 
                + "id=" + std::to_string(field.get()->id) + " " 
                + "not_null=" + std::to_string(field.get()->not_null) + " " 
                + "size=" + std::to_string(field.get()->size) + " "
                + "has_default=" + std::to_string(field.get()->has_default) + "\n";
        }
        return str;
    }

    RecordSize record_size() const {
        RecordSize size = 0;
        for (const auto &field : fields) {
            size += field.get()->field_size();
        }
        return size;
    }

    void addPrimaryKey(std::pair<std::string, std::vector<std::string>> _primary_key) {
        primary_key = std::move(_primary_key);
    }
};