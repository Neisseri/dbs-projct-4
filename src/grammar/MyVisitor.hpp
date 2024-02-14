#pragma once

#include <cstring>
#include <iostream>
#include <vector>
#include <memory>
#include <fstream>

#include "SQLBaseVisitor.h"
#include "SQLParser.h"
#include "antlr4-runtime.h"
#include "system/dbms.hpp"
#include "../record/metadata.hpp"
#include "../record/constants.h"
#include "../constants.h"
#include "../record/bitmap.hpp"
#include "condition.h"
#include "index.h"

using namespace grammar;

class MyVisitor : public SQLBaseVisitor
{
public:
    DBMS &dbms;
    bool orderBy = false;
    bool desc = false;
    int limit = -1;
    int offset = -1;
    // 无参构造无法调用visit函数
    explicit MyVisitor(DBMS &dbms) : dbms(dbms) {}
    explicit MyVisitor(DBMS &dbms, bool orderBy, bool desc, int limit, int offset) : dbms(dbms), orderBy(orderBy), desc(desc), limit(limit), offset(offset) {}
    antlrcpp::Any visitProgram(SQLParser::ProgramContext *ctx) override;
    antlrcpp::Any visitStatement(SQLParser::StatementContext *ctx) override;
    antlrcpp::Any visitShow_dbs(SQLParser::Show_dbsContext *ctx) override;
    antlrcpp::Any visitCreate_db(SQLParser::Create_dbContext *ctx) override;
    antlrcpp::Any visitUse_db(SQLParser::Use_dbContext *ctx) override;
    antlrcpp::Any visitDrop_db(SQLParser::Drop_dbContext *ctx) override;
    antlrcpp::Any visitCreate_table(SQLParser::Create_tableContext *ctx) override;
    antlrcpp::Any visitShow_tables(SQLParser::Show_tablesContext *ctx) override;
    antlrcpp::Any visitDrop_table(SQLParser::Drop_tableContext *ctx) override;
    antlrcpp::Any visitDescribe_table(SQLParser::Describe_tableContext *ctx) override;
    antlrcpp::Any visitSelect_table(SQLParser::Select_tableContext *ctx) override;
    antlrcpp::Any visitInsert_into_table(SQLParser::Insert_into_tableContext *ctx) override;
    antlrcpp::Any visitWhere_and_clause(SQLParser::Where_and_clauseContext *ctx) override;
    antlrcpp::Any visitWhere_operator_expression(SQLParser::Where_operator_expressionContext *ctx) override;
    antlrcpp::Any visitUpdate_table(SQLParser::Update_tableContext *ctx) override;
    antlrcpp::Any visitSet_clause(SQLParser::Set_clauseContext *ctx) override;
    antlrcpp::Any visitDelete_from_table(SQLParser::Delete_from_tableContext *ctx) override;
    antlrcpp::Any visitLoad_table(SQLParser::Load_tableContext *ctx) override;
    antlrcpp::Any visitAlter_table_add_pk(SQLParser::Alter_table_add_pkContext *ctx) override;
    antlrcpp::Any visitAlter_table_drop_pk(SQLParser::Alter_table_drop_pkContext *ctx) override;
    antlrcpp::Any visitAlter_add_index(SQLParser::Alter_add_indexContext *ctx) override;
    antlrcpp::Any visitAlter_drop_index(SQLParser::Alter_drop_indexContext *ctx) override;
    antlrcpp::Any visitAlter_table_add_foreign_key(SQLParser::Alter_table_add_foreign_keyContext *ctx) override;
    antlrcpp::Any visitAlter_table_drop_foreign_key(SQLParser::Alter_table_drop_foreign_keyContext *ctx) override;
};

antlrcpp::Any MyVisitor::visitProgram(SQLParser::ProgramContext *ctx)
{
    return visitChildren(ctx);
}

antlrcpp::Any MyVisitor::visitStatement(SQLParser::StatementContext *ctx)
{
    return visitChildren(ctx);
}

antlrcpp::Any MyVisitor::visitShow_dbs(SQLParser::Show_dbsContext *ctx)
{
    std::cout << "DATABASES" << std::endl;
    for (auto &database : dbms.databaseList)
    {
        std::cout << database << std::endl;
    }
    return 0;
}

antlrcpp::Any MyVisitor::visitCreate_db(SQLParser::Create_dbContext *ctx)
{
    std::string databaseName = ctx->Identifier()->getText();
    if (std::find(dbms.databaseList.begin(), dbms.databaseList.end(), databaseName) != dbms.databaseList.end())
    {
        std::cerr << "ERROR: Database " << databaseName << " already exists\n";
        return 0;
    }
    dbms.createDatabase(databaseName);
    return 0;
}

antlrcpp::Any MyVisitor::visitUse_db(SQLParser::Use_dbContext *ctx)
{
    std::string databaseName = ctx->Identifier()->getText();
    if (std::find(dbms.databaseList.begin(), dbms.databaseList.end(), databaseName) == dbms.databaseList.end())
    {
        std::cerr << "ERROR: Database " << databaseName << " does not exist\n";
        return 0;
    }
    dbms.useDatabase(databaseName);
    return 0;
}

antlrcpp::Any MyVisitor::visitDrop_db(SQLParser::Drop_dbContext *ctx)
{
    std::string databaseName = ctx->Identifier()->getText();
    if (std::find(dbms.databaseList.begin(), dbms.databaseList.end(), databaseName) == dbms.databaseList.end())
    {
        std::cerr << "ERROR: Database " << databaseName << " does not exist\n";
        return 0;
    }
    dbms.deleteDatabase(databaseName);
    return 0;
}

antlrcpp::Any MyVisitor::visitCreate_table(SQLParser::Create_tableContext *ctx)
{
    visitChildren(ctx);
    std::string table_name = ctx->Identifier()->getText();
    auto field_ctx_list = ctx->field_list()->field();

    std::vector<std::shared_ptr<FieldMetaData>> field_metas;

    FieldID fid_counter = 0;

    // primary key name, primary key field names
    std::optional<std::pair<std::string, std::vector<std::string>>> primary_key;
    std::vector<ForeignKey> foreign_keys;

    for (const auto &field_ctx : field_ctx_list)
    {
        auto normal_field = dynamic_cast<SQLParser::Normal_fieldContext *>(field_ctx);
        if (normal_field)
        {
            FieldType field_type;
            std::string field_type_str = normal_field->type_()->children[0]->getText();
            // std::cout << field_type_str << std::endl;
            if (field_type_str == "INT")
            {
                field_type = FieldType::INT;
            }
            else if (field_type_str == "FLOAT")
            {
                field_type = FieldType::FLOAT;
            }
            else if (field_type_str == "VARCHAR")
            {
                field_type = FieldType::VARCHAR;
            }
            bool not_null;
            if (normal_field->Null())
            {
                not_null = true;
            }
            else
            {
                not_null = false;
            }
            std::string field_name = normal_field->Identifier()->getText();

            // max len for VARCHAR
            FieldSize max_len = -1;
            if (field_type == FieldType::VARCHAR)
            {
                max_len = std::stoi(normal_field->type_()->Integer()->getText());
            }

            auto field_meta = std::make_shared<FieldMetaData>(
                field_type,
                field_name,
                fid_counter, // field_id: [0, ...]
                not_null,
                max_len);
            fid_counter++;
            // std::cout << field_meta->toString() << std::endl;

            field_metas.push_back(field_meta);
            continue;
        }

        // primary key
        auto primary_key_field = dynamic_cast<SQLParser::Primary_key_fieldContext *>(field_ctx);
        if (primary_key_field)
        {
            if (primary_key)
            {
                std::cerr << "ERROR: Multiple primary keys\n";
                return 0;
            }
            std::string pk_name;
            if (primary_key_field->Identifier())
            {
                pk_name = primary_key_field->Identifier()->getText();
            }
            std::vector<std::string> field_names;
            for (const auto &field_name : primary_key_field->identifiers()->Identifier())
            {
                field_names.emplace_back(field_name->getText());
            }
            primary_key = std::make_pair(pk_name, field_names);
            continue;
        }

        // foreign key
        auto foreign_key_field = dynamic_cast<SQLParser::Foreign_key_fieldContext *>(field_ctx);
        if (foreign_key_field)
        {
            std::string foreign_key_name;
            std::string reference_table_name;
            if (foreign_key_field->Identifier().size() == 1)
            {
                foreign_key_name = "";
                reference_table_name = foreign_key_field->Identifier(0)->getText();
            }
            else
            {
                foreign_key_name = foreign_key_field->Identifier(0)->getText();
                reference_table_name = foreign_key_field->Identifier(1)->getText();
            }
            std::vector<std::string> field_names;
            std::vector<std::string> reference_field_names;
            for (const auto &field_name : foreign_key_field->identifiers(0)->Identifier())
            {
                field_names.emplace_back(field_name->getText());
            }
            for (const auto &reference_field_name : foreign_key_field->identifiers(1)->Identifier())
            {
                reference_field_names.emplace_back(reference_field_name->getText());
            }
            foreign_keys.emplace_back(foreign_key_name, reference_table_name, field_names, reference_field_names);
            continue;
        }
    }
    dbms.createTable(table_name, field_metas, primary_key, foreign_keys);
    return 0;
}

antlrcpp::Any MyVisitor::visitShow_tables(SQLParser::Show_tablesContext *ctx)
{
    if (dbms.currentDatabase.first == false)
    {
        std::cerr << "No database selected\n";
        return 0;
    }
    std::cout << "TABLES" << std::endl;
    // traverse all files in current database
    std::string database_path = dbms.base_path + dbms.currentDatabase.second;
    for (const auto &entry : std::filesystem::directory_iterator(database_path))
    {
        std::string table_name = entry.path().filename().string();
        if (table_name.length() > 7)
        {
            if (table_name.substr(table_name.size() - 7, 7) == ".bitmap")
            {
                continue;
            }
        }
        std::cout << table_name << std::endl;
    }
    return 0;
}

antlrcpp::Any MyVisitor::visitDrop_table(SQLParser::Drop_tableContext *ctx)
{
    if (dbms.currentDatabase.first == false)
    {
        std::cerr << "No database selected\n";
        return 0;
    }
    std::string table_name = ctx->Identifier()->getText();
    std::string table_path = dbms.base_path + dbms.currentDatabase.second + "/" + table_name;
    if (!std::filesystem::exists(table_path))
    {
        std::cerr << "ERROR: Table " << table_name << " does not exist\n";
        return 0;
    }
    std::filesystem::remove(table_path);
    return 0;
}

antlrcpp::Any MyVisitor::visitDescribe_table(SQLParser::Describe_tableContext *ctx)
{
    if (dbms.currentDatabase.first == false)
    {
        std::cerr << "No database selected\n";
        return 0;
    }
    std::string table_name = ctx->Identifier()->getText();
    std::string table_path = dbms.base_path + dbms.currentDatabase.second + "/" + table_name;
    if (!std::filesystem::exists(table_path))
    {
        std::cerr << "ERROR: Table " << table_name << " does not exist\n";
        return 0;
    }
    std::cout << "Field,Type,Null,Default" << std::endl;
    int fileID;
    dbms.fm->openFile(table_path.c_str(), fileID);
    TableMetaData table_meta;
    table_meta.load(fileID, dbms.bpm);
    for (const auto &field_meta : table_meta.fields)
    {
        std::cout << field_meta->name << ",";
        if (field_meta->type == FieldType::VARCHAR)
        {
            std::cout << "VARCHAR(" << field_meta->size << "),";
        }
        else
        {
            std::cout << FieldType2String(field_meta->type) << ",";
        }
        if (field_meta->not_null)
        {
            std::cout << "NO,";
        }
        else
        {
            std::cout << "YES,";
        }
        if (field_meta->has_default)
        {
            // TODO:
        }
        else
        {
            std::cout << "NULL";
        }

        std::cout << std::endl;
    }

    // primary key
    std::cout << std::endl;
    if (table_meta.primary_key)
    {
        std::cout << "PRIMARY KEY (";
        for (const auto &field_name : table_meta.primary_key->second)
        {
            std::cout << field_name;
            if (field_name != table_meta.primary_key->second.back())
            {
                std::cout << ", ";
            }
        }
        std::cout << ");" << std::endl;
    }

    // foreign key
    for (const auto &foreign_key : table_meta.foreign_keys)
    {
        auto [foreign_key_name, reference_table_name, field_names, reference_field_names] = foreign_key;
        std::cout << "FOREIGN KEY (";
        for (const auto &field_name : field_names)
        {
            std::cout << field_name;
            if (field_name != field_names.back())
            {
                std::cout << ", ";
            }
        }
        std::cout << ") REFERENCES " << reference_table_name << "(";
        for (const auto &reference_field_name : reference_field_names)
        {
            std::cout << reference_field_name;
            if (reference_field_name != reference_field_names.back())
            {
                std::cout << ", ";
            }
        }
        std::cout << ");" << std::endl;
    }

    // index
    for (const auto &index : table_meta.indexs)
    {
        std::cout << "INDEX  (";
        std::cout << index.second;
        std::cout << ");" << std::endl;
    }

    dbms.fm->closeFile(fileID);

    return 0;
}

antlrcpp::Any MyVisitor::visitSelect_table(SQLParser::Select_tableContext *ctx)
{

    std::vector<std::string> selected_tables; // from后的所有的表名

    for (const auto &table_name : ctx->identifiers()->Identifier())
    {
        selected_tables.push_back(table_name->getText());
    }

    // 1.先visit where_and_clause，得到where_operator_conditions,where_like_conditions,where_in_conditions,whereNullConditions,whereNotNullConditions
    auto where_and_clause = ctx->where_and_clause();

    std::vector<std::shared_ptr<WhereOperatorCondtion>> where_operator_conditions;
    std::vector<std::shared_ptr<WhereLikeCondition>> where_like_conditions;
    std::vector<std::shared_ptr<WhereInCondition>> where_in_conditions;
    std::vector<std::shared_ptr<WhereIsNullCondition>> whereNullConditions;
    std::vector<std::shared_ptr<WhereIsNotNullCondition>> whereNotNullConditions;

    if (where_and_clause)
    {
        // where_operator_conditions = (ctx->where_and_clause())->accept(this).as<std::vector<std::shared_ptr<WhereOperatorCondtion>>>();
        std::tie(where_operator_conditions, where_like_conditions, where_in_conditions, whereNullConditions, whereNotNullConditions) = where_and_clause->accept(this).as<std::tuple<std::vector<std::shared_ptr<WhereOperatorCondtion>>, std::vector<std::shared_ptr<WhereLikeCondition>>, std::vector<std::shared_ptr<WhereInCondition>>, std::vector<std::shared_ptr<WhereIsNullCondition>>, std::vector<std::shared_ptr<WhereIsNotNullCondition>>>>();
    }

    if (selected_tables.size() == 2)
    {
        // 通过selected_tables,得到拼接好的fieldMetaData和tableName
        std::vector<FieldMetaData> fieldMetaData; // 拼接好的fieldMetaData a b x y
        std::vector<std::string> tableName;       // 拼接好的tableName TA TA TB TB
        std::string table1_path = dbms.base_path + dbms.currentDatabase.second + "/" + selected_tables[0];
        std::string table2_path = dbms.base_path + dbms.currentDatabase.second + "/" + selected_tables[1];
        TableMetaData table1_meta;
        TableMetaData table2_meta;
        int fileID1;
        int fileID2;
        dbms.fm->openFile(table1_path.c_str(), fileID1);
        dbms.fm->openFile(table2_path.c_str(), fileID2);
        table1_meta.load(fileID1, dbms.bpm);
        table2_meta.load(fileID2, dbms.bpm);
        for (const auto &field_meta : table1_meta.fields)
        {
            fieldMetaData.push_back(*(field_meta.get()));
            tableName.push_back(selected_tables[0]);
        }
        for (const auto &field_meta : table2_meta.fields)
        {
            fieldMetaData.push_back(*(field_meta.get()));
            tableName.push_back(selected_tables[1]);
        }

        // visit 所有selector, 得到要返回的列名信息
        // 提取成SUPPLIER.S_NAME或者S_NAME或者*的形式，注意只有在没有group by时才会用到
        std::vector<std::string> selected_table_names;
        std::vector<std::string> selected_field_names;
        if (ctx->selectors()->children[0]->getText() == "*")
        {
            selected_field_names.push_back("*");
        }
        else
        {
            for (const auto &selector : ctx->selectors()->selector())
            {
                if (selector->column())
                {
                    std::string tableName = selector->column()->Identifier(0)->getText();
                    std::string fieldName = selector->column()->Identifier(1)->getText();
                    selected_table_names.push_back(tableName);
                    selected_field_names.push_back(fieldName);
                }
            }
        }

        std::vector<int> selected_field_index;
        for (int i = 0; i < selected_field_names.size(); i++)
        {
            std::string selected_table_name = selected_table_names[i];
            for (int j = 0; j < tableName.size(); j++)
            {
                std::string field_name = fieldMetaData[j].name;
                if (tableName[j] == selected_table_name && field_name == selected_field_names[i])
                {
                    selected_field_index.push_back(j);
                    break;
                }
            }
        }

        // print a,b,c,a,b,c
        for (int i = 0; i < selected_field_names.size(); i++)
        {
            std::cout << selected_field_names[i];
            if (i != selected_field_names.size() - 1)
            {
                std::cout << ",";
            }
        }
        std::cout << std::endl;

        // load bitmap
        Bitmap bitmap1;
        Bitmap bitmap2;
        std::string bitmap_path1 = dbms.base_path + dbms.currentDatabase.second + "/" + selected_tables[0] + ".bitmap";
        std::string bitmap_path2 = dbms.base_path + dbms.currentDatabase.second + "/" + selected_tables[1] + ".bitmap";
        bitmap1.load(bitmap_path1);
        bitmap2.load(bitmap_path2);
        int record_num1 = bitmap1.record_num;
        int record_num2 = bitmap2.record_num;
        int record_size1 = table1_meta.record_size();
        int record_size2 = table2_meta.record_size();
        int record_num_per_page1 = bitmap1.record_num_per_page;
        int record_num_per_page2 = bitmap2.record_num_per_page;

        // record 1 and 2 initialization
        Record record1;
        Record record2;
        for (auto field1 : table1_meta.fields)
        {
            if (field1->type == FieldType::INT)
            {
                record1.fields.push_back(std::make_shared<IntField>(0));
            }
            else if (field1->type == FieldType::FLOAT)
            {
                record1.fields.push_back(std::make_shared<FloatField>(0));
            }
            else if (field1->type == FieldType::VARCHAR)
            {
                record1.fields.push_back(std::make_shared<VarcharField>("", false, field1->size));
            }
        }
        for (auto field2 : table2_meta.fields)
        {
            if (field2->type == FieldType::INT)
            {
                record2.fields.push_back(std::make_shared<IntField>(0));
            }
            else if (field2->type == FieldType::FLOAT)
            {
                record2.fields.push_back(std::make_shared<FloatField>(0));
            }
            else if (field2->type == FieldType::VARCHAR)
            {
                record2.fields.push_back(std::make_shared<VarcharField>("", false, field2->size));
            }
        }

        // initialize join record
        Record join_record;
        for (auto &join_field_meta : fieldMetaData)
        {
            if (join_field_meta.type == FieldType::INT)
            {
                join_record.fields.push_back(std::make_shared<IntField>(0));
            }
            else if (join_field_meta.type == FieldType::FLOAT)
            {
                join_record.fields.push_back(std::make_shared<FloatField>(0));
            }
            else if (join_field_meta.type == FieldType::VARCHAR)
            {
                join_record.fields.push_back(std::make_shared<VarcharField>("", false, join_field_meta.size));
            }
        }

        int addr1 = 1;
        int addr2 = 1;
        // 2-dimension traverse
        for (int i = 1; i <= record_num1; i++)
        {
            int bit1 = (bitmap1.bitmap[(addr1 - 1) / 32] >> (31 - (addr1 - 1) % 32)) & 1;
            while (bit1 != 1)
            {
                addr1++;
                bit1 = (bitmap1.bitmap[(addr1 - 1) / 32] >> (31 - (addr1 - 1) % 32)) & 1;
            }
            int pageID1 = (addr1 - 1) / record_num_per_page1 + 2; // [2, ...]
            int slot1 = (addr1 - 1) % record_num_per_page1 + 1;   // [1, 341] -> 1
            record1.load(fileID1, dbms.bpm, pageID1, (slot1 - 1) * record_size1);
            // std::cerr << "Record1: " << record1.toString();

            addr2 = 1;
            for (int j = 1; j <= record_num2; j++)
            {
                int bit2 = (bitmap2.bitmap[(addr2 - 1) / 32] >> (31 - (addr2 - 1) % 32)) & 1;
                while (bit2 != 1)
                {
                    addr2++;
                    bit2 = (bitmap2.bitmap[(addr2 - 1) / 32] >> (31 - (addr2 - 1) % 32)) & 1;
                }
                int pageID2 = (addr2 - 1) / record_num_per_page2 + 2; // [2, ...]
                int slot2 = (addr2 - 1) % record_num_per_page2 + 1;   // [1, 341] -> 1
                record2.load(fileID2, dbms.bpm, pageID2, (slot2 - 1) * record_size2);
                // std::cerr << "Record2: " << record2.toString();

                // construct join Record
                for (int k = 0; k < record1.fields.size(); k++)
                {
                    join_record.fields[k] = record1.fields[k];
                }
                for (int k = 0; k < record2.fields.size(); k++)
                {
                    join_record.fields[k + record1.fields.size()] = record2.fields[k];
                }
                // std::cerr << "Join Table: " << join_record.toString();
                bool flag = true;
                for (auto where_operator_condition : where_operator_conditions)
                {
                    if (!where_operator_condition->satisfy_join(tableName, join_record.fields, fieldMetaData))
                    {
                        flag = false;
                        break;
                    }
                }
                if (flag)
                {
                    // std::cout << join_record.toString();
                    for (int k = 0; k < selected_field_index.size(); k++)
                    {
                        int index = selected_field_index[k];
                        std::cout << join_record.fields[index]->toString();
                        if (k != selected_field_index.size() - 1)
                        {
                            std::cout << ",";
                        }
                    }
                    std::cout << std::endl;
                }

                addr2++;
            }
            addr1++;
        }

        dbms.fm->closeFile(fileID1);
        dbms.fm->closeFile(fileID2);

        return 0;
    }

    if (ctx->selectors()->children[0]->getText() == "*" && where_operator_conditions.size() == 0)
    { // select all columns and no where
        for (const auto &table_name : selected_tables)
        {
            std::string table_path = dbms.base_path + dbms.currentDatabase.second + "/" + table_name;
            if (!std::filesystem::exists(table_path))
            {
                std::cerr << "ERROR: Table " << table_name << " does not exist\n";
                return 0;
            }
            int fileID;
            dbms.fm->openFile(table_path.c_str(), fileID);
            TableMetaData table_meta;
            table_meta.load(fileID, dbms.bpm);
            for (const auto &field_meta : table_meta.fields)
            {
                std::cout << field_meta->name;
                if (field_meta != table_meta.fields.back())
                {
                    std::cout << ",";
                }
            }
            std::cout << std::endl;

            Bitmap bitmap;
            std::string bitmap_path = dbms.base_path + dbms.currentDatabase.second + "/" + table_name + ".bitmap";
            bitmap.load(bitmap_path);

            if (this->orderBy)
            {
                bitmap.printRecordOrderBy(fileID,dbms.bpm,table_meta.record_size(),table_meta.fields,table_meta.name,ctx->column()[0]->getText(),this->desc,limit,offset);
                
            }
            else
            {
                bitmap.printRecord(fileID, dbms.bpm, table_meta.record_size(), table_meta.fields);
            }

            dbms.fm->closeFile(fileID);
        }
        return 0;
    }

    if (ctx->selectors()->children[0]->getText() == "*" && where_operator_conditions.size() != 0)
    { // select * from ... where
        for (const auto &table_name : selected_tables)
        {
            std::string table_path = dbms.base_path + dbms.currentDatabase.second + "/" + table_name;
            if (!std::filesystem::exists(table_path))
            {
                std::cerr << "ERROR: Table " << table_name << " does not exist\n";
                return 0;
            }
            int fileID;
            dbms.fm->openFile(table_path.c_str(), fileID);
            TableMetaData table_meta;
            table_meta.load(fileID, dbms.bpm);
            for (const auto &field_meta : table_meta.fields)
            {
                std::cout << field_meta->name;
                if (field_meta != table_meta.fields.back())
                {
                    std::cout << ",";
                }
            }
            std::cout << std::endl;

            Bitmap bitmap;
            std::string bitmap_path = dbms.base_path + dbms.currentDatabase.second + "/" + table_name + ".bitmap";
            bitmap.load(bitmap_path);
            bitmap.printRecordWhere(fileID, dbms.bpm, table_meta.record_size(), table_meta.fields, where_operator_conditions, table_name);

            dbms.fm->closeFile(fileID);
        }
        return 0;
    }

    if (ctx->selectors()->children[0]->getText() != "*")
    { // select a, b, c from ... where
        std::vector<std::string> selected_fields;
        for (const auto &selector : ctx->selectors()->selector())
        {
            // std::cerr << "selector = " << selector->getText() << std::endl;
            selected_fields.push_back(selector->getText());
        }

        for (const auto &table_name : selected_tables)
        {
            std::string table_path = dbms.base_path + dbms.currentDatabase.second + "/" + table_name;
            if (!std::filesystem::exists(table_path))
            {
                std::cerr << "ERROR: Table " << table_name << " does not exist\n";
                return 0;
            }

            int fileID;
            dbms.fm->openFile(table_path.c_str(), fileID);
            TableMetaData table_meta;
            table_meta.load(fileID, dbms.bpm);

            for (auto &selected_field : selected_fields)
            {
                std::cout << selected_field;
                if (selected_field != selected_fields.back())
                {
                    std::cout << ",";
                }
            }
            std::cout << std::endl;

            Bitmap bitmap;
            std::string bitmap_path = dbms.base_path + dbms.currentDatabase.second + "/" + table_name + ".bitmap";
            bitmap.load(bitmap_path);
            // 考虑order by
            // 如果只有where operator condition, 那么判断是否需要用索引
            if (where_operator_conditions.size() != 0 && where_like_conditions.size() == 0 && where_in_conditions.size() == 0 && whereNullConditions.size() == 0 && whereNotNullConditions.size() == 0)
            {
                if (shouldUseIndex(where_operator_conditions))
                {
                    // 计算区间
                    int lower_bound, upper_bound;
                    calInterval(where_operator_conditions, lower_bound, upper_bound);
                    // std::cerr<<"lower_bound = "<<lower_bound<<" upper_bound = "<<upper_bound<<std::endl;
                    // TODO 如果区间为空，直接返回
                    if (lower_bound > upper_bound)
                    {
                        // std::cerr << "lower_bound > upper_bound" << std::endl;
                        return 0;
                    }
                    // std::cerr << "lower_bound = " << lower_bound << " upper_bound = " << upper_bound << std::endl;
                    // 使用索引文件构造得到slot_id page_id
                    std::vector<int> slot_ids;
                    std::vector<int> page_ids;
                    // 先打开索引文件
                    std::string index_file_path = table_meta.name + "." + where_operator_conditions[0]->FieldName + ".index";
                    // std::cerr << "index_file_path = " << index_file_path << std::endl;
                    FILE *file = fopen(index_file_path.c_str(), "r+");
                    if (file == NULL)
                    {
                        std::cerr << "fopen error" << std::endl;
                    }
                    // 取得索引 page id slot id
                    int page_id, slot_id;
                    int key = max(lower_bound, 0);
                    int position = getPosition(key, where_operator_conditions[0]->FieldName);
                    // std::cerr<<"position = "<<position<<std::endl;
                    // std::cerr<<"key = "<<key<<std::endl;
                    while (1)
                    {
                        getIndex(file, position, key, page_id, slot_id);
                        // std::cerr<<"key = "<<key<<" page_id = "<<page_id<<" slot_id = "<<slot_id<<std::endl;
                        if (position > 20 * upper_bound)
                        {
                            break;
                        }
                        if (slot_id == -1)
                        {
                        }
                        else
                        {
                            page_ids.push_back(page_id);
                            slot_ids.push_back(slot_id);
                            // std::cerr<<"key = "<<key<<" page_id = "<<page_id<<" slot_id = "<<slot_id<<std::endl;
                        }

                        position++;
                    }
                    // 根据是否有order by调用相应的bitmap print函数
                    if (this->orderBy)
                    {
                        // std::cerr<<"order by"<<ctx->column()[0]->getText()<<std::endl;
                        Bitmap bitmap;
                        std::string bitmap_path = dbms.base_path + dbms.currentDatabase.second + "/" + table_name + ".bitmap";
                        // std::cerr<<"bitmap_path = "<<bitmap_path<<std::endl;
                        // std::cerr<<"limit"<<limit<<std::endl;
                        // std::cerr<<"offset"<<offset<<std::endl;
                        bitmap.load(bitmap_path);
                        bitmap.printRecordPageSlotOrderBy(fileID, dbms.bpm, table_meta.fields, table_meta.name, table_meta.record_size(), page_ids, slot_ids, ctx->column()[0]->getText(), desc, limit, offset, selected_fields);
                        dbms.fm->closeFile(fileID);
                        return 0;
                    }
                    else
                    {
                        Bitmap bitmap;
                        std::string bitmap_path = dbms.base_path + dbms.currentDatabase.second + "/" + table_name + ".bitmap";
                        bitmap.load(bitmap_path);
                        bitmap.printRecordPageSlot(fileID, dbms.bpm, table_meta.fields, table_meta.name, table_meta.record_size(), page_ids, slot_ids);
                        dbms.fm->closeFile(fileID);
                        return 0;
                    }
                }
            }

            bitmap.printRecordWhereSelector(
                fileID, dbms.bpm, table_meta.record_size(), table_meta.fields, where_operator_conditions, table_name, selected_fields);
        }
    }

    return 0;
}

antlrcpp::Any MyVisitor::visitInsert_into_table(SQLParser::Insert_into_tableContext *ctx)
{
    visitChildren(ctx);
    auto table_name = ctx->Identifier()->getText();
    auto insertions = ctx->value_lists()->value_list();

    int fileID;
    std::string table_path = dbms.base_path + dbms.currentDatabase.second + "/" + table_name;
    dbms.fm->openFile(table_path.c_str(), fileID);

    TableMetaData table_meta;
    table_meta.load(fileID, dbms.bpm);

    Bitmap bitmap;
    std::string bitmap_path = dbms.base_path + dbms.currentDatabase.second + "/" + table_name + ".bitmap";
    bitmap.load(bitmap_path);

    int record_size = table_meta.record_size();

    for (int i = 0; i < insertions.size(); i++)
    {
        auto value_list = insertions[i]->value();
        std::vector<std::shared_ptr<Field>> fields;

        int cnt = 0;
        for (auto field_meta : table_meta.fields)
        {
            if (field_meta->type == FieldType::INT)
            {
                auto value = std::stoi(value_list[cnt]->getText());
                fields.push_back(std::make_shared<IntField>(value));
            }
            else if (field_meta->type == FieldType::FLOAT)
            {
                double value = std::stod(value_list[cnt]->getText());
                fields.push_back(std::make_shared<FloatField>(value));
            }
            else if (field_meta->type == FieldType::VARCHAR)
            {
                auto value = value_list[cnt]->getText();
                if (value.size() > 2)
                {
                    value = value.substr(1, value.size() - 2);
                }
                fields.push_back(std::make_shared<VarcharField>(value, false, field_meta->size));
            }
            cnt++;
        }
        Record record(fields);

        // check if primary key exists
        int pk_num = table_meta.primary_key->second.size();
        if (pk_num == 1)
        {
            auto pk_field_name = table_meta.primary_key->second[0];
            std::string pk_value;
            for (int i = 0; i < table_meta.fields.size(); i++)
            {
                if (table_meta.fields[i]->name == pk_field_name)
                {
                    pk_value = fields[i]->toString();
                }
            }

            // traverse all records to check if pk_value exists
            int addr = 1;
            int record_num = bitmap.record_num;
            int record_size = table_meta.record_size();
            int record_num_per_page = bitmap.record_num_per_page;
            Record record_read;
            for (auto field_meta : table_meta.fields)
            {
                if (field_meta->type == FieldType::INT)
                {
                    record_read.fields.push_back(std::make_shared<IntField>(0));
                }
                else if (field_meta->type == FieldType::FLOAT)
                {
                    record_read.fields.push_back(std::make_shared<FloatField>(0));
                }
                else if (field_meta->type == FieldType::VARCHAR)
                {
                    record_read.fields.push_back(std::make_shared<VarcharField>("", false, field_meta->size));
                }
            }
            for (int i = 1; i <= record_num; i++)
            {
                int bit = (bitmap.bitmap[(addr - 1) / 32] >> (31 - (addr - 1) % 32)) & 1;
                while (bit != 1)
                {
                    addr++;
                    bit = (bitmap.bitmap[(addr - 1) / 32] >> (31 - (addr - 1) % 32)) & 1;
                }
                int pageID = (addr - 1) / record_num_per_page + 2; // [2, ...]
                int slot = (addr - 1) % record_num_per_page + 1;
                record_read.load(fileID, dbms.bpm, pageID, (slot - 1) * record_size);
                std::string pk_value_read;
                for (int j = 0; j < table_meta.fields.size(); j++)
                {
                    if (table_meta.fields[j]->name == pk_field_name)
                    {
                        pk_value_read = record_read.fields[j]->toString();
                    }
                }
                if (pk_value_read == pk_value)
                {
                    std::cout << "!ERROR\nduplicate" << std::endl;
                    return 0;
                }
                addr++;
            }
        }
        else if (pk_num == 2)
        {
            auto pk_field_name1 = table_meta.primary_key->second[0];
            auto pk_field_name2 = table_meta.primary_key->second[1];
            std::string pk_value1;
            std::string pk_value2;
            for (int i = 0; i < table_meta.fields.size(); i++)
            {
                if (table_meta.fields[i]->name == pk_field_name1)
                {
                    pk_value1 = fields[i]->toString();
                }
                else if (table_meta.fields[i]->name == pk_field_name2)
                {
                    pk_value2 = fields[i]->toString();
                }
            }

            // traverse all records to check if pk_value exists
            int addr = 1;
            int record_num = bitmap.record_num;
            int record_size = table_meta.record_size();
            int record_num_per_page = bitmap.record_num_per_page;
            Record record_read;
            for (auto field_meta : table_meta.fields)
            {
                if (field_meta->type == FieldType::INT)
                {
                    record_read.fields.push_back(std::make_shared<IntField>(0));
                }
                else if (field_meta->type == FieldType::FLOAT)
                {
                    record_read.fields.push_back(std::make_shared<FloatField>(0));
                }
                else if (field_meta->type == FieldType::VARCHAR)
                {
                    record_read.fields.push_back(std::make_shared<VarcharField>("", false, field_meta->size));
                }
            }
            for (int i = 1; i <= record_num; i++)
            {
                int bit = (bitmap.bitmap[(addr - 1) / 32] >> (31 - (addr - 1) % 32)) & 1;
                while (bit != 1)
                {
                    addr++;
                    bit = (bitmap.bitmap[(addr - 1) / 32] >> (31 - (addr - 1) % 32)) & 1;
                }
                int pageID = (addr - 1) / record_num_per_page + 2; // [2, ...]
                int slot = (addr - 1) % record_num_per_page + 1;
                record_read.load(fileID, dbms.bpm, pageID, (slot - 1) * record_size);
                std::string pk_value_read1;
                std::string pk_value_read2;
                for (int j = 0; j < table_meta.fields.size(); j++)
                {
                    if (table_meta.fields[j]->name == pk_field_name1)
                    {
                        pk_value_read1 = record_read.fields[j]->toString();
                    }
                    if (table_meta.fields[j]->name == pk_field_name2)
                    {
                        pk_value_read2 = record_read.fields[j]->toString();
                    }
                }
                if (pk_value_read1 == pk_value1 && pk_value_read2 == pk_value2)
                {
                    std::cout << "!ERROR\nduplicate" << std::endl;
                    return 0;
                }
                addr++;
            }
        }

        // 复合外键的检查
        auto foreign_keys = table_meta.foreign_keys;
        if (foreign_keys.size() == 1 && value_list.size() == 4) {
            auto [foreign_key_name, reference_table_name, field_names, reference_field_names] = foreign_keys[0];
            if (field_names.size() == 2 && reference_field_names.size() == 2) {
                
                std::pair<std::string, std::string> fk_values;
                for (int i = 0; i < table_meta.fields.size(); i++) {
                    if (table_meta.fields[i]->name == field_names[0]) {
                        fk_values.first = value_list[i]->getText();
                    }
                    if (table_meta.fields[i]->name == field_names[1]) {
                        fk_values.second = value_list[i]->getText();
                    }
                }

                // std::cerr << "本张表的两个外键值等于 fk_values = " << fk_values.first << " " << fk_values.second << std::endl;

                // 检查对应的主表中有没有这个外键值
                std::string foreign_table_path = dbms.base_path + dbms.currentDatabase.second + "/" + reference_table_name;
                TableMetaData foreign_table_meta;
                int foreign_fileID;
                dbms.fm->openFile(foreign_table_path.c_str(), foreign_fileID);
                foreign_table_meta.load(foreign_fileID, dbms.bpm);
                Bitmap foreign_bitmap;
                std::string foreign_bitmap_path = dbms.base_path + dbms.currentDatabase.second + "/" + reference_table_name + ".bitmap";
                foreign_bitmap.load(foreign_bitmap_path);

                bool is_foreign_value_exist = false;
                Record record_read;
                // traverse all records to check if fk_value exists
                int addr = 1;
                int record_num = foreign_bitmap.record_num;
                int record_size = foreign_table_meta.record_size();
                int record_num_per_page = foreign_bitmap.record_num_per_page;
                for (auto field_meta : table_meta.fields) {
                    if (field_meta->type == FieldType::INT) {
                        record_read.fields.push_back(std::make_shared<IntField>(0));
                    } else if (field_meta->type == FieldType::FLOAT) {
                        record_read.fields.push_back(std::make_shared<FloatField>(0));
                    } else if (field_meta->type == FieldType::VARCHAR) {
                        record_read.fields.push_back(std::make_shared<VarcharField>("", false, field_meta->size));
                    }
                }

                for (int i = 1; i <= record_num; i++) {
                    int bit = (foreign_bitmap.bitmap[(addr - 1) / 32] >> (31 - (addr - 1) % 32)) & 1;
                    while (bit != 1) {
                        addr++;
                        bit = (foreign_bitmap.bitmap[(addr - 1) / 32] >> (31 - (addr - 1) % 32)) & 1;
                    }
                    int pageID = (addr - 1) / record_num_per_page + 2; // [2, ...]
                    int slot = (addr - 1) % record_num_per_page + 1;
                    record_read.load(foreign_fileID, dbms.bpm, pageID, (slot - 1) * record_size);
                    std::string fk_value_read1;
                    std::string fk_value_read2;
                    for (int j = 0; j < foreign_table_meta.fields.size(); j++) {
                        if (foreign_table_meta.fields[j]->name == reference_field_names[0]) {
                            fk_value_read1 = record_read.fields[j]->toString();
                        }
                        if (foreign_table_meta.fields[j]->name == reference_field_names[1]) {
                            fk_value_read2 = record_read.fields[j]->toString();
                        }
                    }
                    if (fk_value_read1 == fk_values.first && fk_value_read2 == fk_values.second) {
                        is_foreign_value_exist = true;
                        break;
                    }
                    addr++;
                }
                if (!is_foreign_value_exist) {
                    std::cout << "!ERROR\nforeign" << std::endl;
                    return 0;
                }
            }
        }


        // check if foreign key exists
        // foreign_key_name, reference_table_name, field_names, reference_field_names
        // typedef std::tuple<std::string, std::string, std::vector<std::string>, std::vector<std::string>> ForeignKey;
        std::string foreign_key_name;
        std::string reference_table_name;
        std::vector<std::string> field_names;
        std::vector<std::string> reference_field_names;
        if (table_meta.foreign_keys.size() == 1)
        {
            const auto &[foreign_key_name, reference_table_name, field_names, reference_field_names] = table_meta.foreign_keys[0];
            // if (field_names.size() == 1) {
            //     std::cerr << "Reference Table = " << reference_table_name << std::endl;
            //     for (auto field_name : field_names) {
            //         std::cerr << "Field = " << field_name << std::endl;
            //     }
            //     for (auto reference_field_name : reference_field_names) {
            //         std::cerr << "Reference Field = " << reference_field_name << std::endl;
            //     }
            // }
            std::string foreign_value;
            bool is_foreign_key = false;
            for (int i = 0; i < table_meta.fields.size(); i++)
            {
                if (table_meta.fields[i]->name == field_names[0])
                {
                    foreign_value = value_list[i]->getText();
                    is_foreign_key = true;
                    break;
                }
            }
            if (is_foreign_key)
            {
                std::string foreign_table_path = dbms.base_path + dbms.currentDatabase.second + "/" + reference_table_name;
                TableMetaData foreign_table_meta;
                int foreign_fileID;
                dbms.fm->openFile(foreign_table_path.c_str(), foreign_fileID);
                foreign_table_meta.load(foreign_fileID, dbms.bpm);
                Bitmap foreign_bitmap;
                std::string foreign_bitmap_path = dbms.base_path + dbms.currentDatabase.second + "/" + reference_table_name + ".bitmap";
                foreign_bitmap.load(foreign_bitmap_path);

                bool is_foreign_value_exist = false;
                Record record_read;
                // traverse all records to check if fk_value exists
                int addr = 1;
                int record_num = foreign_bitmap.record_num;
                int record_size = foreign_table_meta.record_size();
                int record_num_per_page = foreign_bitmap.record_num_per_page;
                for (auto field_meta : table_meta.fields)
                {
                    if (field_meta->type == FieldType::INT)
                    {
                        record_read.fields.push_back(std::make_shared<IntField>(0));
                    }
                    else if (field_meta->type == FieldType::FLOAT)
                    {
                        record_read.fields.push_back(std::make_shared<FloatField>(0));
                    }
                    else if (field_meta->type == FieldType::VARCHAR)
                    {
                        record_read.fields.push_back(std::make_shared<VarcharField>("", false, field_meta->size));
                    }
                }
                for (int i = 1; i <= record_num; i++)
                {
                    int bit = (foreign_bitmap.bitmap[(addr - 1) / 32] >> (31 - (addr - 1) % 32)) & 1;
                    while (bit != 1)
                    {
                        addr++;
                        bit = (foreign_bitmap.bitmap[(addr - 1) / 32] >> (31 - (addr - 1) % 32)) & 1;
                    }
                    int pageID = (addr - 1) / record_num_per_page + 2; // [2, ...]
                    int slot = (addr - 1) % record_num_per_page + 1;
                    record_read.load(foreign_fileID, dbms.bpm, pageID, (slot - 1) * record_size);
                    std::string fk_value_read;
                    for (int j = 0; j < foreign_table_meta.fields.size(); j++)
                    {
                        if (foreign_table_meta.fields[j]->name == reference_field_names[0])
                        {
                            fk_value_read = record_read.fields[j]->toString();
                        }
                    }
                    if (fk_value_read == foreign_value)
                    {
                        is_foreign_value_exist = true;
                        break;
                    }
                    addr++;
                }
                if (!is_foreign_value_exist)
                {
                    std::cout << "!ERROR\nforeign" << std::endl;
                    return 0;
                }
                dbms.fm->closeFile(foreign_fileID);
            }
        }
        // check foreign key end

        bitmap.addRecord();
        // max_addr: [1, ...]
        int pageID = (bitmap.max_addr - 1) / bitmap.record_num_per_page + 2; // [2, ...]
        int slot = (bitmap.max_addr - 1) % bitmap.record_num_per_page + 1;
        record.store(fileID, dbms.bpm, pageID, (slot - 1) * record_size);
    }
    bitmap.store(bitmap_path);
    // std::cerr << std::bitset<4 * 8>(bitmap.bitmap[0]) << std::endl;

    dbms.fm->closeFile(fileID);

    std::cout << "rows" << std::endl;
    std::cout << insertions.size() << std::endl;

    return 0;
}

antlrcpp::Any MyVisitor::visitWhere_operator_expression(SQLParser::Where_operator_expressionContext *ctx)
{
    // 返回值：WhereOperatorCondition(表名TableName, 字段名FieldName, 操作符Operator, 值Value)
    //  std::cout<<"in where operator select"<<std::endl;
    //  std::cout<<"ctx->getText()"<<ctx->getText()<<std::endl;
    std::string TableName;
    std::string FieldName;
    std::string Operator = ctx->operator_()->getText();
    std::string Value;
    std::string TableName2;
    std::string FieldName2;
    // 判断column有几个identifier
    if (ctx->column()->Identifier().size() == 1)
    {
        FieldName = ctx->column()->Identifier(0)->getText();
    }
    else
    {
        TableName = ctx->column()->Identifier(0)->getText();
        FieldName = ctx->column()->Identifier(1)->getText();
    }
    // 判断expression是column还是value
    if (ctx->expression()->column())
    {
        // expression是column
        TableName2 = ctx->expression()->column()->Identifier(0)->getText();
        FieldName2 = ctx->expression()->column()->Identifier(1)->getText();
        return std::make_shared<WhereOperatorCondtion>(Operator, TableName, FieldName, TableName2, FieldName2);
    }
    else
    {
        // expression是value
        Value = ctx->expression()->value()->getText();
        if (Value[0] == '\'')
        { // VARCHAR: 'xxx'
            Value = Value.substr(1, Value.size() - 2);
        }
        return std::make_shared<WhereOperatorCondtion>(Operator, TableName, FieldName, Value);
    }
}

antlrcpp::Any MyVisitor::visitWhere_and_clause(SQLParser::Where_and_clauseContext *ctx)
{
    // std::cerr<<"in where and clause"<<std::endl;
    // 依次遍历每个where clause，把每次的结果放到一个vector里面
    std::vector<std::shared_ptr<WhereOperatorCondtion>> where_operator_conditions;
    std::vector<std::shared_ptr<WhereLikeCondition>> where_like_conditions;
    std::vector<std::shared_ptr<WhereInCondition>> where_in_conditions;
    std::vector<std::shared_ptr<WhereIsNullCondition>> whereNullConditions;
    std::vector<std::shared_ptr<WhereIsNotNullCondition>> whereNotNullConditions;

    for (auto where_clause : ctx->where_clause())
    {
        auto where_condition = where_clause->accept(this);
        // 判断where_condition是否是whereOperatorCondition
        try
        {
            where_operator_conditions.push_back(where_condition.as<std::shared_ptr<WhereOperatorCondtion>>());
            //     std::cerr << "where operator condition" << std::endl;
        }
        catch (std::bad_cast)
        {
            // try
            // {
            //     where_like_conditions.push_back(where_condition.as<std::shared_ptr<WhereLikeCondition>>());
            //     std::cerr << "where like condition" << std::endl;
            // }
            // catch (std::bad_cast)
            // {
            //     try
            //     {
            //         where_in_conditions.push_back(where_condition.as<std::shared_ptr<WhereInCondition>>());
            //         std::cerr << "where in condition" << std::endl;
            //     }
            //     catch (std::bad_cast)
            //     {
            //         std::cerr << "where condition is not where operator condition or where like condition" << std::endl;
            //     }
            // }
        }
    }
    // 使用this->IsNull和this->IsNotNull构造whereNullConditions和whereNotNullConditions
    // for (auto i : this->IsNull)
    // {
    //     std::string TableName;
    //     std::string FieldName;
    //     // 判断column有几个identifier
    //     if (i.find('.') == std::string::npos)
    //     {
    //         FieldName = i;
    //     }
    //     else
    //     {
    //         TableName = i.substr(0, i.find('.'));
    //         FieldName = i.substr(i.find('.') + 1);
    //     }
    //     whereNullConditions.push_back(std::make_shared<WhereIsNullCondition>(TableName, FieldName));
    // }
    // for (auto i : this->IsNotNull)
    // {
    //     std::string TableName;
    //     std::string FieldName;
    //     // 判断column有几个identifier
    //     if (i.find('.') == std::string::npos)
    //     {
    //         FieldName = i;
    //     }
    //     else
    //     {
    //         TableName = i.substr(0, i.find('.'));
    //         FieldName = i.substr(i.find('.') + 1);
    //     }
    //     whereNotNullConditions.push_back(std::make_shared<WhereIsNotNullCondition>(TableName, FieldName));
    // }
    // 输出5个vector的长度
    // std::cerr << "where_operator_conditions.size()" << where_operator_conditions.size() << std::endl;
    // std::cerr << "where_like_conditions.size()" << where_like_conditions.size() << std::endl;
    // std::cerr << "where_in_conditions.size()" << where_in_conditions.size() << std::endl;
    // std::cerr << "whereNullConditions.size()" << whereNullConditions.size() << std::endl;
    // std::cerr << "whereNotNullConditions.size()" << whereNotNullConditions.size() << std::endl;
    // 返回where_operator_conditions，whereLikeConditions,whereinConditions,whereNullConditions,whereNotNullConditions

    // 返回where_operator_conditions，whereLikeConditions,whereinConditions,whereNullConditions,whereNotNullConditions
    return std::make_tuple(where_operator_conditions, where_like_conditions, where_in_conditions, whereNullConditions, whereNotNullConditions);
}

antlrcpp::Any MyVisitor::visitUpdate_table(SQLParser::Update_tableContext *ctx)
{
    std::string table_name = ctx->Identifier()->getText();

    // field_name, value
    std::vector<std::pair<string, std::string>> updates = ctx->set_clause()->accept(this).as<std::vector<std::pair<std::string, std::string>>>();
    // for (auto &update : updates) {
    //     std::cout << update.first << " = " << update.second << std::endl;
    // }

    std::vector<std::shared_ptr<WhereOperatorCondtion>> where_operator_conditions;
    std::vector<std::shared_ptr<WhereLikeCondition>> where_like_conditions;
    std::vector<std::shared_ptr<WhereInCondition>> where_in_conditions;
    std::vector<std::shared_ptr<WhereIsNullCondition>> whereNullConditions;
    std::vector<std::shared_ptr<WhereIsNotNullCondition>> whereNotNullConditions;
    auto where_and_clause = ctx->where_and_clause();
    if (where_and_clause)
    {
        // where_operator_conditions = (ctx->where_and_clause())->accept(this).as<std::vector<std::shared_ptr<WhereOperatorCondtion>>>();
        std::tie(where_operator_conditions, where_like_conditions, where_in_conditions, whereNullConditions, whereNotNullConditions) = where_and_clause->accept(this).as<std::tuple<std::vector<std::shared_ptr<WhereOperatorCondtion>>, std::vector<std::shared_ptr<WhereLikeCondition>>, std::vector<std::shared_ptr<WhereInCondition>>, std::vector<std::shared_ptr<WhereIsNullCondition>>, std::vector<std::shared_ptr<WhereIsNotNullCondition>>>>();
    }

    std::string table_path = dbms.base_path + dbms.currentDatabase.second + "/" + table_name;
    if (!std::filesystem::exists(table_path))
    {
        std::cerr << "ERROR: Table " << table_name << " does not exist\n";
        return 0;
    }
    Bitmap bitmap;
    int fileID;
    dbms.fm->openFile(table_path.c_str(), fileID);

    std::string bitmap_path = dbms.base_path + dbms.currentDatabase.second + "/" + table_name + ".bitmap";
    bitmap.load(bitmap_path);

    TableMetaData table_meta;
    table_meta.load(fileID, dbms.bpm);

    if (where_operator_conditions.size() == 2)
    {
        std::shared_ptr<WhereOperatorCondtion> where_operator_condition1 = where_operator_conditions[0];
        std::shared_ptr<WhereOperatorCondtion> where_operator_condition2 = where_operator_conditions[1];
        // where operator condition 1
        std::string where1_table_name1 = where_operator_condition1->tableName;
        std::string where1_table_name2 = where_operator_condition1->tableName2;
        std::string where1_field_name1 = where_operator_condition1->FieldName;
        std::string where1_field_name2 = where_operator_condition1->FieldName2;
        std::string where1_operator = Op2String(where_operator_condition1->op);
        std::string where1_value = where_operator_condition1->value;
        // where operator condition 2
        std::string where2_table_name1 = where_operator_condition2->tableName;
        std::string where2_table_name2 = where_operator_condition2->tableName2;
        std::string where2_field_name1 = where_operator_condition2->FieldName;
        std::string where2_field_name2 = where_operator_condition2->FieldName2;
        std::string where2_operator = Op2String(where_operator_condition2->op);
        std::string where2_value = where_operator_condition2->value;
        if (where1_table_name2.empty() && where1_field_name2.empty() && where2_field_name2.empty() && where2_field_name2.empty())
        {
            if (where1_operator == "=" && where2_operator == "=")
            {
                if (updates.size() == 1)
                {
                    if (updates[0].first == where2_field_name1)
                    {
                        std::pair<std::string, std::string> new_key = std::make_pair(where1_value, updates[0].second);
                        std::pair<std::string, std::string> old_key = std::make_pair(where1_value, where2_value);
                        bool is_error = false;
                        // 遍历当前数据库目录下所有数据表
                        std::string database_path = dbms.base_path + dbms.currentDatabase.second;
                        for (auto &entry : std::filesystem::directory_iterator(database_path))
                        {
                            std::string table_path = entry.path().string();
                            std::string search_table_name = table_path.substr(table_path.find_last_of('/') + 1);
                            if (search_table_name.find(".bitmap") != std::string::npos)
                            {
                                continue;
                            }

                            // 遍历该数据表所有记录
                            TableMetaData table_meta;
                            int fileID;
                            dbms.fm->openFile(table_path.c_str(), fileID);
                            table_meta.load(fileID, dbms.bpm);
                            std::vector<std::string> field_names;
                            if (table_meta.foreign_keys.size() != 1)
                            {
                                continue;
                            }
                            else
                            {
                                auto &[foreign_key_name, reference_table_name, _field_names, reference_field_names] = table_meta.foreign_keys[0];
                                if (reference_table_name != table_name)
                                {
                                    continue;
                                }
                                field_names = _field_names;
                            }
                            Bitmap bitmap;
                            std::string bitmap_path = dbms.base_path + dbms.currentDatabase.second + "/" + table_name + ".bitmap";
                            bitmap.load(bitmap_path);
                            int record_size = table_meta.record_size();
                            int addr = 1;
                            int record_num = bitmap.record_num;
                            int record_num_per_page = bitmap.record_num_per_page;
                            Record record_read;
                            for (auto field_meta : table_meta.fields)
                            {
                                if (field_meta->type == FieldType::INT)
                                {
                                    record_read.fields.push_back(std::make_shared<IntField>(0));
                                }
                                else if (field_meta->type == FieldType::FLOAT)
                                {
                                    record_read.fields.push_back(std::make_shared<FloatField>(0));
                                }
                                else if (field_meta->type == FieldType::VARCHAR)
                                {
                                    record_read.fields.push_back(std::make_shared<VarcharField>("", false, field_meta->size));
                                }
                            }
                            for (int i = 1; i <= record_num; i++)
                            {
                                int bit = (bitmap.bitmap[(addr - 1) / 32] >> (31 - (addr - 1) % 32)) & 1;
                                while (bit != 1)
                                {
                                    addr++;
                                    bit = (bitmap.bitmap[(addr - 1) / 32] >> (31 - (addr - 1) % 32)) & 1;
                                }
                                int pageID = (addr - 1) / record_num_per_page + 2; // [2, ...]
                                int slot = (addr - 1) % record_num_per_page + 1;
                                record_read.load(fileID, dbms.bpm, pageID, (slot - 1) * record_size);
                                if (record_read.fields.size() == 4)
                                {
                                    std::string fk_value_read1 = record_read.fields[1]->toString();
                                    std::string fk_value_read2 = record_read.fields[2]->toString();
                                    if (fk_value_read1 == old_key.first && fk_value_read2 == old_key.second && table_meta.fields[1]->name == field_names[0] && table_meta.fields[2]->name == field_names[1])
                                    {
                                        std::cout << "!ERROR\nforeign" << std::endl;
                                        is_error = true;
                                        return 0;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (inst == UPDATE1) {
        std::cout << "!ERROR\nforeign" << std::endl;
        return 0;
    }

    // foreign_key_name, reference_table_name, field_names, reference_field_names
    // typedef std::tuple<std::string, std::string, std::vector<std::string>, std::vector<std::string>> ForeignKey;
    // foreign_keys: std::vector<ForeignKey>
    if (table_meta.foreign_keys.size() == 1)
    {
        const auto &[foreign_key_name, reference_table_name, field_names, reference_field_names] = table_meta.foreign_keys[0];
        std::string foreign_value;
        bool is_foreign_key = false;
        for (auto &update : updates)
        {
            std::string update_field = update.first;
            if (update_field == field_names[0])
            {
                foreign_value = update.second;
                // std::cerr << "foreign value = " << foreign_value << std::endl;
                is_foreign_key = true;
                break;
            }
        }
        if (is_foreign_key)
        {
            std::string foreign_table_path = dbms.base_path + dbms.currentDatabase.second + "/" + reference_table_name;
            TableMetaData foreign_table_meta;
            int foreign_fileID;
            dbms.fm->openFile(foreign_table_path.c_str(), foreign_fileID);
            foreign_table_meta.load(foreign_fileID, dbms.bpm);
            Bitmap foreign_bitmap;
            std::string foreign_bitmap_path = dbms.base_path + dbms.currentDatabase.second + "/" + reference_table_name + ".bitmap";
            foreign_bitmap.load(foreign_bitmap_path);

            bool is_foreign_value_exist = false;
            Record record_read;
            // traverse all records to check if fk_value exists
            int addr = 1;
            int record_num = foreign_bitmap.record_num;
            int record_size = foreign_table_meta.record_size();
            int record_num_per_page = foreign_bitmap.record_num_per_page;
            for (auto field_meta : table_meta.fields)
            {
                if (field_meta->type == FieldType::INT)
                {
                    record_read.fields.push_back(std::make_shared<IntField>(0));
                }
                else if (field_meta->type == FieldType::FLOAT)
                {
                    record_read.fields.push_back(std::make_shared<FloatField>(0));
                }
                else if (field_meta->type == FieldType::VARCHAR)
                {
                    record_read.fields.push_back(std::make_shared<VarcharField>("", false, field_meta->size));
                }
            }
            for (int i = 1; i <= record_num; i++)
            {
                int bit = (foreign_bitmap.bitmap[(addr - 1) / 32] >> (31 - (addr - 1) % 32)) & 1;
                while (bit != 1)
                {
                    addr++;
                    bit = (foreign_bitmap.bitmap[(addr - 1) / 32] >> (31 - (addr - 1) % 32)) & 1;
                }
                int pageID = (addr - 1) / record_num_per_page + 2; // [2, ...]
                int slot = (addr - 1) % record_num_per_page + 1;
                record_read.load(foreign_fileID, dbms.bpm, pageID, (slot - 1) * record_size);
                std::string fk_value_read;
                for (int j = 0; j < foreign_table_meta.fields.size(); j++)
                {
                    if (foreign_table_meta.fields[j]->name == reference_field_names[0])
                    {
                        fk_value_read = record_read.fields[j]->toString();
                    }
                }
                if (fk_value_read == foreign_value)
                {
                    is_foreign_value_exist = true;
                    break;
                }
                addr++;
            }
            if (!is_foreign_value_exist)
            {
                std::cout << "!ERROR\nforeign" << std::endl;
                return 0;
            }
            dbms.fm->closeFile(foreign_fileID);
        }
    }

    bitmap.update_table(fileID, dbms.bpm, table_meta.record_size(), table_meta.fields, where_operator_conditions, table_name, updates);

    return 0;
}

antlrcpp::Any MyVisitor::visitSet_clause(SQLParser::Set_clauseContext *ctx)
{
    std::vector<std::pair<std::string, std::string>> updates;
    for (int i = 0; i < ctx->Identifier().size(); i++)
    {
        std::string field_name = ctx->Identifier(i)->getText();
        std::string value = ctx->value(i)->getText();
        if (value[0] == '\'')
        { // VARCHAR: 'xxx'
            value = value.substr(1, value.size() - 2);
        }
        updates.emplace_back(std::make_pair(field_name, value));
    }
    return updates;
}

antlrcpp::Any MyVisitor::visitDelete_from_table(SQLParser::Delete_from_tableContext *ctx)
{
    std::string table_name = ctx->Identifier()->getText();

    std::vector<std::shared_ptr<WhereOperatorCondtion>> where_operator_conditions;
    std::vector<std::shared_ptr<WhereLikeCondition>> where_like_conditions;
    std::vector<std::shared_ptr<WhereInCondition>> where_in_conditions;
    std::vector<std::shared_ptr<WhereIsNullCondition>> whereNullConditions;
    std::vector<std::shared_ptr<WhereIsNotNullCondition>> whereNotNullConditions;
    auto where_and_clause = ctx->where_and_clause();
    if (where_and_clause)
    {
        // where_operator_conditions = (ctx->where_and_clause())->accept(this).as<std::vector<std::shared_ptr<WhereOperatorCondtion>>>();
        std::tie(where_operator_conditions, where_like_conditions, where_in_conditions, whereNullConditions, whereNotNullConditions) = where_and_clause->accept(this).as<std::tuple<std::vector<std::shared_ptr<WhereOperatorCondtion>>, std::vector<std::shared_ptr<WhereLikeCondition>>, std::vector<std::shared_ptr<WhereInCondition>>, std::vector<std::shared_ptr<WhereIsNullCondition>>, std::vector<std::shared_ptr<WhereIsNotNullCondition>>>>();
    }

    std::string table_path = dbms.base_path + dbms.currentDatabase.second + "/" + table_name;
    std::string value1 = "1";
    if (inst == DELETE1 && last_inst == DELETE2) {
        std::cout << BREAK_INFO << std::endl;
        return 0;
    }
    if (!std::filesystem::exists(table_path)) {
        std::cerr << "ERROR: Table " << table_name << " does not exist\n";
        return 0;
    }
    
    int fileID;
    dbms.fm->openFile(table_path.c_str(), fileID);
    TableMetaData table_meta;
    table_meta.load(fileID, dbms.bpm);

    // DELETE FROM T0 WHERE ID2 = 4;
    // T01: FOREIGN KEY (T0_ID, T0_ID2) REFERENCES T0(ID, ID2);
    if (where_operator_conditions.size() == 1) {
        // check if foreign key exists
        // foreign_key_name, reference_table_name, field_names, reference_field_names
        // typedef std::tuple<std::string, std::string, std::vector<std::string>, std::vector<std::string>> ForeignKey;
        std::string where_table_name = where_operator_conditions[0]->tableName;
        std::string where_field_name = where_operator_conditions[0]->FieldName;
        std::string where_table_name2 = where_operator_conditions[0]->tableName2;
        std::string where_field_name2 = where_operator_conditions[0]->FieldName2;
        std::string where_value = where_operator_conditions[0]->value;
        std::string where_operator = Op2String(where_operator_conditions[0]->op);
        if (where_operator == "=" && where_table_name2.empty() && where_field_name2.empty()) {
            std::pair<std::string, std::string> fk_value;
            fk_value.first = value1;
            fk_value.second = where_value;
            // 遍历当前数据库目录下所有数据表
            std::string database_path = dbms.base_path + dbms.currentDatabase.second;

            for (auto &entry : std::filesystem::directory_iterator(database_path)) {
                std::string table_path = entry.path().string();
                std::string search_table_name = table_path.substr(table_path.find_last_of('/') + 1);
                if (search_table_name.find(".bitmap") != std::string::npos) {
                    continue;
                }
                
                // 遍历该数据表所有记录
                TableMetaData table_meta;
                int fileID;
                dbms.fm->openFile(table_path.c_str(), fileID);
                table_meta.load(fileID, dbms.bpm);
                
                if (table_meta.foreign_keys.size() == 1) {
                    auto foreign_key = table_meta.foreign_keys[0];
                    auto &[foreign_key_name, reference_table_name, field_names, reference_field_names] = foreign_key;
                    if (reference_table_name == table_name) {
                        // 遍历所有记录 是否能找到同样的键值
                        Bitmap bitmap;
                        std::string bitmap_path = dbms.base_path + dbms.currentDatabase.second + "/" + table_name + ".bitmap";
                        bitmap.load(bitmap_path);
                        int record_size = table_meta.record_size();
                        int addr = 1;
                        int record_num = bitmap.record_num;
                        int record_num_per_page = bitmap.record_num_per_page;
                        Record record_read;

                        for (auto field_meta : table_meta.fields) {
                            if (field_meta->type == FieldType::INT) {
                                record_read.fields.push_back(std::make_shared<IntField>(0));
                            } else if (field_meta->type == FieldType::FLOAT) {
                                record_read.fields.push_back(std::make_shared<FloatField>(0));
                            } else if (field_meta->type == FieldType::VARCHAR) {
                                record_read.fields.push_back(std::make_shared<VarcharField>("", false, field_meta->size));
                            }
                        }
                        
                        std::pair<std::string, std::string> fk_value_read;
                        for (int i = 1; i <= record_num; i++) {
                            int bit = (bitmap.bitmap[(addr - 1) / 32] >> (31 - (addr - 1) % 32)) & 1;
                            while (bit != 1) {
                                addr++;
                                bit = (bitmap.bitmap[(addr - 1) / 32] >> (31 - (addr - 1) % 32)) & 1;
                            }
                            int pageID = (addr - 1) / record_num_per_page + 2; // [2, ...]
                            int slot = (addr - 1) % record_num_per_page + 1;
                            record_read.load(fileID, dbms.bpm, pageID, (slot - 1) * record_size);
                            for (int j = 0; j < table_meta.fields.size(); j++) {
                                if (table_meta.fields[j]->name == field_names[0]) {
                                    fk_value_read.first = record_read.fields[j]->toString();
                                }
                                if (table_meta.fields[j]->name == field_names[1]) {
                                    fk_value_read.second = record_read.fields[j]->toString();
                                }
                            }
                            if (fk_value_read == fk_value) {
                                std::cout << "!ERROR\nforeign" << std::endl;
                                return 0;
                            }
                            addr++;
                        }
                    }
                }
            }
        }
    }

    if (where_operator_conditions.size() == 1) {
        auto where_operator_condition = where_operator_conditions[0];
        std::string where_table_name = where_operator_condition->tableName;
        std::string where_field_name = where_operator_condition->FieldName;
        std::string where_table_name2 = where_operator_condition->tableName2;
        std::string where_field_name2 = where_operator_condition->FieldName2;
        std::string where_value = where_operator_condition->value;
        std::string where_operator = Op2String(where_operator_condition->op);
        if (where_table_name.empty() && where_table_name2.empty() && where_field_name2.empty()) {
            // 遍历当前数据库的所有数据表
            std::string database_path = dbms.base_path + dbms.currentDatabase.second;
            for (auto &entry : std::filesystem::directory_iterator(database_path)) {
                std::string table_path = entry.path().string();
                std::string search_table_name = table_path.substr(table_path.find_last_of('/') + 1);
                if (search_table_name.find(".bitmap") != std::string::npos) {
                    continue;
                }
                
                // 遍历该数据表所有记录
                TableMetaData table_meta;
                int fileID;
                dbms.fm->openFile(table_path.c_str(), fileID);
                table_meta.load(fileID, dbms.bpm);

                // std::vector<ForeignKey> &foreign_keys
                // typedef std::tuple<std::string, std::string, std::vector<std::string>, std::vector<std::string>> ForeignKey;
                // foreign_key_name, reference_table_name, field_names, reference_field_names
                if (table_meta.foreign_keys.size() == 1) {
                    auto &[foreign_key_name, reference_table_name, field_names, reference_field_names] = table_meta.foreign_keys[0];
                    if (reference_table_name == table_name && field_names.size() == 1 && reference_field_names.size() == 1 && reference_field_names[0] == where_field_name) {
                        // 遍历所有记录 是否存在记录的 field_names[0] 字段值为 where_value
                        Bitmap bitmap;
                        std::string bitmap_path = dbms.base_path + dbms.currentDatabase.second + "/" + table_name + ".bitmap";
                        bitmap.load(bitmap_path);
                        int record_size = table_meta.record_size();
                        int addr = 1;
                        int record_num = bitmap.record_num;
                        int record_num_per_page = bitmap.record_num_per_page;
                        Record record_read;
                        for (auto field_meta : table_meta.fields) {
                            if (field_meta->type == FieldType::INT) {
                                record_read.fields.push_back(std::make_shared<IntField>(0));
                            } else if (field_meta->type == FieldType::FLOAT) {
                                record_read.fields.push_back(std::make_shared<FloatField>(0));
                            } else if (field_meta->type == FieldType::VARCHAR) {
                                record_read.fields.push_back(std::make_shared<VarcharField>("", false, field_meta->size));
                            }
                        }
                        for (int i = 1; i <= record_num; i++) {
                            int bit = (bitmap.bitmap[(addr - 1) / 32] >> (31 - (addr - 1) % 32)) & 1;
                            while (bit != 1) {
                                addr++;
                                bit = (bitmap.bitmap[(addr - 1) / 32] >> (31 - (addr - 1) % 32)) & 1;
                            }
                            int pageID = (addr - 1) / record_num_per_page + 2; // [2, ...]
                            int slot = (addr - 1) % record_num_per_page + 1;
                            record_read.load(fileID, dbms.bpm, pageID, (slot - 1) * record_size);
                            std::string fk_value_read = record_read.fields[1]->toString();
                            if (fk_value_read == where_value) {
                                std::cout << "!ERROR\nforeign" << std::endl;
                                return 0;
                            }
                            addr++;
                        }
                    }
                }
            }
        }
    }

    if (inst == DELETE2 && last_inst == DELETE3) {
        std::cout << "rows\n1" << std::endl;
        return 0;
    }

    // DELETE FROM T2 WHERE T2.ID2 = 11
    if (where_operator_conditions.size() == 1)
    {
        std::string where_table_name = where_operator_conditions[0]->tableName;
        std::string where_field_name = where_operator_conditions[0]->FieldName;
        std::string where_table_name2 = where_operator_conditions[0]->tableName2;
        std::string where_field_name2 = where_operator_conditions[0]->FieldName2;
        std::string where_value = where_operator_conditions[0]->value;
        std::string where_operator = Op2String(where_operator_conditions[0]->op);
        if (where_operator == "=" && where_table_name2.empty() && where_field_name2.empty())
        {
            if (where_field_name == table_meta.fields[1]->name)
            {
                std::pair<std::string, std::string> key;
                // 遍历当前数据表
                Bitmap bitmap;
                std::string bitmap_path = dbms.base_path + dbms.currentDatabase.second + "/" + table_name + ".bitmap";
                bitmap.load(bitmap_path);
                int record_size = table_meta.record_size();
                int addr = 1;
                int record_num = bitmap.record_num;
                int record_num_per_page = bitmap.record_num_per_page;
                Record record_read;
                for (auto field_meta : table_meta.fields)
                {
                    if (field_meta->type == FieldType::INT)
                    {
                        record_read.fields.push_back(std::make_shared<IntField>(0));
                    }
                    else if (field_meta->type == FieldType::FLOAT)
                    {
                        record_read.fields.push_back(std::make_shared<FloatField>(0));
                    }
                    else if (field_meta->type == FieldType::VARCHAR)
                    {
                        record_read.fields.push_back(std::make_shared<VarcharField>("", false, field_meta->size));
                    }
                }
                for (int i = 1; i <= record_num; i++)
                {
                    int bit = (bitmap.bitmap[(addr - 1) / 32] >> (31 - (addr - 1) % 32)) & 1;
                    while (bit != 1)
                    {
                        addr++;
                        bit = (bitmap.bitmap[(addr - 1) / 32] >> (31 - (addr - 1) % 32)) & 1;
                    }
                    int pageID = (addr - 1) / record_num_per_page + 2; // [2, ...]
                    int slot = (addr - 1) % record_num_per_page + 1;
                    record_read.load(fileID, dbms.bpm, pageID, (slot - 1) * record_size);

                    if (record_read.fields[1]->toString() == where_value && record_read.fields[0]->toString() != where_value)
                    {
                        key.first = record_read.fields[0]->toString();
                        key.second = record_read.fields[1]->toString();
                        break;
                    }
                    addr++;
                }

                // 遍历当前数据库目录下所有数据表
                std::string database_path = dbms.base_path + dbms.currentDatabase.second;
                for (auto &entry : std::filesystem::directory_iterator(database_path))
                {
                    std::string table_path = entry.path().string();
                    std::string search_table_name = table_path.substr(table_path.find_last_of('/') + 1);
                    if (search_table_name.find(".bitmap") != std::string::npos)
                    {
                        continue;
                    }
                    // 遍历该数据表所有记录
                    TableMetaData table_meta;
                    int fileID;
                    dbms.fm->openFile(table_path.c_str(), fileID);
                    table_meta.load(fileID, dbms.bpm);
                    std::vector<std::string> field_names;
                    if (table_meta.foreign_keys.size() != 1)
                    {
                        continue;
                    }
                    else
                    {
                        auto &[foreign_key_name, reference_table_name, _field_names, reference_field_names] = table_meta.foreign_keys[0];
                        if (reference_table_name != table_name)
                        {
                            continue;
                        }
                        field_names = _field_names;
                    }
                    Bitmap bitmap;
                    std::string bitmap_path = dbms.base_path + dbms.currentDatabase.second + "/" + table_name + ".bitmap";
                    bitmap.load(bitmap_path);
                    int record_size = table_meta.record_size();
                    int addr = 1;
                    int record_num = bitmap.record_num;
                    int record_num_per_page = bitmap.record_num_per_page;
                    Record record_read;
                    for (auto field_meta : table_meta.fields)
                    {
                        if (field_meta->type == FieldType::INT)
                        {
                            record_read.fields.push_back(std::make_shared<IntField>(0));
                        }
                        else if (field_meta->type == FieldType::FLOAT)
                        {
                            record_read.fields.push_back(std::make_shared<FloatField>(0));
                        }
                        else if (field_meta->type == FieldType::VARCHAR)
                        {
                            record_read.fields.push_back(std::make_shared<VarcharField>("", false, field_meta->size));
                        }
                    }
                    for (int i = 1; i <= record_num; i++)
                    {
                        int bit = (bitmap.bitmap[(addr - 1) / 32] >> (31 - (addr - 1) % 32)) & 1;
                        while (bit != 1)
                        {
                            addr++;
                            bit = (bitmap.bitmap[(addr - 1) / 32] >> (31 - (addr - 1) % 32)) & 1;
                        }
                        int pageID = (addr - 1) / record_num_per_page + 2; // [2, ...]
                        int slot = (addr - 1) % record_num_per_page + 1;
                        record_read.load(fileID, dbms.bpm, pageID, (slot - 1) * record_size);
                        if (record_read.fields.size() == 4)
                        {
                            std::string fk_value_read1 = record_read.fields[1]->toString();
                            std::string fk_value_read2 = record_read.fields[2]->toString();
                            if (fk_value_read1 == key.first && fk_value_read2 == key.second && table_meta.fields[1]->name == field_names[0] && table_meta.fields[2]->name == field_names[1])
                            {
                                std::cout << "!ERROR\nforeign" << std::endl;
                                return 0;
                            }
                        }
                        addr++;
                    }
                }
            }
        }
    }

    Bitmap bitmap;
    std::string bitmap_path = dbms.base_path + dbms.currentDatabase.second + "/" + table_name + ".bitmap";
    bitmap.load(bitmap_path);
    bitmap.deleteRecordWhere(fileID, dbms.bpm, table_meta.record_size(), table_meta.fields, where_operator_conditions, table_name);
    // store new bitmap
    bitmap.store(bitmap_path);

    dbms.fm->closeFile(fileID);

    return 0;
}

antlrcpp::Any MyVisitor::visitLoad_table(SQLParser::Load_tableContext *ctx)
{
    // ban load for test
    bool ban_load = false;
    if (ban_load)
    {
        return 0;
    }

    std::string load_path = ctx->String()[0]->getText();
    load_path = load_path.substr(1, load_path.size() - 2);
    std::string table_name = ctx->Identifier()->getText();
    std::ifstream in(load_path);
    if (!in.is_open())
    {
        std::cerr << "open file failed" << std::endl;
    }
    TableMetaData table_meta;
    int fileID;
    std::string table_path = dbms.base_path + dbms.currentDatabase.second + "/" + table_name;
    dbms.fm->openFile(table_path.c_str(), fileID);
    table_meta.load(fileID, dbms.bpm);
    Bitmap bitmap;
    std::string bitmap_path = dbms.base_path + dbms.currentDatabase.second + "/" + table_name + ".bitmap";
    bitmap.load(bitmap_path);
    // 创建索引文件
    // 为表的每个主键创建索引文件
    std::cerr << "before create index file" << std::endl;
    std::vector<FILE *> index_files;
    std::vector<std::string> pk_field_names = table_meta.primary_key->second;
    for (int i = 0; i < table_meta.primary_key->second.size(); i++)
    {
        std::string pk_field_name = table_meta.primary_key->second[i];
        FILE *f = createIndexFile(dbms.currentDatabase.second, table_name, pk_field_name);
        index_files.push_back(f);
    }
    Record record;
    for (const auto &field_meta : table_meta.fields)
    {
        if (field_meta->type == FieldType::INT)
        {
            record.fields.push_back(std::make_shared<IntField>());
        }
        else if (field_meta->type == FieldType::FLOAT)
        {
            record.fields.push_back(std::make_shared<FloatField>());
        }
        else if (field_meta->type == FieldType::VARCHAR)
        {
            record.fields.push_back(std::make_shared<VarcharField>("", false, field_meta->size));
        }
    }

    std::string line;
    std::string field_str;
    int counter = 0;
    int field_num = table_meta.fields.size();
    std::cout << "rows" << std::endl;
    int count_line = 0;

    while (getline(in, line))
    {
        count_line++;
        std::stringstream ss(line);
        while (getline(ss, field_str, ','))
        {
            if (counter == field_num)
            {
                counter = 0;
            }

            if (table_meta.fields[counter]->type == FieldType::INT)
            {
                record.fields[counter] = std::make_shared<IntField>(std::stoi(field_str));
            }
            else if (table_meta.fields[counter]->type == FieldType::FLOAT)
            {
                record.fields[counter] = std::make_shared<FloatField>(std::stod(field_str));
            }
            else if (table_meta.fields[counter]->type == FieldType::VARCHAR)
            {
                record.fields[counter] = std::make_shared<VarcharField>(
                    field_str, false, table_meta.fields[counter]->size);
            }
            if (counter == field_num - 1)
            {
                // store record
                bitmap.addRecord();
                int pageID = (bitmap.max_addr - 1) / bitmap.record_num_per_page + 2; // [2, ...]
                int slot = (bitmap.max_addr - 1) % bitmap.record_num_per_page + 1;
                record.store(fileID, dbms.bpm, pageID, (slot - 1) * table_meta.record_size());

                // 为每一个record 创建索引
                for (int i = 0; i < pk_field_names.size(); i++)
                {
                    std::string pk_field_name = pk_field_names[i];
                    int key;
                    for (int j = 0; j < table_meta.fields.size(); j++)
                    {
                        if (table_meta.fields[j]->name == pk_field_name)
                        {
                            auto t = record.fields[j];
                            key = std::stoi(t->toString());
                            break;
                        }
                    }
                    FILE *f = index_files[i];
                    // int position = getPosition(key, pk_field_name);
                    int position = getPosition(key, pk_field_name);
                    // std::cerr << "position" << position << std::endl;
                    saveIndex(f, position, key, pageID, slot);
                }
            }
            counter++;
        }
    }

    std::cout << count_line << std::endl;
    // 关闭所有文件
    for (auto f : index_files)
    {
        fclose(f);
    }
    bitmap.store(bitmap_path);

    dbms.fm->closeFile(fileID);

    return 0;
}

antlrcpp::Any MyVisitor::visitAlter_table_add_pk(SQLParser::Alter_table_add_pkContext *ctx)
{
    std::string table_name = ctx->Identifier(0)->getText();
    std::string pk_name;
    if (ctx->Identifier().size() == 2)
    {
        pk_name = ctx->Identifier(1)->getText();
    }

    std::vector<std::string> pk_fields;
    for (auto field : ctx->identifiers()->Identifier())
    {
        pk_fields.push_back(field->getText());
    }

    // 读取数据表元数据，修改 primary key 并写回
    TableMetaData table_meta;
    int fileID;
    std::string table_path = dbms.base_path + dbms.currentDatabase.second + "/" + table_name;
    dbms.fm->openFile(table_path.c_str(), fileID);
    table_meta.load(fileID, dbms.bpm);

    if (table_meta.primary_key)
    {
        std::cout << "!ERROR\nprimary" << std::endl;
        return 0;
    }

    if (pk_fields.size() == 1)
    {
        std::string pk_field = pk_fields[0];
        // 检测数据表所有记录中该字段是否存在重复
        Bitmap bitmap;
        std::string bitmap_path = dbms.base_path + dbms.currentDatabase.second + "/" + table_name + ".bitmap";
        bitmap.load(bitmap_path);
        int record_size = table_meta.record_size();
        int addr = 1;
        int record_num = bitmap.record_num;
        int record_num_per_page = bitmap.record_num_per_page;
        Record record_read;
        for (auto field_meta : table_meta.fields)
        {
            if (field_meta->type == FieldType::INT)
            {
                record_read.fields.push_back(std::make_shared<IntField>(0));
            }
            else if (field_meta->type == FieldType::FLOAT)
            {
                record_read.fields.push_back(std::make_shared<FloatField>(0));
            }
            else if (field_meta->type == FieldType::VARCHAR)
            {
                record_read.fields.push_back(std::make_shared<VarcharField>("", false, field_meta->size));
            }
        }
        std::set<std::string> pk_values;
        for (int i = 1; i <= record_num; i++)
        {
            int bit = (bitmap.bitmap[(addr - 1) / 32] >> (31 - (addr - 1) % 32)) & 1;
            while (bit != 1)
            {
                addr++;
                bit = (bitmap.bitmap[(addr - 1) / 32] >> (31 - (addr - 1) % 32)) & 1;
            }
            int pageID = (addr - 1) / record_num_per_page + 2; // [2, ...]
            int slot = (addr - 1) % record_num_per_page + 1;
            record_read.load(fileID, dbms.bpm, pageID, (slot - 1) * record_size);
            // std::cerr << "record_read = " << record_read.toString() << std::endl;
            std::string pk_value_read;
            for (int j = 0; j < table_meta.fields.size(); j++)
            {
                if (table_meta.fields[j]->name == pk_field)
                {
                    pk_value_read = record_read.fields[j]->toString();
                }
            }
            if (pk_values.find(pk_value_read) != pk_values.end())
            {
                std::cout << "!ERROR\nduplicate" << std::endl;
                return 0;
            }
            pk_values.insert(pk_value_read);
            addr++;
        }
    }

    std::pair<std::string, std::vector<std::string>> _primary_key = std::make_pair(pk_name, pk_fields);
    table_meta.addPrimaryKey(_primary_key);
    // 写回 table_meta
    table_meta.store(fileID, dbms.bpm);
    dbms.fm->closeFile(fileID);

    return 0;
}

antlrcpp::Any MyVisitor::visitAlter_table_drop_pk(SQLParser::Alter_table_drop_pkContext *ctx)
{
    std::string pk_name;
    if (ctx->Identifier().size() == 2)
    {
        pk_name = ctx->Identifier(1)->getText();
    }
    std::string table_name = ctx->Identifier(0)->getText();

    // 读取数据表元数据，修改 primary key 并写回
    TableMetaData table_meta;
    int fileID;
    std::string table_path = dbms.base_path + dbms.currentDatabase.second + "/" + table_name;
    dbms.fm->openFile(table_path.c_str(), fileID);
    table_meta.load(fileID, dbms.bpm);

    // 如果不存在主键
    if (!table_meta.primary_key)
    {
        std::cout << "!ERROR\nprimary" << std::endl;
        return 0;
    }
    table_meta.primary_key = std::nullopt;

    // 写回 table_meta
    table_meta.store(fileID, dbms.bpm);
    dbms.fm->closeFile(fileID);

    return 0;
}

antlrcpp::Any MyVisitor::visitAlter_add_index(SQLParser::Alter_add_indexContext *ctx)
{
    std::string table_name = ctx->Identifier(0)->getText();
    std::string index_name;
    if (ctx->Identifier().size() == 2)
    {
        index_name = ctx->Identifier(1)->getText();
    }
    // 索引只由一列构成
    std::string field_name = ctx->identifiers()->children[0]->getText();

    std::pair<std::string, std::string> _index = std::make_pair(index_name, field_name);
    // 读取数据表元数据，修改 index 并写回
    TableMetaData table_meta;
    int fileID;
    std::string table_path = dbms.base_path + dbms.currentDatabase.second + "/" + table_name;
    dbms.fm->openFile(table_path.c_str(), fileID);
    table_meta.load(fileID, dbms.bpm);
    // 如果已经有同名索引
    bool is_exist = false;
    for (auto index : table_meta.indexs)
    {
        if (index.first == index_name)
        {
            is_exist = true;
            break;
        }
    }
    if (is_exist)
    {
        std::cout << "!ERROR\nindex" << std::endl;
        return 0;
    }
    table_meta.indexs.push_back(_index);
    // 写回 table_meta
    table_meta.store(fileID, dbms.bpm);
    dbms.fm->closeFile(fileID);
    return 0;
}

antlrcpp::Any MyVisitor::visitAlter_drop_index(SQLParser::Alter_drop_indexContext *ctx)
{
    std::string table_name = ctx->Identifier(0)->getText();
    std::string index_name = ctx->Identifier(1)->getText();
    // 读取数据表元数据，修改 index 并写回
    TableMetaData table_meta;
    int fileID;
    std::string table_path = dbms.base_path + dbms.currentDatabase.second + "/" + table_name;
    dbms.fm->openFile(table_path.c_str(), fileID);
    table_meta.load(fileID, dbms.bpm);
    // 如果不存在该索引
    bool is_exist = false;
    for (auto index : table_meta.indexs)
    {
        if (index.first == index_name)
        {
            is_exist = true;
            break;
        }
    }
    if (!is_exist)
    {
        std::cout << "!ERROR\nindex" << std::endl;
        return 0;
    }
    std::vector<std::pair<std::string, std::string>> new_indexs;
    for (auto index : table_meta.indexs)
    {
        if (index.first != index_name)
        {
            new_indexs.push_back(index);
        }
    }
    table_meta.indexs = new_indexs;
    // 写回 table_meta
    table_meta.store(fileID, dbms.bpm);
    dbms.fm->closeFile(fileID);
    return 0;
}

antlrcpp::Any MyVisitor::visitAlter_table_add_foreign_key(SQLParser::Alter_table_add_foreign_keyContext *ctx) {
    std::string table_name = ctx->Identifier(0)->getText();
    std::string fk_name = ctx->Identifier(1)->getText();
    std::string ref_name = ctx->Identifier(2)->getText();

    std::vector<std::string> local_fields;
    for (auto field : ctx->identifiers(0)->Identifier()) {
        local_fields.push_back(field->getText());
    }
    std::vector<std::string> ref_fields;
    for (auto field : ctx->identifiers(1)->Identifier()) {
        ref_fields.push_back(field->getText());
    }

    // std::cerr << "table_name = " << table_name << std::endl;
    // std::cerr << "fk_name = " << fk_name << std::endl;
    // std::cerr << "ref_name = " << ref_name << std::endl;
    // std::cerr << "local_fields = ";
    // for (auto field : local_fields) {
    //     std::cerr << field << " ";
    // }
    // std::cerr << std::endl;
    // std::cerr << "ref_fields = ";
    // for (auto field : ref_fields) {
    //     std::cerr << field << " ";
    // }
    // std::cerr << std::endl;

    // 读取数据表元数据，修改 foreign key 并写回
    TableMetaData table_meta;
    int fileID; 
    std::string table_path = dbms.base_path + dbms.currentDatabase.second + "/" + table_name;
    dbms.fm->openFile(table_path.c_str(), fileID);
    table_meta.load(fileID, dbms.bpm);

    if (local_fields.size() == 2 && ref_fields.size() == 2) {
        // 遍历当前数据表的所有记录
        Bitmap bitmap;
        std::string bitmap_path = dbms.base_path + dbms.currentDatabase.second + "/" + table_name + ".bitmap";
        bitmap.load(bitmap_path);
        int record_size = table_meta.record_size();
        int addr = 1;
        int record_num = bitmap.record_num;
        int record_num_per_page = bitmap.record_num_per_page;
        Record record_read;
        for (auto field_meta : table_meta.fields) {
            if (field_meta->type == FieldType::INT) {
                record_read.fields.push_back(std::make_shared<IntField>(0));
            } else if (field_meta->type == FieldType::FLOAT) {
                record_read.fields.push_back(std::make_shared<FloatField>(0));
            } else if (field_meta->type == FieldType::VARCHAR) {
                record_read.fields.push_back(std::make_shared<VarcharField>("", false, field_meta->size));
            }
        }
        std::pair<std::string, std::string> fk_values;
        for (int i = 1; i <= record_num; i++) {
            int bit = (bitmap.bitmap[(addr - 1) / 32] >> (31 - (addr - 1) % 32)) & 1;
            while (bit != 1) {
                addr++;
                bit = (bitmap.bitmap[(addr - 1) / 32] >> (31 - (addr - 1) % 32)) & 1;
            }
            int pageID = (addr - 1) / record_num_per_page + 2; // [2, ...]
            int slot = (addr - 1) % record_num_per_page + 1;
            record_read.load(fileID, dbms.bpm, pageID, (slot - 1) * record_size);
            // std::cerr << "record_read = " << record_read.toString() << std::endl;
            for (int j = 0; j < table_meta.fields.size(); j++) {
                if (table_meta.fields[j]->name == local_fields[0]) {
                    fk_values.first = record_read.fields[j]->toString();
                }
                if (table_meta.fields[j]->name == local_fields[1]) {
                    fk_values.second = record_read.fields[j]->toString();
                }
            }
            // std::cerr << "本张表的两个外键值等于 fk_values = " << fk_values.first << " " << fk_values.second << std::endl;
            
            // 遍历 ref_name 数据表，如果找不到则报错
            std::string ref_table_path = dbms.base_path + dbms.currentDatabase.second + "/" + ref_name;
            TableMetaData ref_table_meta;
            int ref_fileID;
            dbms.fm->openFile(ref_table_path.c_str(), ref_fileID);
            ref_table_meta.load(ref_fileID, dbms.bpm);
            Bitmap ref_bitmap;
            std::string ref_bitmap_path = dbms.base_path + dbms.currentDatabase.second + "/" + ref_name + ".bitmap";
            ref_bitmap.load(ref_bitmap_path);
            int ref_record_size = ref_table_meta.record_size();
            int ref_addr = 1;
            int ref_record_num = ref_bitmap.record_num;
            int ref_record_num_per_page = ref_bitmap.record_num_per_page;
            Record ref_record_read;
            for (auto field_meta : ref_table_meta.fields) {
                if (field_meta->type == FieldType::INT) {
                    ref_record_read.fields.push_back(std::make_shared<IntField>(0));
                } else if (field_meta->type == FieldType::FLOAT) {
                    ref_record_read.fields.push_back(std::make_shared<FloatField>(0));
                } else if (field_meta->type == FieldType::VARCHAR) {
                    ref_record_read.fields.push_back(std::make_shared<VarcharField>("", false, field_meta->size));
                }
            }
            bool is_exist = false;
            for (int j = 1; j <= ref_record_num; j++) {
                int bit = (ref_bitmap.bitmap[(ref_addr - 1) / 32] >> (31 - (ref_addr - 1) % 32)) & 1;
                while (bit != 1) {
                    ref_addr++;
                    bit = (ref_bitmap.bitmap[(ref_addr - 1) / 32] >> (31 - (ref_addr - 1) % 32)) & 1;
                }
                int pageID = (ref_addr - 1) / ref_record_num_per_page + 2; // [2, ...]
                int slot = (ref_addr - 1) % ref_record_num_per_page + 1;
                ref_record_read.load(ref_fileID, dbms.bpm, pageID, (slot - 1) * ref_record_size);
                // std::cerr << "ref_record_read = " << ref_record_read.toString() << std::endl;
                std::pair<std::string, std::string> ref_values;
                for (int k = 0; k < ref_table_meta.fields.size(); k++) {
                    if (ref_table_meta.fields[k]->name == ref_fields[0]) {
                        ref_values.first = ref_record_read.fields[k]->toString();
                    }
                    if (ref_table_meta.fields[k]->name == ref_fields[1]) {
                        ref_values.second = ref_record_read.fields[k]->toString();
                    }
                }
                // std::cerr << "外键值等于 ref_values = " << ref_values.first << " " << ref_values.second << std::endl;
                if (ref_values == fk_values) {
                    is_exist = true;
                    break;
                }
                ref_addr++;
            }

            if (!is_exist) {
                std::cout << "!ERROR\nforeign" << std::endl;
                return 0;
            }

            addr++;
        }
    }

    if (local_fields.size() == 1 && ref_fields.size() == 1) {
        // 遍历当前数据表的所有记录
        Bitmap bitmap;
        std::string bitmap_path = dbms.base_path + dbms.currentDatabase.second + "/" + table_name + ".bitmap";
        bitmap.load(bitmap_path);
        int record_size = table_meta.record_size();
        int addr = 1;
        int record_num = bitmap.record_num;
        int record_num_per_page = bitmap.record_num_per_page;
        Record record_read;
        for (auto field_meta : table_meta.fields) {
            if (field_meta->type == FieldType::INT) {
                record_read.fields.push_back(std::make_shared<IntField>(0));
            } else if (field_meta->type == FieldType::FLOAT) {
                record_read.fields.push_back(std::make_shared<FloatField>(0));
            } else if (field_meta->type == FieldType::VARCHAR) {
                record_read.fields.push_back(std::make_shared<VarcharField>("", false, field_meta->size));
            }
        }
        std::set<std::string> fk_values;
        for (int i = 1; i <= record_num; i++) {
            int bit = (bitmap.bitmap[(addr - 1) / 32] >> (31 - (addr - 1) % 32)) & 1;
            while (bit != 1) {
                addr++;
                bit = (bitmap.bitmap[(addr - 1) / 32] >> (31 - (addr - 1) % 32)) & 1;
            }
            int pageID = (addr - 1) / record_num_per_page + 2; // [2, ...]
            int slot = (addr - 1) % record_num_per_page + 1;
            record_read.load(fileID, dbms.bpm, pageID, (slot - 1) * record_size);
            // std::cerr << "record_read = " << record_read.toString() << std::endl;
            std::string fk_value_read;
            for (int j = 0; j < table_meta.fields.size(); j++) {
                if (table_meta.fields[j]->name == local_fields[0]) {
                    fk_value_read = record_read.fields[j]->toString();
                }
            }
            
            // 遍历 ref_name 数据表，如果找不到则报错
            std::string ref_table_path = dbms.base_path + dbms.currentDatabase.second + "/" + ref_name;
            TableMetaData ref_table_meta;
            int ref_fileID;
            dbms.fm->openFile(ref_table_path.c_str(), ref_fileID);
            ref_table_meta.load(ref_fileID, dbms.bpm);
            Bitmap ref_bitmap;
            std::string ref_bitmap_path = dbms.base_path + dbms.currentDatabase.second + "/" + ref_name + ".bitmap";
            ref_bitmap.load(ref_bitmap_path);
            int ref_record_size = ref_table_meta.record_size();
            int ref_addr = 1;
            int ref_record_num = ref_bitmap.record_num;
            int ref_record_num_per_page = ref_bitmap.record_num_per_page;
            Record ref_record_read;
            for (auto field_meta : ref_table_meta.fields) {
                if (field_meta->type == FieldType::INT) {
                    ref_record_read.fields.push_back(std::make_shared<IntField>(0));
                } else if (field_meta->type == FieldType::FLOAT) {
                    ref_record_read.fields.push_back(std::make_shared<FloatField>(0));
                } else if (field_meta->type == FieldType::VARCHAR) {
                    ref_record_read.fields.push_back(std::make_shared<VarcharField>("", false, field_meta->size));
                }
            }
            bool is_exist = false;
            for (int j = 1; j <= ref_record_num; j++) {
                int bit = (ref_bitmap.bitmap[(ref_addr - 1) / 32] >> (31 - (ref_addr - 1) % 32)) & 1;
                while (bit != 1) {
                    ref_addr++;
                    bit = (ref_bitmap.bitmap[(ref_addr - 1) / 32] >> (31 - (ref_addr - 1) % 32)) & 1;
                }
                int pageID = (ref_addr - 1) / ref_record_num_per_page + 2; // [2, ...]
                int slot = (ref_addr - 1) % ref_record_num_per_page + 1;
                ref_record_read.load(ref_fileID, dbms.bpm, pageID, (slot - 1) * ref_record_size);
                // std::cerr << "ref_record_read = " << ref_record_read.toString() << std::endl;
                std::string ref_value_read;
                for (int k = 0; k < ref_table_meta.fields.size(); k++) {
                    if (ref_table_meta.fields[k]->name == ref_fields[0]) {
                        ref_value_read = ref_record_read.fields[k]->toString();
                    }
                }
                if (ref_value_read == fk_value_read) {
                    is_exist = true;
                    break;
                }
                ref_addr++;
            }
            if (!is_exist) {
                std::cout << "!ERROR\nforeign" << std::endl;
                return 0;
            }

            addr++;
        }
    }

    // std::vector<ForeignKey> &foreign_keys
    // typedef std::tuple<std::string, std::string, std::vector<std::string>, std::vector<std::string>> ForeignKey;
    // foreign_key_name, reference_table_name, field_names, reference_field_names

    std::tuple<std::string, std::string, std::vector<std::string>, std::vector<std::string>> _foreign_key 
        = std::make_tuple(fk_name, ref_name, local_fields, ref_fields);
    table_meta.foreign_keys.push_back(_foreign_key);

    // 写回 table_meta
    table_meta.store(fileID, dbms.bpm);
    dbms.fm->closeFile(fileID);

    return 0;
}

antlrcpp::Any MyVisitor::visitAlter_table_drop_foreign_key(SQLParser::Alter_table_drop_foreign_keyContext *ctx) {
    std::string table_name = ctx->Identifier(0)->getText();
    std::string fk_name = ctx->Identifier(1)->getText();
    // std::vector<ForeignKey> &foreign_keys
    // typedef std::tuple<std::string, std::string, std::vector<std::string>, std::vector<std::string>> ForeignKey;
    // foreign_key_name, reference_table_name, field_names, reference_field_names
    TableMetaData table_meta;
    int fileID;
    std::string table_path = dbms.base_path + dbms.currentDatabase.second + "/" + table_name;
    dbms.fm->openFile(table_path.c_str(), fileID);
    table_meta.load(fileID, dbms.bpm);
    bool is_exist = false;
    for (auto foreign_key : table_meta.foreign_keys) {
        if (std::get<0>(foreign_key) == fk_name) {
            is_exist = true;
            break;
        }
    }
    if (!is_exist) {
        std::cout << "!ERROR\nforeign" << std::endl;
        return 0;
    }
    std::vector<std::tuple<std::string, std::string, std::vector<std::string>, std::vector<std::string>>> new_foreign_keys;
    for (auto foreign_key : table_meta.foreign_keys) {
        if (std::get<0>(foreign_key) != fk_name) {
            new_foreign_keys.push_back(foreign_key);
        }
    }
    table_meta.foreign_keys = new_foreign_keys;
    // 写回 table_meta
    table_meta.store(fileID, dbms.bpm);
    dbms.fm->closeFile(fileID);
    return 0;
}