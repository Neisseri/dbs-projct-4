#pragma once

#include <vector>
#include <iostream>
#include <fstream>

#include "constants.h"
#include "record.hpp"
#include "metadata.hpp"
#include "../grammar/condition.h"

#include "../filesystem/bufmanager/BufPageManager.h"
#include "../filesystem/fileio/FileManager.h"
#include "../filesystem/utils/pagedef.h"
typedef std::vector<std::shared_ptr<Record>> RecordList;
class Bitmap
{
public:
    RecordNum record_num;
    RecordNum max_addr;            // record_num and max_addr start from 1
    RecordNum record_num_per_page; // const
    std::vector<uint32_t> bitmap;

    explicit Bitmap() = default;
    explicit Bitmap(RecordSize record_size)
    {
        record_num = 0;
        max_addr = 0;
        record_num_per_page = PAGE_SIZE / 4 / record_size; // record_size: 32 bytes as a unit
    }

    void store(std::string bitmap_path)
    {
        // clear the file
        ofstream truncateFile(bitmap_path, ios::trunc);
        truncateFile.close();

        ofstream ofs(bitmap_path);
        ofs << record_num;
        ofs << " ";
        ofs << max_addr;
        ofs << " ";
        ofs << record_num_per_page;
        ofs << " ";
        for (auto &bit : bitmap)
        {
            ofs << bit;
            ofs << " ";
        }
        ofs.close();
    }

    void load(std::string bitmap_path)
    {
        ifstream ifs(bitmap_path);
        ifs >> record_num;
        ifs >> max_addr;
        ifs >> record_num_per_page;
        for (int i = 0; i < (max_addr + 31) / 32; i++)
        {
            uint32_t bit;
            ifs >> bit;
            bitmap.push_back(bit);
        }
        ifs.close();
    }

    std::string toString()
    {
        std::string str;
        str += "record num = " + std::to_string(record_num) + '\n';
        str += "max addr = " + std::to_string(max_addr) + '\n';
        str += "record num per page = " + std::to_string(record_num_per_page) + '\n';
        str += "bitmap = ";
        for (int i = 0; i < max_addr; i++)
        {
            if (i % 32 == 0)
            {
                str += '\n';
            }
            str += std::to_string((bitmap[i / 32] >> (31 - i % 32)) & 1);
        }
        str += '\n';
        return str;
    }

    void addRecord()
    {
        record_num++;
        max_addr++;
        if (max_addr % 32 == 1)
        { // == 1, 33 ...
            bitmap.push_back(0);
        }
        int block = bitmap.size() - 1; // last block
        int offset = (max_addr - 1) % 32;
        // offset :  0 ~ 31
        //     << : 31 ~ 0
        bitmap[block] |= (1 << (31 - offset));
    }

    void printRecordOrderBy(
        int fileID,
        BufPageManager *&bpm,
        int record_size,
        std::vector<std::shared_ptr<FieldMetaData>> &fields,
        std::string table_name,
        std::string orderByFieldName,
        bool desc,
        int limit,
        int offset
        )
    {
        RecordList recordList;
        for (int i = 1; i <= max_addr; i++)
        {
            int bit = (bitmap[(i - 1) / 32] >> (31 - (i - 1) % 32)) & 1;
            if (bit == 1)
            {
                int pageID = (i - 1) / record_num_per_page + 2; // [2, ...]
                int slot = (i - 1) % record_num_per_page + 1;   // [1, 341] -> 1
                int index;
                BufType b = bpm->getPage(fileID, pageID, index);
                Record record_read;
                for (auto &field_meta : fields)
                {
                    if (field_meta.get()->type == FieldType::INT)
                    {
                        record_read.fields.push_back(std::make_shared<IntField>());
                    }
                    else if (field_meta.get()->type == FieldType::FLOAT)
                    {
                        record_read.fields.push_back(std::make_shared<FloatField>());
                    }
                    else if (field_meta.get()->type == FieldType::VARCHAR)
                    {
                        record_read.fields.push_back(
                            std::make_shared<VarcharField>("", false, field_meta.get()->size));
                    }
                }
                record_read.load(fileID, bpm, pageID, (slot - 1) * record_size);
                recordList.push_back(std::make_shared<Record>(record_read));
                bpm->access(index);
                bpm->release(index);
            }
        }

        int fieldIndex = -1;

        for (int i = 0; i < fields.size(); i++)
        {
            if (fields[i]->name == orderByFieldName)
            {
                fieldIndex = i;
                break;
            }
        }
        std::sort(recordList.begin(), recordList.end(), [&](std::shared_ptr<Record> a, std::shared_ptr<Record> b)
                  {
                      // Field一定是INT
                      IntField *aField = (IntField *)a->fields[fieldIndex].get();
                      IntField *bField = (IntField *)b->fields[fieldIndex].get();
                      if (desc)
                      {

                          return aField->value > bField->value;
                      }
                      else
                      {
                          return aField->value < bField->value;
                      }
                  });
        // 可能有offset 和 limit
        if (offset != -1)
        {
            if (offset >= recordList.size())
            {
                return;
            }
            recordList.erase(recordList.begin(), recordList.begin() + offset);
        }
        if (limit != -1)
        {
            if (limit >= recordList.size())
            {
                limit = recordList.size();
            }
            recordList.erase(recordList.begin() + limit, recordList.end());
        }
        // 依次输出recordList
        for (auto record : recordList)
        {
            std::cout << record->toString();
        }
    }

    void printRecord(
        int fileID,
        BufPageManager *&bpm,
        int record_size,
        std::vector<std::shared_ptr<FieldMetaData>> &fields)
    {
        Record record_read;
        // initialization
        for (auto &field_meta : fields)
        {
            if (field_meta.get()->type == FieldType::INT)
            {
                record_read.fields.push_back(std::make_shared<IntField>());
            }
            else if (field_meta.get()->type == FieldType::FLOAT)
            {
                record_read.fields.push_back(std::make_shared<FloatField>());
            }
            else if (field_meta.get()->type == FieldType::VARCHAR)
            {
                record_read.fields.push_back(
                    std::make_shared<VarcharField>("", false, field_meta.get()->size));
            }
        }

        for (int i = 1; i <= max_addr; i++)
        {
            int bit = (bitmap[(i - 1) / 32] >> (31 - (i - 1) % 32)) & 1;
            if (bit == 1)
            {
                int pageID = (i - 1) / record_num_per_page + 2; // [2, ...]
                int slot = (i - 1) % record_num_per_page + 1;   // [1, 341] -> 1
                int index;
                BufType b = bpm->getPage(fileID, pageID, index);

                record_read.load(fileID, bpm, pageID, (slot - 1) * record_size);
                std::cout << record_read.toString();

                bpm->access(index);
                bpm->release(index);
            }
        }
    }

    void printRecordPageSlot(
        int fileID,
        BufPageManager *&bpm,
        std::vector<std::shared_ptr<FieldMetaData>> &fields,
        std::string table_name,
        int record_size,
        std::vector<int> page_ids,
        std::vector<int> slot_ids)
    {
        Record record_read;
        // initialization
        for (auto &field_meta : fields)
        {
            if (field_meta.get()->type == FieldType::INT)
            {
                record_read.fields.push_back(std::make_shared<IntField>());
            }
            else if (field_meta.get()->type == FieldType::FLOAT)
            {
                record_read.fields.push_back(std::make_shared<FloatField>());
            }
            else if (field_meta.get()->type == FieldType::VARCHAR)
            {
                record_read.fields.push_back(
                    std::make_shared<VarcharField>("", false, field_meta.get()->size));
            }
        }
        for (int i = 0; i < page_ids.size(); i++)
        {
            int page_id = page_ids[i];
            int slot = slot_ids[i];
            int index;
            BufType b = bpm->getPage(fileID, page_id, index);
            record_read.load(fileID, bpm, page_id, (slot - 1) * record_size);
            std::cout << record_read.toString();
            bpm->access(index);
            bpm->release(index);
        }
    }
    void printRecordPageSlotOrderBy(
        int fileID,
        BufPageManager *&bpm,
        std::vector<std::shared_ptr<FieldMetaData>> &fields,
        std::string table_name,
        int record_size,
        std::vector<int> page_ids,
        std::vector<int> slot_ids,
        std::string orderByFieldName,
        bool desc,
        int limit,
        int offset)
    {
        RecordList recordList;
        for (int i = 0; i < page_ids.size(); i++)
        {
            int page_id = page_ids[i];
            int slot = slot_ids[i];
            int index;
            BufType b = bpm->getPage(fileID, page_id, index);
            Record record_read;
            // initialization
            for (auto &field_meta : fields)
            {
                if (field_meta.get()->type == FieldType::INT)
                {
                    record_read.fields.push_back(std::make_shared<IntField>());
                }
                else if (field_meta.get()->type == FieldType::FLOAT)
                {
                    record_read.fields.push_back(std::make_shared<FloatField>());
                }
                else if (field_meta.get()->type == FieldType::VARCHAR)
                {
                    record_read.fields.push_back(
                        std::make_shared<VarcharField>("", false, field_meta.get()->size));
                }
            }
            record_read.load(fileID, bpm, page_id, (slot - 1) * record_size);
            recordList.push_back(std::make_shared<Record>(record_read));
            bpm->access(index);
            bpm->release(index);
        }
        int fieldIndex = -1;
        for (int i = 0; i < fields.size(); i++)
        {
            if (fields[i]->name == orderByFieldName)
            {
                fieldIndex = i;
                break;
            }
        }
        std::sort(recordList.begin(), recordList.end(), [&](std::shared_ptr<Record> a, std::shared_ptr<Record> b)
                  {
                      // Field一定是INT
                      IntField *aField = (IntField *)a->fields[fieldIndex].get();
                      IntField *bField = (IntField *)b->fields[fieldIndex].get();
                      if (desc)
                      {

                          return aField->value > bField->value;
                      }
                      else
                      {
                          return aField->value < bField->value;
                      }
                  });
        // 可能有offset 和 limit
        if (offset != -1)
        {
            if (offset >= recordList.size())
            {
                return;
            }
            recordList.erase(recordList.begin(), recordList.begin() + offset);
        }
        if (limit != -1)
        {
            if (limit >= recordList.size())
            {
                limit = recordList.size();
            }
            recordList.erase(recordList.begin() + limit, recordList.end());
        }
        // 依次输出recordList
        for (auto record : recordList)
        {
            std::cout << record->toString();
        }
    }
    void printRecordPageSlotOrderBy(
        int fileID,
        BufPageManager *&bpm,
        std::vector<std::shared_ptr<FieldMetaData>> &fields,
        std::string table_name,
        int record_size,
        std::vector<int> page_ids,
        std::vector<int> slot_ids,
        std::string orderByFieldName,
        bool desc,
        int limit,
        int offset,
        std::vector<std::string> &selectors)
    {
        RecordList recordList;
        for (int i = 0; i < page_ids.size(); i++)
        {
            int page_id = page_ids[i];
            int slot = slot_ids[i];
            int index;
            BufType b = bpm->getPage(fileID, page_id, index);
            Record record_read;
            // initialization
            for (auto &field_meta : fields)
            {
                if (field_meta.get()->type == FieldType::INT)
                {
                    record_read.fields.push_back(std::make_shared<IntField>());
                }
                else if (field_meta.get()->type == FieldType::FLOAT)
                {
                    record_read.fields.push_back(std::make_shared<FloatField>());
                }
                else if (field_meta.get()->type == FieldType::VARCHAR)
                {
                    record_read.fields.push_back(
                        std::make_shared<VarcharField>("", false, field_meta.get()->size));
                }
            }
            record_read.load(fileID, bpm, page_id, (slot - 1) * record_size);
            recordList.push_back(std::make_shared<Record>(record_read));
            bpm->access(index);
            bpm->release(index);
        }
        int fieldIndex = -1;
        for (int i = 0; i < fields.size(); i++)
        {
            if (fields[i]->name == orderByFieldName)
            {
                fieldIndex = i;
                break;
            }
        }
        std::sort(recordList.begin(), recordList.end(), [&](std::shared_ptr<Record> a, std::shared_ptr<Record> b)
                  {
                      // Field一定是INT
                      IntField *aField = (IntField *)a->fields[fieldIndex].get();
                      IntField *bField = (IntField *)b->fields[fieldIndex].get();
                      if (desc)
                      {

                          return aField->value > bField->value;
                      }
                      else
                      {
                          return aField->value < bField->value;
                      }
                  });
        // 可能有offset 和 limit
        if (offset != -1)
        {
            if (offset >= recordList.size())
            {
                return;
            }
            recordList.erase(recordList.begin(), recordList.begin() + offset);
        }
        if (limit != -1)
        {
            if (limit >= recordList.size())
            {
                limit = recordList.size();
            }
            recordList.erase(recordList.begin() + limit, recordList.end());
        }
        // 先输出表头

        // 按照selector的顺序输出recordList
        for (auto record : recordList)
        {
            for (auto selector : selectors)
            {
                for (int i = 0; i < fields.size(); i++)
                {
                    if (fields[i]->name == selector)
                    {
                        std::cout << record->fields[i]->toString();
                        if (selector != selectors.back())
                        {
                            std::cout << ",";
                        }
                        break;
                    }
                }
            }
            std::cout << std::endl;
        }
    }

    void printRecordWhere(
        int fileID,
        BufPageManager *&bpm,
        int record_size,
        std::vector<std::shared_ptr<FieldMetaData>> &fields,
        std::vector<std::shared_ptr<WhereOperatorCondtion>> &conditions,
        std::string table_name)
    {
        Record record_read;
        // initialization
        for (auto &field_meta : fields)
        {
            if (field_meta.get()->type == FieldType::INT)
            {
                record_read.fields.push_back(std::make_shared<IntField>());
            }
            else if (field_meta.get()->type == FieldType::FLOAT)
            {
                record_read.fields.push_back(std::make_shared<FloatField>());
            }
            else if (field_meta.get()->type == FieldType::VARCHAR)
            {
                record_read.fields.push_back(
                    std::make_shared<VarcharField>("", false, field_meta.get()->size));
            }
        }

        for (int i = 1; i <= max_addr; i++)
        {
            int bit = (bitmap[(i - 1) / 32] >> (31 - (i - 1) % 32)) & 1;
            if (bit == 1)
            {
                int pageID = (i - 1) / record_num_per_page + 2; // [2, ...]
                int slot = (i - 1) % record_num_per_page + 1;   // [1, 341] -> 1
                int index;
                BufType b = bpm->getPage(fileID, pageID, index);

                record_read.load(fileID, bpm, pageID, (slot - 1) * record_size);
                bool flag = true;
                for (auto &condition : conditions)
                {
                    if (condition->satisfy(table_name, record_read.fields, fields) == false)
                    {
                        flag = false;
                        break;
                    }
                }
                if (flag)
                {
                    std::cout << record_read.toString();
                }
                bpm->access(index);
                bpm->release(index);
            }
        }
    }

    void printRecordWhereSelector(
        int fileID,
        BufPageManager *&bpm,
        int record_size,
        std::vector<std::shared_ptr<FieldMetaData>> &fields,
        std::vector<std::shared_ptr<WhereOperatorCondtion>> &conditions,
        std::string table_name,
        std::vector<std::string> &selectors)
    {
        Record record_read;
        // initialization
        for (auto &field_meta : fields)
        {
            if (field_meta.get()->type == FieldType::INT)
            {
                record_read.fields.push_back(std::make_shared<IntField>());
            }
            else if (field_meta.get()->type == FieldType::FLOAT)
            {
                record_read.fields.push_back(std::make_shared<FloatField>());
            }
            else if (field_meta.get()->type == FieldType::VARCHAR)
            {
                record_read.fields.push_back(
                    std::make_shared<VarcharField>("", false, field_meta.get()->size));
            }
        }

        for (int i = 1; i <= max_addr; i++)
        {
            int bit = (bitmap[(i - 1) / 32] >> (31 - (i - 1) % 32)) & 1;
            if (bit == 1)
            {
                int pageID = (i - 1) / record_num_per_page + 2; // [2, ...]
                int slot = (i - 1) % record_num_per_page + 1;   // [1, 341] -> 1
                int index;
                BufType b = bpm->getPage(fileID, pageID, index);

                record_read.load(fileID, bpm, pageID, (slot - 1) * record_size);

                bool flag = true;
                for (auto &condition : conditions)
                {
                    if (condition->satisfy(table_name, record_read.fields, fields) == false)
                    {
                        flag = false;
                        break;
                    }
                }
                if (flag)
                {
                    for (auto selector : selectors)
                    {
                        for (int i = 0; i < fields.size(); i++)
                        {
                            if (fields[i]->name == selector)
                            {
                                std::cout << record_read.fields[i]->toString();
                                if (selector != selectors.back())
                                {
                                    std::cout << ",";
                                }
                                break;
                            }
                        }
                    }
                    std::cout << std::endl;
                }

                bpm->access(index);
                bpm->release(index);
            }
        }
    }

    void deleteRecord(int delete_addr)
    {
        record_num--;
        int block = (delete_addr - 1) / 32;  // [1: 32] -> 0
        int offset = (delete_addr - 1) % 32; // 1 -> 0
        bitmap[block] &= ~(1 << (31 - offset));
    }

    void update_table(
        int fileID,
        BufPageManager *&bpm,
        int record_size,
        std::vector<std::shared_ptr<FieldMetaData>> &fields,
        std::vector<std::shared_ptr<WhereOperatorCondtion>> &conditions,
        std::string table_name,
        std::vector<std::pair<std::string, std::string>> &updates)
    {
        Record record_read;
        // initialization
        for (auto &field_meta : fields)
        {
            if (field_meta.get()->type == FieldType::INT)
            {
                record_read.fields.push_back(std::make_shared<IntField>());
            }
            else if (field_meta.get()->type == FieldType::FLOAT)
            {
                record_read.fields.push_back(std::make_shared<FloatField>());
            }
            else if (field_meta.get()->type == FieldType::VARCHAR)
            {
                record_read.fields.push_back(
                    std::make_shared<VarcharField>("", false, field_meta.get()->size));
            }
        }

        std::cout << "rows" << std::endl;
        int counter = 0;
        for (int i = 1; i <= max_addr; i++)
        {
            int bit = (bitmap[(i - 1) / 32] >> (31 - (i - 1) % 32)) & 1;
            if (bit == 1)
            {
                int pageID = (i - 1) / record_num_per_page + 2; // [2, ...]
                int slot = (i - 1) % record_num_per_page + 1;   // [1, 341] -> 1
                int index;
                BufType b = bpm->getPage(fileID, pageID, index);

                record_read.load(fileID, bpm, pageID, (slot - 1) * record_size);

                bool flag = true;
                for (auto &condition : conditions)
                {
                    if (condition->satisfy(table_name, record_read.fields, fields) == false)
                    {
                        flag = false;
                        break;
                    }
                }
                if (flag)
                {
                    counter++;
                    // update record
                    for (auto &update : updates)
                    {
                        for (int i = 0; i < fields.size(); i++)
                        {
                            if (fields[i]->name == update.first)
                            {
                                if (fields[i]->type == FieldType::INT)
                                {
                                    record_read.fields[i] = std::make_shared<IntField>(std::stoi(update.second));
                                }
                                else if (fields[i]->type == FieldType::FLOAT)
                                {
                                    record_read.fields[i] = std::make_shared<FloatField>(std::stod(update.second));
                                }
                                else if (fields[i]->type == FieldType::VARCHAR)
                                {
                                    record_read.fields[i] = std::make_shared<VarcharField>(update.second, false, fields[i]->size);
                                }
                                break;
                            }
                        }
                    }
                    // write back record
                    record_read.store(fileID, bpm, pageID, (slot - 1) * record_size);
                }

                bpm->markDirty(index);
                bpm->writeBack(index);
            }
        }
        std::cout << counter << std::endl;
    }

    void deleteRecordWhere(
        int fileID,
        BufPageManager *&bpm,
        int record_size,
        std::vector<std::shared_ptr<FieldMetaData>> &fields,
        std::vector<std::shared_ptr<WhereOperatorCondtion>> &conditions,
        std::string table_name)
    {
        Record record_read;
        // initialization
        for (auto &field_meta : fields)
        {
            if (field_meta.get()->type == FieldType::INT)
            {
                record_read.fields.push_back(std::make_shared<IntField>());
            }
            else if (field_meta.get()->type == FieldType::FLOAT)
            {
                record_read.fields.push_back(std::make_shared<FloatField>());
            }
            else if (field_meta.get()->type == FieldType::VARCHAR)
            {
                record_read.fields.push_back(
                    std::make_shared<VarcharField>("", false, field_meta.get()->size));
            }
        }

        std::cout << "rows" << std::endl;
        int counter = 0;
        for (int i = 1; i <= max_addr; i++)
        {
            int bit = (bitmap[(i - 1) / 32] >> (31 - (i - 1) % 32)) & 1;
            if (bit == 1)
            {
                int pageID = (i - 1) / record_num_per_page + 2; // [2, ...]
                int slot = (i - 1) % record_num_per_page + 1;   // [1, 341] -> 1
                int index;
                BufType b = bpm->getPage(fileID, pageID, index);

                record_read.load(fileID, bpm, pageID, (slot - 1) * record_size);
                bool flag = true;
                for (auto &condition : conditions)
                {
                    if (condition->satisfy(table_name, record_read.fields, fields) == false)
                    {
                        flag = false;
                        break;
                    }
                }
                if (flag)
                {
                    counter++;
                    deleteRecord(i);
                }
                bpm->access(index);
                bpm->release(index);
            }
        }
        std::cout << counter << std::endl;
    }
};