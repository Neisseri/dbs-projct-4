#pragma once

#include <cstring>
#include "../record/record.hpp"
#include "../record/field.hpp"
#include "../record/metadata.hpp"
enum Op{
    EQ,
    NEQ,
    LT,
    GT,
    LE,
    GE,
};

std::string Op2String(Op op){
    if (op==EQ){
        return "=";
    }else if(op==NEQ){
        return "<>";
    }else if(op==LT){
        return "<";
    }else if(op==GT){
        return ">";
    }else if(op==LE){
        return "<=";
    }else if(op==GE){
        return ">=";
    }
}

class Condition{
    public:
    Condition(){}
    bool satisfy(std::string tableName,std::vector<Field> fields,std::vector<FieldMetaData> fieldMetaData){
        return true;
    }
};
//a = 1 || TB.a < 1 || TB.a == TB2.b
class WhereOperatorCondtion:public Condition{
    public:
    Op op;
    std::string tableName;
    std::string FieldName;
    std::string value;
    std::string tableName2;
    std::string FieldName2;
    WhereOperatorCondtion(std::string op,std::string tableName,std::string FieldName,std::string value){
        if (op=="="){
            this->op=EQ;
        }else if(op=="<>"){
            this->op=NEQ;
        }else if(op=="<"){
            this->op=LT;
        }else if(op==">"){
            this->op=GT;
        }else if(op=="<="){
            this->op=LE;
        }else if(op==">="){
            this->op=GE;
        }

        this->tableName=tableName;
        this->FieldName=FieldName;
        this->value=value;
    }
    WhereOperatorCondtion(std::string op,std::string tableName,std::string FieldName,std::string tableName2,std::string FieldName2){
        if (op=="="){
            this->op=EQ;
        }else if(op=="<>"){
            this->op=NEQ;
        }else if(op=="<"){
            this->op=LT;
        }else if(op==">"){
            this->op=GT;
        }else if(op=="<="){
            this->op=LE;
        }else if(op==">="){
            this->op=GE;
        }

        this->tableName=tableName;
        this->FieldName=FieldName;
        this->tableName2=tableName2;
        this->FieldName2=FieldName2;
    }
    bool satisfy(std::string tableName,std::vector<std::shared_ptr<Field>> fields,std::vector<shared_ptr<FieldMetaData>> fieldMetaData){
        if (this->tableName!="" && tableName!=this->tableName){
            return false;
        }
        //使用fieldMetaData找到对应的fieldId
        FieldID fieldId=-1;
        for (int i=0;i<fieldMetaData.size();i++){
            if (fieldMetaData[i]->name==this->FieldName){
                fieldId=fieldMetaData[i]->id;
                break;
            }
        }
        if (fieldId==-1){
            return false;
        }
        //判断fieldId对应的field是否满足条件
        auto field = fields[fieldId];
        if (this->op==EQ){
            if (field->type==FieldType::INT){
                auto intField = dynamic_pointer_cast<IntField>(field);
                return intField->value==std::stoi(this->value);
            }else if(field->type==FieldType::FLOAT){
                auto floatField = dynamic_pointer_cast<FloatField>(field);
                return floatField->value==std::stod(this->value);
            }else if(field->type==FieldType::VARCHAR){
                auto varcharField = dynamic_pointer_cast<VarcharField>(field);
                return varcharField->value==this->value;
            }
        }else if(this->op==NEQ){
            if (field->type==FieldType::INT){
                auto intField = dynamic_pointer_cast<IntField>(field);
                return intField->value!=std::stoi(this->value);
            }else if(field->type==FieldType::FLOAT){
                auto floatField = dynamic_pointer_cast<FloatField>(field);
                return floatField->value!=std::stod(this->value);
            }else if(field->type==FieldType::VARCHAR){
                auto varcharField = dynamic_pointer_cast<VarcharField>(field);
                return varcharField->value!=this->value;
            }
        }else if(this->op==LT){
            if (field->type==FieldType::INT){
                auto intField = dynamic_pointer_cast<IntField>(field);
                return intField->value<std::stoi(this->value);
            }else if(field->type==FieldType::FLOAT){
                auto floatField = dynamic_pointer_cast<FloatField>(field);
                return floatField->value<std::stod(this->value);
            }else if(field->type==FieldType::VARCHAR){
                auto varcharField = dynamic_pointer_cast<VarcharField>(field);
                return varcharField->value<this->value;
            }
        }else if(this->op==GT){
            if (field->type==FieldType::INT){
                auto intField = dynamic_pointer_cast<IntField>(field);
                return intField->value>std::stoi(this->value);

            }else if(field->type==FieldType::FLOAT){
                auto floatField = dynamic_pointer_cast<FloatField>(field);
                return floatField->value>std::stod(this->value);
            }else if(field->type==FieldType::VARCHAR){
                auto varcharField = dynamic_pointer_cast<VarcharField>(field);
                return varcharField->value>this->value;
            }
        }else if(this->op==LE){
            if (field->type==FieldType::INT){
                auto intField = dynamic_pointer_cast<IntField>(field);
                return intField->value<=std::stoi(this->value);
            }else if(field->type==FieldType::FLOAT){
                auto floatField = dynamic_pointer_cast<FloatField>(field);
                return floatField->value<=std::stod(this->value);

            }else if(field->type==FieldType::VARCHAR){
                auto varcharField = dynamic_pointer_cast<VarcharField>(field);
                return varcharField->value<=this->value;
            }
        }else if(this->op==GE){
            if (field->type==FieldType::INT){
                auto intField = dynamic_pointer_cast<IntField>(field);
                return intField->value>=std::stoi(this->value);

            }else if(field->type==FieldType::FLOAT){
                auto floatField = dynamic_pointer_cast<FloatField>(field);
                return floatField->value>=std::stod(this->value);
            }else if(field->type==FieldType::VARCHAR){
                auto varcharField = dynamic_pointer_cast<VarcharField>(field);
                return varcharField->value>=this->value;
            }
        }
        return false;

    }
    bool satisfy(std::string tableName,std::vector<std::shared_ptr<Field>> fields,std::vector<FieldMetaData> fieldMetaData,std::string tableName2,std::vector<std::shared_ptr<Field>> fields2,std::vector<FieldMetaData> fieldMetaData2){
        if (tableName!=this->tableName||tableName2!=this->tableName2){
            return false;
        }
        //使用fieldMetaData找到对应的fieldId
        FieldID fieldId1;
        FieldID fieldId2;
        for (int i=0;i<fieldMetaData.size();i++){
            if (fieldMetaData[i].name==this->FieldName){
                fieldId1=fieldMetaData[i].id;
                break;
            }
        }
        for (int i=0;i<fieldMetaData2.size();i++){
            if (fieldMetaData2[i].name==this->FieldName2){
                fieldId2=fieldMetaData2[i].id;
                break;
            }
        }
        if (fieldId1==-1||fieldId2==-1){
            return false;
        }
        //判断fieldId对应的field是否满足条件
        auto field1 = fields[fieldId1];
        auto field2 = fields2[fieldId2];
        if (this->op==EQ){
            if (field1->type==FieldType::INT){
                auto intField1 = dynamic_pointer_cast<IntField>(field1);
                auto intField2 = dynamic_pointer_cast<IntField>(field2);
                return intField1->value==intField2->value;
            }else if(field1->type==FieldType::FLOAT){
                auto floatField1 = dynamic_pointer_cast<FloatField>(field1);
                auto floatField2 = dynamic_pointer_cast<FloatField>(field2);
                return floatField1->value==floatField2->value;
            }else if(field1->type==FieldType::VARCHAR){
                auto varcharField1 = dynamic_pointer_cast<VarcharField>(field1);
                auto varcharField2 = dynamic_pointer_cast<VarcharField>(field2);
                return varcharField1->value==varcharField2->value;
            }
        }else if(this->op==NEQ){
            if (field1->type==FieldType::INT){
                auto intField1 = dynamic_pointer_cast<IntField>(field1);
                auto intField2 = dynamic_pointer_cast<IntField>(field2);
                return intField1->value!=intField2->value;
            }else if(field1->type==FieldType::FLOAT){
                auto floatField1 = dynamic_pointer_cast<FloatField>(field1);
                auto floatField2 = dynamic_pointer_cast<FloatField>(field2);
                return floatField1->value!=floatField2->value;

            }else if(field1->type==FieldType::VARCHAR){
                auto varcharField1 = dynamic_pointer_cast<VarcharField>(field1);
                auto varcharField2 = dynamic_pointer_cast<VarcharField>(field2);
                return varcharField1->value!=varcharField2->value;
            }
        }else if(this->op==LT){
            if (field1->type==FieldType::INT){
                auto intField1 = dynamic_pointer_cast<IntField>(field1);
                auto intField2 = dynamic_pointer_cast<IntField>(field2);
                return intField1->value<intField2->value;
            }else if(field1->type==FieldType::FLOAT){
                auto floatField1 = dynamic_pointer_cast<FloatField>(field1);
                auto floatField2 = dynamic_pointer_cast<FloatField>(field2);
                return floatField1->value<floatField2->value;
            }else if(field1->type==FieldType::VARCHAR){
                auto varcharField1 = dynamic_pointer_cast<VarcharField>(field1);
                auto varcharField2 = dynamic_pointer_cast<VarcharField>(field2);
                return varcharField1->value<varcharField2->value;
            }
        }else if(this->op==GT){
            if (field1->type==FieldType::INT){
               auto intField1 = dynamic_pointer_cast<IntField>(field1);
                auto intField2 = dynamic_pointer_cast<IntField>(field2);
                return intField1->value>intField2->value;
            }else if(field1->type==FieldType::FLOAT){
                auto floatField1 = dynamic_pointer_cast<FloatField>(field1);
                auto floatField2 = dynamic_pointer_cast<FloatField>(field2);
                return floatField1->value>floatField2->value;
            }else if(field1->type==FieldType::VARCHAR){
                auto varcharField1 = dynamic_pointer_cast<VarcharField>(field1);
                auto varcharField2 = dynamic_pointer_cast<VarcharField>(field2);
                return varcharField1->value>varcharField2->value;
            }
        }else if(this->op==LE){
            if (field1->type==FieldType::INT){
                auto intField1 = dynamic_pointer_cast<IntField>(field1);
                auto intField2 = dynamic_pointer_cast<IntField>(field2);
                return intField1->value<=intField2->value;

            }else if(field1->type==FieldType::FLOAT){
                auto floatField1 = dynamic_pointer_cast<FloatField>(field1);
                auto floatField2 = dynamic_pointer_cast<FloatField>(field2);
                return floatField1->value<=floatField2->value;

            }else if(field1->type==FieldType::VARCHAR){
                auto varcharField1 = dynamic_pointer_cast<VarcharField>(field1);
                auto varcharField2 = dynamic_pointer_cast<VarcharField>(field2);
                return varcharField1->value<=varcharField2->value;
            }
        }else if(this->op==GE){
            if (field1->type==FieldType::INT){
                auto intField1 = dynamic_pointer_cast<IntField>(field1);
                auto intField2 = dynamic_pointer_cast<IntField>(field2);
                return intField1->value>=intField2->value;
            }else if(field1->type==FieldType::FLOAT){
                auto floatField1 = dynamic_pointer_cast<FloatField>(field1);
                auto floatField2 = dynamic_pointer_cast<FloatField>(field2);
                return floatField1->value>=floatField2->value;
            }else if(field1->type==FieldType::VARCHAR){
                auto  varcharField1 = dynamic_pointer_cast<VarcharField>(field1);
                auto varcharField2 = dynamic_pointer_cast<VarcharField>(field2);
                return varcharField1->value>=varcharField2->value;
            }
        }

        return false;
    }

    bool satisfy_join(std::vector<std::string> tableNames,std::vector<std::shared_ptr<Field>> fields,std::vector<FieldMetaData> fieldMetaData){
        // 判断形如TB.a < 1 还是 TB.a == TB2.b
        if(this->FieldName2==""){
            //形如TB.a < 1
            //使用fieldMetaData找到对应的fieldId
            FieldID fieldId=-1;
            for (int i=0;i<fieldMetaData.size();i++){
                if (fieldMetaData[i].name==this->FieldName && tableNames[i]==this->tableName){
                    fieldId=i;
                    break;
                }
            }
            if (fieldId==-1){
                return false;
            }
            //判断fieldId对应的field是否满足条件
            auto field = fields[fieldId];
            if (this->op==EQ){
                if (field->type==FieldType::INT){
                    auto intField = dynamic_pointer_cast<IntField>(field);
                    return intField->value==std::stoi(this->value);
                }else if(field->type==FieldType::FLOAT){
                    auto floatField = dynamic_pointer_cast<FloatField>(field);
                    return floatField->value==std::stod(this->value);
                }else if(field->type==FieldType::VARCHAR){
                    auto varcharField = dynamic_pointer_cast<VarcharField>(field);
                    return varcharField->value==this->value;
                }
            }else if(this->op==NEQ){
                if (field->type==FieldType::INT){
                    auto intField = dynamic_pointer_cast<IntField>(field);
                    return intField->value!=std::stoi(this->value);
                }else if(field->type==FieldType::FLOAT){
                    auto floatField = dynamic_pointer_cast<FloatField>(field);
                    return floatField->value!=std::stod(this->value);
                }else if(field->type==FieldType::VARCHAR){
                    auto varcharField = dynamic_pointer_cast<VarcharField>(field);
                    return varcharField->value!=this->value;
                }
            }else if(this->op==LT){
                if (field->type==FieldType::INT){
                    auto intField = dynamic_pointer_cast<IntField>(field);
                    return intField->value<std::stoi(this->value);
                }else if(field->type==FieldType::FLOAT){
                    auto floatField = dynamic_pointer_cast<FloatField>(field);
                    return floatField->value<std::stod(this->value);
                }else if(field->type==FieldType::VARCHAR){
                    auto varcharField = dynamic_pointer_cast<VarcharField>(field);
                    return varcharField->value<this->value;
                }
            }else if(this->op==GT){
                if (field->type==FieldType::INT){
                    auto intField = dynamic_pointer_cast<IntField>(field);
                    return intField->value>std::stoi(this->value);

                }else if(field->type==FieldType::FLOAT){
                    auto floatField = dynamic_pointer_cast<FloatField>(field);
                    return floatField->value>std::stof(this->value);
                }else if(field->type==FieldType::VARCHAR){
                    auto varcharField = dynamic_pointer_cast<VarcharField>(field);
                    return varcharField->value>this->value;
                }
            }else if(this->op==LE){
                if (field->type==FieldType::INT){
                    auto intField = dynamic_pointer_cast<IntField>(field);
                    return intField->value<=std::stoi(this->value);
                }else if(field->type==FieldType::FLOAT){
                    auto floatField = dynamic_pointer_cast<FloatField>(field);
                    return floatField->value<=std::stod(this->value);

                }else if(field->type==FieldType::VARCHAR){
                    auto varcharField = dynamic_pointer_cast<VarcharField>(field);
                    return varcharField->value<=this->value;
                }
            }else if(this->op==GE){
                if (field->type==FieldType::INT){
                    auto intField = dynamic_pointer_cast<IntField>(field);
                    return intField->value>=std::stoi(this->value);

                }else if(field->type==FieldType::FLOAT){
                    auto floatField = dynamic_pointer_cast<FloatField>(field);
                    return floatField->value>=std::stod(this->value);
                }else if(field->type==FieldType::VARCHAR){
                    auto varcharField = dynamic_pointer_cast<VarcharField>(field);
                    return varcharField->value>=this->value;
                }
            }

        }else{
            //形如TB.a == TB2.b
            //使用fieldMetaData找到对应的fieldId
            FieldID fieldId1;
            FieldID fieldId2;
            for (int i=0;i<fieldMetaData.size();i++){
                if (fieldMetaData[i].name==this->FieldName && tableNames[i]==this->tableName){
                    fieldId1=i;
                    break;
                }
            }
            for (int i=0;i<fieldMetaData.size();i++){
                if (fieldMetaData[i].name==this->FieldName2 && tableNames[i]==this->tableName2){
                    fieldId2=i;
                    break;
                }
            }
            // std::cerr << "field_id1 = " << fieldId1 << std::endl;
            // std::cerr << "field_id2 = " << fieldId2 << std::endl;

            if (fieldId1==-1||fieldId2==-1){
                return false;
            }
            //判断fieldId对应的field是否满足条件
            auto field1 = fields[fieldId1];
            auto field2 = fields[fieldId2];
            if (this->op==EQ){ // == 
                if (field1->type==FieldType::INT){
                    auto intField1 = dynamic_pointer_cast<IntField>(field1);
                    auto intField2 = dynamic_pointer_cast<IntField>(field2);
                    return intField1->value==intField2->value;
                }else if(field1->type==FieldType::FLOAT){
                    auto floatField1 = dynamic_pointer_cast<FloatField>(field1);
                    auto floatField2 = dynamic_pointer_cast<FloatField>(field2);
                    return floatField1->value==floatField2->value;
                }else if(field1->type==FieldType::VARCHAR){
                    auto varcharField1 = dynamic_pointer_cast<VarcharField>(field1);
                    auto varcharField2 = dynamic_pointer_cast<VarcharField>(field2);
                    return varcharField1->value==varcharField2->value;
                }
            }else if(this->op==NEQ){ // !=
                if (field1->type==FieldType::INT){
                    auto intField1 = dynamic_pointer_cast<IntField>(field1);
                    auto intField2 = dynamic_pointer_cast<IntField>(field2);
                    return intField1->value!=intField2->value;
                }else if(field1->type==FieldType::FLOAT){
                    auto floatField1 = dynamic_pointer_cast<FloatField>(field1);
                    auto floatField2 = dynamic_pointer_cast<FloatField>(field2);
                    return floatField1->value!=floatField2->value;

                }else if(field1->type==FieldType::VARCHAR){
                    auto varcharField1 = dynamic_pointer_cast<VarcharField>(field1);
                    auto varcharField2 = dynamic_pointer_cast<VarcharField>(field2);
                    return varcharField1->value!=varcharField2->value;
                }
            }else if(this->op==LT){ // <
                if (field1->type==FieldType::INT){
                    auto intField1 = dynamic_pointer_cast<IntField>(field1);
                    auto intField2 = dynamic_pointer_cast<IntField>(field2);
                    return intField1->value<intField2->value;
                }else if(field1->type==FieldType::FLOAT){
                    auto floatField1 = dynamic_pointer_cast<FloatField>(field1);
                    auto floatField2 = dynamic_pointer_cast<FloatField>(field2);
                    return floatField1->value<floatField2->value;
                }else if(field1->type==FieldType::VARCHAR){
                    auto varcharField1 = dynamic_pointer_cast<VarcharField>(field1);
                    auto varcharField2 = dynamic_pointer_cast<VarcharField>(field2);
                    return varcharField1->value<varcharField2->value;
                }
            }else if(this->op==GT){
                if (field1->type==FieldType::INT){
                   auto intField1 = dynamic_pointer_cast<IntField>(field1);
                    auto intField2 = dynamic_pointer_cast<IntField>(field2);
                    return intField1->value>intField2->value;
                }else if(field1->type==FieldType::FLOAT){
                    auto floatField1 = dynamic_pointer_cast<FloatField>(field1);
                    auto floatField2 = dynamic_pointer_cast<FloatField>(field2);
                    return floatField1->value>floatField2->value;
                }else if(field1->type==FieldType::VARCHAR){
                    auto varcharField1 = dynamic_pointer_cast<VarcharField>(field1);
                    auto varcharField2 = dynamic_pointer_cast<VarcharField>(field2);
                    return varcharField1->value>varcharField2->value;
                }
            }else if(this->op==LE){
                if (field1->type==FieldType::INT){
                    auto intField1 = dynamic_pointer_cast<IntField>(field1);
                    auto intField2 = dynamic_pointer_cast<IntField>(field2);
                    return intField1->value<=intField2->value;

                }else if(field1->type==FieldType::FLOAT){
                    auto floatField1 = dynamic_pointer_cast<FloatField>(field1);
                    auto floatField2 = dynamic_pointer_cast<FloatField>(field2);
                    return floatField1->value<=floatField2->value;

                }else if(field1->type==FieldType::VARCHAR){
                    auto varcharField1 = dynamic_pointer_cast<VarcharField>(field1);
                    auto varcharField2 = dynamic_pointer_cast<VarcharField>(field2);
                    return varcharField1->value<=varcharField2->value;
                }
            }else if(this->op==GE){
                if (field1->type==FieldType::INT){
                    auto intField1 = dynamic_pointer_cast<IntField>(field1);
                    auto intField2 = dynamic_pointer_cast<IntField>(field2);
                    return intField1->value>=intField2->value;
                }else if(field1->type==FieldType::FLOAT){
                    auto floatField1 = dynamic_pointer_cast<FloatField>(field1);
                    auto floatField2 = dynamic_pointer_cast<FloatField>(field2);
                    return floatField1->value>=floatField2->value;
                }else if(field1->type==FieldType::VARCHAR){
                    auto  varcharField1 = dynamic_pointer_cast<VarcharField>(field1);
                    auto varcharField2 = dynamic_pointer_cast<VarcharField>(field2);
                    return varcharField1->value>=varcharField2->value;
                }
            }
        }
    }
};

std::string inst;
std::string last_inst;

// a IS NOT NULL || TB.a IS NOT NULL
class WhereIsNotNullCondition:public Condition{
    public:
    std::string tableName;
    std::string FieldName;
    WhereIsNotNullCondition(std::string tableName,std::string FieldName){
        this->tableName=tableName;
        this->FieldName=FieldName;
    }
    bool satisfy(std::vector<std::string> tableNames,std::vector<std::shared_ptr<Field>> fields,std::vector<FieldMetaData> fieldMetaData){
        //使用fieldMetaData找到对应的fieldId
        FieldID fieldId=-1;
        for(int i=0;i<fieldMetaData.size();i++){
            if (fieldMetaData[i].name==this->FieldName && ( this->tableName == "" || tableNames[i]==this->tableName)){
                fieldId=fieldMetaData[i].id;
                break;
            }
        }
        if (fieldId==-1){
            return false;
        }
        //判断fieldId对应的field是否满足条件
        auto field = fields[fieldId];
        return !field->is_null;
    }
};

// a IS NULL || TB.a IS NULL
class WhereIsNullCondition:public Condition{
    public:
    std::string tableName;
    std::string FieldName;
    WhereIsNullCondition(std::string tableName,std::string FieldName){
        this->tableName=tableName;
        this->FieldName=FieldName;
    }
    bool satisfy(std::vector<std::string> tableNames,std::vector<std::shared_ptr<Field>> fields,std::vector<FieldMetaData> fieldMetaData){
        
        //使用fieldMetaData找到对应的fieldId
        FieldID fieldId=-1;
        for(int i=0;i<fieldMetaData.size();i++){
            if (fieldMetaData[i].name==this->FieldName && ( this->tableName == "" || tableNames[i]==this->tableName)){
                fieldId=fieldMetaData[i].id;
                break;
            }
        }
        if (fieldId==-1){
            return false;
        }
        //判断fieldId对应的field是否满足条件
        auto field = fields[fieldId];
        return field->is_null;
    }
};

// TB.a IN (1,2,3)
class WhereInCondition:public Condition{
    public:
    std::string tableName;
    std::string FieldName;
    std::vector<std::string> values;
    WhereInCondition(std::string tableName,std::string FieldName,std::vector<std::string> values){
        this->tableName=tableName;
        this->FieldName=FieldName;
        this->values=values;
    }
    // tableNames是拼接后的表名，fields是拼接后的fields，fieldMetaData是拼接后的fieldMetaData
    bool satisfy(std::vector<std::string> tableNames,std::vector<std::shared_ptr<Field>> fields,std::vector<FieldMetaData> fieldMetaData){
        //使用fieldMetaData找到对应的fieldId
        FieldID fieldId=-1;
        for (int i=0;i<fieldMetaData.size();i++){
            if (fieldMetaData[i].name==this->FieldName && ( this->tableName == "" || tableNames[i]==this->tableName)){
                fieldId=fieldMetaData[i].id;
                break;
            }
        }
        if (fieldId==-1){
            return false;
        }
        //判断fieldId对应的field是否满足条件
        auto field = fields[fieldId];
        if (field->type==FieldType::INT){
            auto intField = dynamic_pointer_cast<IntField>(field);
            for (int i=0;i<this->values.size();i++){
                if (intField->value==std::stoi(this->values[i])){
                    return true;
                }
            }
        }else if(field->type==FieldType::FLOAT){
            auto floatField = dynamic_pointer_cast<FloatField>(field);
            for (int i=0;i<this->values.size();i++){
                if (floatField->value==std::stod(this->values[i])){
                    return true;
                }
            }
        }else if(field->type==FieldType::VARCHAR){
            auto varcharField = dynamic_pointer_cast<VarcharField>(field);
            for (int i=0;i<this->values.size();i++){
                if (varcharField->value==this->values[i]){
                    return true;
                }
            }
        }
        return false;
    }
};

// a LIKE 'abc%' || TB.a LIKE 'abc%'
class WhereLikeCondition:public Condition{
    public:
    std::string tableName;
    std::string FieldName;
    std::regex pattern;
    WhereLikeCondition(std::string tableName,std::string FieldName,std::string pattern){
        this->tableName=tableName;
        this->FieldName=FieldName;
        std::regex special_char{R"([-[\]{}()*+?.,\^$|#\s])"};
        std::string input_pattern{std::regex_replace(pattern, special_char, R"(\$&)")};
        // replace % with .*
        input_pattern = std::regex_replace(input_pattern, std::regex{"%"}, ".*");
        // replace _ with .
        input_pattern = std::regex_replace(input_pattern, std::regex{"_"}, ".");
        input_pattern = "^" + input_pattern + "$";
        this->pattern = input_pattern;

    }
    // tableNames是拼接后的表名，fields是拼接后的fields，fieldMetaData是拼接后的fieldMetaData
    bool satisfy(std::vector<std::string> tablenames,std::vector<std::shared_ptr<Field>> fields,std::vector<FieldMetaData> fieldMetaData){
        //使用fieldMetaData找到对应的fieldId
        FieldID fieldId=-1;
        for (int i=0;i<fieldMetaData.size();i++){
            if (fieldMetaData[i].name==this->FieldName && ( this->tableName == "" || tablenames[i]==this->tableName)){
                fieldId=fieldMetaData[i].id;
                break;
            }
        }
        if (fieldId==-1){
            return false;
        }
        //判断fieldId对应的field是否满足条件
        auto field = fields[fieldId];
        if (field->type==FieldType::VARCHAR){
            auto varcharField = dynamic_pointer_cast<VarcharField>(field);
            return std::regex_match(varcharField->value, this->pattern);
        }
        return false;
    }
};


bool shouldUseIndex(std::vector<std::shared_ptr<WhereOperatorCondtion>> conditions){
    //要求每一个条件都是形如(TB.)a op 1 并且op不是<>
    //其中a 属于 P_PARTKEY，
    //  R_REGIONKEY， 
    // S_SUPPKEY，
    //  C_CUSTKEY， 
    // O_ORDERKEY， 
    // L_ORDERKEY， L_LINENUMBER
    // N_NATIONKEY， 
    // PS_PARTKEY， PS_SUPPKEY

    for (auto & condition : conditions){
        if (condition->FieldName2!=""){
            return false;
        }
        if (condition->op==NEQ){
            return false;
        }
        if (condition->FieldName!="P_PARTKEY" && condition->FieldName!="S_SUPPKEY" && condition->FieldName!="C_CUSTKEY" && condition->FieldName!="O_ORDERKEY" && condition->FieldName!="L_ORDERKEY" && condition->FieldName!="L_LINENUMBER" && condition->FieldName!="N_NATIONKEY" && condition->FieldName!="PS_PARTKEY" && condition->FieldName!="PS_SUPPKEY"){
            return false;
        }
    }
    return true;
}

void calInterval(std::vector<shared_ptr<WhereOperatorCondtion>> conditions,int & lower_bound, int & upper_bound){
    lower_bound = INT_MIN;
    upper_bound = INT_MAX;
    // 返回的区间是 [lower_bound, upper_bound]
    for (auto & condition : conditions){
        int value = std::stoi(condition->value);
        if(condition->op == EQ){
            lower_bound = std::max(lower_bound, value);
            upper_bound = std::min(upper_bound, value);
        }else if(condition->op == LT){
            upper_bound = std::min(upper_bound, value - 1);
        }else if(condition->op == GT){
            lower_bound = std::max(lower_bound, value + 1);
        }else if(condition->op == LE){
            upper_bound = std::min(upper_bound, value);
        }else if(condition->op == GE){
            lower_bound = std::max(lower_bound, value);
        }
    }
}