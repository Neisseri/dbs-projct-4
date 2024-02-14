#pragma once

#include <cstring>

#include "constants.h"

#include "../filesystem/bufmanager/BufPageManager.h"
#include "../filesystem/fileio/FileManager.h"
#include "../filesystem/utils/pagedef.h"

enum class FieldType {
    INT = 0,
    FLOAT = 1,
    VARCHAR = 2
};

std::string FieldType2String(FieldType type) {
    switch (type) {
        case FieldType::INT:
            return "INT";
        case FieldType::FLOAT:
            return "FLOAT";
        case FieldType::VARCHAR:
            return "VARCHAR";
        default:
            return "UNKNOWN";
    }
}

class Field {
public:
    FieldType type;
    bool is_null;

    explicit Field() = default;
    explicit Field(FieldType type, bool is_null = false) : type(type), is_null(is_null) {}

    virtual ~Field() = default;

    // size of field
    virtual FieldSize size() const = 0;

    // return as string
    virtual std::string toString() const = 0;

    virtual void store(BufType &b, int &offset) const = 0;

    virtual void load(BufType &b, int &offset) = 0;
    // pure virtual function must be defined in derived class
};

class IntField : public Field {
public:
    int value;

    explicit IntField() : Field(FieldType::INT) {}
    explicit IntField(int value, bool is_null = false) : Field(FieldType::INT, is_null), value(value) {}

    ~IntField() override = default;

    FieldSize size() const override { return sizeof(int); }
    
    std::string toString() const override { return std::to_string(value); }

    void store(BufType &b, int &offset) const override {
        b[offset] = value;
        offset++;
    }

    void load(BufType &b, int &offset) override {
        value = b[offset];
        offset++;
    }
};

class FloatField : public Field {
public:
    double value;

    explicit FloatField() : Field(FieldType::FLOAT) {}
    explicit FloatField(double value, bool is_null = false) : Field(FieldType::FLOAT, is_null), value(value) {}

    ~FloatField() override = default;

    FieldSize size() const override { return sizeof(double); }

    std::string toString() const override {
        std::string ret;
        stringstream ss;
        ss << fixed << setprecision(2) << value;
        ss >> ret;
        return ret;
    }

    void store(BufType &b, int &offset) const override {
        // value: 8 bytes
        memcpy(b + offset, &value, sizeof(double));
        offset += 2;
    }

    void load(BufType &b, int &offset) override {
        memcpy(&value, b + offset, sizeof(double));
        offset += 2;
    }
};

class VarcharField : public Field {
public:
    std::string value;
    FieldSize max_len; // fixed length for string

    explicit VarcharField() : Field(FieldType::VARCHAR) {}
    explicit VarcharField(std::string value, bool is_null = false, FieldSize max_len = 20) : 
        Field(FieldType::VARCHAR, is_null), value(std::move(value)), max_len(max_len) {}

    ~VarcharField() override = default;

    // Varchar: length + data
    FieldSize size() const override { 
        return value.size() + sizeof(FieldSize); 
    }

    std::string toString() const override {
        return value;
    }

    void store(BufType &b, int &offset) const override {
        int block_size = (max_len + 3) / 4; // 4 char per unit
        memcpy(b + offset, value.c_str(), max_len);
        offset += block_size;
    }

    void load(BufType &b, int &offset) override {
        int block_size = (max_len + 3) / 4; // 4 char per unit
        char str[max_len + 1];
        memcpy(str, b + offset, max_len);
        str[max_len] = '\0';
        value = std::string(str);
        offset += block_size;
    }
};
