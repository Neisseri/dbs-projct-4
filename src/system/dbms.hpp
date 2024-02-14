#pragma once

#include <cstring>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>

#include "../filesystem/bufmanager/BufPageManager.h"
#include "../filesystem/fileio/FileManager.h"
#include "../filesystem/utils/pagedef.h"
#include "../record/metadata.hpp"
#include "../constants.h"
#include "../record/bitmap.hpp"

// Note:
// using 'filesystem' lib in this module, which means '-std=c++17' is needed for compiling

// database management system
class DBMS {
public:
    FileManager* fm;
    BufPageManager* bpm;
    std::string root_path;
    std::string base_path;
    std::string global_path;
    std::vector<std::string> databaseList;
    std::pair<bool, std::string> currentDatabase; // false -> no current database

    explicit DBMS(FileManager* fm, BufPageManager* bpm, std::string root_path) {
        this->fm = fm;
        this->bpm = bpm;
        this->root_path = root_path;
        this->base_path = root_path + "base/";
        this->global_path = root_path + "global/";
        this->currentDatabase.first = false;
        
        // if global meta file exists
        if (std::filesystem::exists(global_path + "meta")) {
            std::ifstream file(global_path + "meta");
            if (file.is_open()) {
                std::string line;
                while (std::getline(file, line)) {
                    databaseList.push_back(line);
                }
                file.close();
            } else {
                std::cerr << "Failed to open file " << global_path + "meta\n";
            }
        } else {
            std::ofstream file(global_path + "meta");
        }
    }

    void storeDatabaseList() {
        std::ofstream file(global_path + "meta");
        if (file.is_open()) {
            for (int i = 0; i < databaseList.size(); i++) {
                file << databaseList[i] << "\n"; 
            }
            file.close();
        } else {
            std::cerr << "Failed to open file " << global_path + "meta\n";
        }
    }

    void createDatabase(std::string name) {
        // if database already exists
        for (int i = 0; i < databaseList.size(); i++) {
            if (databaseList[i] == name) {
                std::cerr << "Database " << name << " already exists\n";
                return;
            }
        }
        std::filesystem::create_directory(base_path + name);
        databaseList.push_back(name);
        storeDatabaseList();
    }

    void useDatabase(std::string name) {
        // if database does not exist
        for (int i = 0; i < databaseList.size(); i++) {
            if (databaseList[i] == name) {
                currentDatabase.first = true;   
                currentDatabase.second = name;
                return;
            }
        }
        std::cerr << "Database " << name << " does not exist\n";
    }

    void deleteDatabase(std::string name) {
        // if database does not exist
        for (int i = 0; i < databaseList.size(); i++) {
            if (databaseList[i] == name) {
                std::filesystem::remove_all(base_path + name);
                databaseList.erase(databaseList.begin() + i);
                storeDatabaseList();
                return;
            }
        }
        std::cerr << "Database " << name << " does not exist\n";
    }

    void createTable(
        std::string name, 
        std::vector<std::shared_ptr<FieldMetaData>> meta_datas,
        std::optional<std::pair<std::string, std::vector<std::string>>> primary_key,
        std::vector<ForeignKey> &foreign_keys
    ) {
        if (currentDatabase.first == false) {
            std::cerr << "No database selected\n";
            return;
        }
        // create data table file
        std::string file_path = base_path + currentDatabase.second + "/" + name;
        fm->createFile(file_path.c_str());
        int table_id = 0;
        for (const auto &entry : std::filesystem::directory_iterator(base_path + currentDatabase.second)) {
            table_id++;
        }
        TableMetaData table_meta(table_id, name, meta_datas, primary_key, foreign_keys);

        int fileID;
        fm->openFile(file_path.c_str(), fileID);
        table_meta.store(fileID, bpm);
        fm->closeFile(fileID);

        // store bitmap into the bitmap file
        Bitmap bitmap(table_meta.record_size());
        std::string bitmap_path = base_path + currentDatabase.second + "/" + name + ".bitmap";
        fm->createFile(bitmap_path.c_str());
        bitmap.store(bitmap_path);
    }

    TableID getTableID(std::string name) {
        if (currentDatabase.first == false) {
            std::cerr << "No database selected\n";
            return -1;
        }
        std::string file_path = base_path + currentDatabase.second + "/" + name;
        if (std::filesystem::exists(file_path)) {
            int fileID;
            fm->openFile(file_path.c_str(), fileID);
            TableMetaData table_meta;
            table_meta.load(fileID, bpm);
            fm->closeFile(fileID);
            return table_meta.table_id;
        }
        std::cerr << "Table " << name << " does not exist\n";
        return -1;
    }
};

#define DELETE_ERROE "DELETE FROM T0 WHERE ID = 2"