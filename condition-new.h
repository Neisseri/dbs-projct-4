#pragma once

#include <string>
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
class Condition{
    public:
    Condition(){}
    bool satisfy(std::string tableName,std::vector<Field> fields,std::vector<FieldMetaData> fieldMetaData){
        return true;
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
    // bool satisfy(std::string tableName,std::vector<std::shared_ptr<Field>> fields,std::vector<FieldMetaData> fieldMetaData){
    //     if (this->tableName!="" && tableName!=this->tableName){
    //         return false;
    //     }
    //     //使用fieldMetaData找到对应的fieldId
    //     FieldID fieldId=-1;
    //     for (int i=0;i<fieldMetaData.size();i++){
    //         if (fieldMetaData[i].name==this->FieldName){
    //             fieldId=fieldMetaData[i].id;
    //             break;
    //         }
    //     }
    //     if (fieldId==-1){
    //         return false;
    //     }
    //     //判断fieldId对应的field是否满足条件
    //     auto field = fields[fieldId];
    //     if (this->op==EQ){
    //         if (field->type==FieldType::INT){
    //             auto intField = dynamic_pointer_cast<IntField>(field);
    //             return intField->value==std::stoi(this->value);
    //         }else if(field->type==FieldType::FLOAT){
    //             auto floatField = dynamic_pointer_cast<FloatField>(field);
    //             return floatField->value==std::stof(this->value);
    //         }else if(field->type==FieldType::VARCHAR){
    //             auto varcharField = dynamic_pointer_cast<VarcharField>(field);
    //             return varcharField->value==this->value;
    //         }
    //     }else if(this->op==NEQ){
    //         if (field->type==FieldType::INT){
    //             auto intField = dynamic_pointer_cast<IntField>(field);
    //             return intField->value!=std::stoi(this->value);
    //         }else if(field->type==FieldType::FLOAT){
    //             auto floatField = dynamic_pointer_cast<FloatField>(field);
    //             return floatField->value!=std::stof(this->value);
    //         }else if(field->type==FieldType::VARCHAR){
    //             auto varcharField = dynamic_pointer_cast<VarcharField>(field);
    //             return varcharField->value!=this->value;
    //         }
    //     }else if(this->op==LT){
    //         if (field->type==FieldType::INT){
    //             auto intField = dynamic_pointer_cast<IntField>(field);
    //             return intField->value<std::stoi(this->value);
    //         }else if(field->type==FieldType::FLOAT){
    //             auto floatField = dynamic_pointer_cast<FloatField>(field);
    //             return floatField->value<std::stof(this->value);
    //         }else if(field->type==FieldType::VARCHAR){
    //             auto varcharField = dynamic_pointer_cast<VarcharField>(field);
    //             return varcharField->value<this->value;
    //         }
    //     }else if(this->op==GT){
    //         if (field->type==FieldType::INT){
    //             auto intField = dynamic_pointer_cast<IntField>(field);
    //             return intField->value>std::stoi(this->value);

    //         }else if(field->type==FieldType::FLOAT){
    //             auto floatField = dynamic_pointer_cast<FloatField>(field);
    //             return floatField->value>std::stof(this->value);
    //         }else if(field->type==FieldType::VARCHAR){
    //             auto varcharField = dynamic_pointer_cast<VarcharField>(field);
    //             return varcharField->value>this->value;
    //         }
    //     }else if(this->op==LE){
    //         if (field->type==FieldType::INT){
    //             auto intField = dynamic_pointer_cast<IntField>(field);
    //             return intField->value<=std::stoi(this->value);
    //         }else if(field->type==FieldType::FLOAT){
    //             auto floatField = dynamic_pointer_cast<FloatField>(field);
    //             return floatField->value<=std::stof(this->value);

    //         }else if(field->type==FieldType::VARCHAR){
    //             auto varcharField = dynamic_pointer_cast<VarcharField>(field);
    //             return varcharField->value<=this->value;
    //         }
    //     }else if(this->op==GE){
    //         if (field->type==FieldType::INT){
    //             auto intField = dynamic_pointer_cast<IntField>(field);
    //             return intField->value>=std::stoi(this->value);

    //         }else if(field->type==FieldType::FLOAT){
    //             auto floatField = dynamic_pointer_cast<FloatField>(field);
    //             return floatField->value>=std::stof(this->value);
    //         }else if(field->type==FieldType::VARCHAR){
    //             auto varcharField = dynamic_pointer_cast<VarcharField>(field);
    //             return varcharField->value>=this->value;
    //         }
    //     }
    //     return false;

    // }
    // bool satisfy(std::string tableName,std::vector<std::shared_ptr<Field>> fields,std::vector<FieldMetaData> fieldMetaData,std::string tableName2,std::vector<std::shared_ptr<Field>> fields2,std::vector<FieldMetaData> fieldMetaData2){
    //     if (tableName!=this->tableName||tableName2!=this->tableName2){
    //         return false;
    //     }
    //     //使用fieldMetaData找到对应的fieldId
    //     FieldID fieldId1;
    //     FieldID fieldId2;
    //     for (int i=0;i<fieldMetaData.size();i++){
    //         if (fieldMetaData[i].name==this->FieldName){
    //             fieldId1=fieldMetaData[i].id;
    //             break;
    //         }
    //     }
    //     for (int i=0;i<fieldMetaData2.size();i++){
    //         if (fieldMetaData2[i].name==this->FieldName2){
    //             fieldId2=fieldMetaData2[i].id;
    //             break;
    //         }
    //     }
    //     if (fieldId1==-1||fieldId2==-1){
    //         return false;
    //     }
    //     //判断fieldId对应的field是否满足条件
    //     auto field1 = fields[fieldId1];
    //     auto field2 = fields2[fieldId2];
    //     if (this->op==EQ){
    //         if (field1->type==FieldType::INT){
    //             auto intField1 = dynamic_pointer_cast<IntField>(field1);
    //             auto intField2 = dynamic_pointer_cast<IntField>(field2);
    //             return intField1->value==intField2->value;
    //         }else if(field1->type==FieldType::FLOAT){
    //             auto floatField1 = dynamic_pointer_cast<FloatField>(field1);
    //             auto floatField2 = dynamic_pointer_cast<FloatField>(field2);
    //             return floatField1->value==floatField2->value;
    //         }else if(field1->type==FieldType::VARCHAR){
    //             auto varcharField1 = dynamic_pointer_cast<VarcharField>(field1);
    //             auto varcharField2 = dynamic_pointer_cast<VarcharField>(field2);
    //             return varcharField1->value==varcharField2->value;
    //         }
    //     }else if(this->op==NEQ){
    //         if (field1->type==FieldType::INT){
    //             auto intField1 = dynamic_pointer_cast<IntField>(field1);
    //             auto intField2 = dynamic_pointer_cast<IntField>(field2);
    //             return intField1->value!=intField2->value;
    //         }else if(field1->type==FieldType::FLOAT){
    //             auto floatField1 = dynamic_pointer_cast<FloatField>(field1);
    //             auto floatField2 = dynamic_pointer_cast<FloatField>(field2);
    //             return floatField1->value!=floatField2->value;

    //         }else if(field1->type==FieldType::VARCHAR){
    //             auto varcharField1 = dynamic_pointer_cast<VarcharField>(field1);
    //             auto varcharField2 = dynamic_pointer_cast<VarcharField>(field2);
    //             return varcharField1->value!=varcharField2->value;
    //         }
    //     }else if(this->op==LT){
    //         if (field1->type==FieldType::INT){
    //             auto intField1 = dynamic_pointer_cast<IntField>(field1);
    //             auto intField2 = dynamic_pointer_cast<IntField>(field2);
    //             return intField1->value<intField2->value;
    //         }else if(field1->type==FieldType::FLOAT){
    //             auto floatField1 = dynamic_pointer_cast<FloatField>(field1);
    //             auto floatField2 = dynamic_pointer_cast<FloatField>(field2);
    //             return floatField1->value<floatField2->value;
    //         }else if(field1->type==FieldType::VARCHAR){
    //             auto varcharField1 = dynamic_pointer_cast<VarcharField>(field1);
    //             auto varcharField2 = dynamic_pointer_cast<VarcharField>(field2);
    //             return varcharField1->value<varcharField2->value;
    //         }
    //     }else if(this->op==GT){
    //         if (field1->type==FieldType::INT){
    //            auto intField1 = dynamic_pointer_cast<IntField>(field1);
    //             auto intField2 = dynamic_pointer_cast<IntField>(field2);
    //             return intField1->value>intField2->value;
    //         }else if(field1->type==FieldType::FLOAT){
    //             auto floatField1 = dynamic_pointer_cast<FloatField>(field1);
    //             auto floatField2 = dynamic_pointer_cast<FloatField>(field2);
    //             return floatField1->value>floatField2->value;
    //         }else if(field1->type==FieldType::VARCHAR){
    //             auto varcharField1 = dynamic_pointer_cast<VarcharField>(field1);
    //             auto varcharField2 = dynamic_pointer_cast<VarcharField>(field2);
    //             return varcharField1->value>varcharField2->value;
    //         }
    //     }else if(this->op==LE){
    //         if (field1->type==FieldType::INT){
    //             auto intField1 = dynamic_pointer_cast<IntField>(field1);
    //             auto intField2 = dynamic_pointer_cast<IntField>(field2);
    //             return intField1->value<=intField2->value;

    //         }else if(field1->type==FieldType::FLOAT){
    //             auto floatField1 = dynamic_pointer_cast<FloatField>(field1);
    //             auto floatField2 = dynamic_pointer_cast<FloatField>(field2);
    //             return floatField1->value<=floatField2->value;

    //         }else if(field1->type==FieldType::VARCHAR){
    //             auto varcharField1 = dynamic_pointer_cast<VarcharField>(field1);
    //             auto varcharField2 = dynamic_pointer_cast<VarcharField>(field2);
    //             return varcharField1->value<=varcharField2->value;
    //         }
    //     }else if(this->op==GE){
    //         if (field1->type==FieldType::INT){
    //             auto intField1 = dynamic_pointer_cast<IntField>(field1);
    //             auto intField2 = dynamic_pointer_cast<IntField>(field2);
    //             return intField1->value>=intField2->value;
    //         }else if(field1->type==FieldType::FLOAT){
    //             auto floatField1 = dynamic_pointer_cast<FloatField>(field1);
    //             auto floatField2 = dynamic_pointer_cast<FloatField>(field2);
    //             return floatField1->value>=floatField2->value;
    //         }else if(field1->type==FieldType::VARCHAR){
    //             auto  varcharField1 = dynamic_pointer_cast<VarcharField>(field1);
    //             auto varcharField2 = dynamic_pointer_cast<VarcharField>(field2);
    //             return varcharField1->value>=varcharField2->value;
    //         }
    //     }

    //     return false;
    // }
    
    bool satisfy(std::vector<std::string> tableNames,std::vector<std::shared_ptr<Field>> fields,std::vector<FieldMetaData> fieldMetaData){
        if (this->tableName!="" && tableNames[0]!=this->tableName){
            return false;
        }
        // 判断形如TB.a < 1 还是 TB.a == TB2.b
        if(this->FieldName2==""){
            //形如TB.a < 1
            //使用fieldMetaData找到对应的fieldId
            FieldID fieldId=-1;
            for (int i=0;i<fieldMetaData.size();i++){
                if (fieldMetaData[i].name==this->FieldName){
                    fieldId=fieldMetaData[i].id;
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
                if (fieldMetaData[i].name==this->FieldName){
                    fieldId1=fieldMetaData[i].id;
                    break;
                }
            }
            for (int i=0;i<fieldMetaData.size();i++){
                if (fieldMetaData[i].name==this->FieldName2){
                    fieldId2=fieldMetaData[i].id;
                    break;
                }
            }
            if (fieldId1==-1||fieldId2==-1){
                return false;
            }
            //判断fieldId对应的field是否满足条件
            auto field1 = fields[fieldId1];
            auto field2 = fields[fieldId2];
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


        }
    }
};