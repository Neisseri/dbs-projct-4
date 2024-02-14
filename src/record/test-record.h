#include "../filesystem/bufmanager/BufPageManager.h"
#include "../filesystem/fileio/FileManager.h"
#include "../filesystem/utils/pagedef.h"

#include "constants.h"

#include "metadata.hpp"
#include "field.hpp"
#include "bitmap.hpp"
#include "record.hpp"

#include <iostream>
#include <cstring>
#include <vector>
#include <memory>
#include <cstdint>

int test_record()
{
    MyBitMap::initConst(); // initialization
    FileManager* fm = new FileManager();
    BufPageManager* bpm = new BufPageManager(fm);
    std::string root_dir = ROOT_DIR;
    std::string addr = root_dir + "student"; // 'student' data table
    std::cout << addr << std::endl;
    fm->createFile(addr.c_str()); // create file
    int fileID;
    fm->openFile(addr.c_str(), fileID); // open file

    // create table meta data
    std::shared_ptr<FieldMetaData> id_meta = std::make_shared<FieldMetaData>(
        FieldType::INT, "id", 0);
    std::shared_ptr<FieldMetaData> name_meta = std::make_shared<FieldMetaData>(
        FieldType::VARCHAR, "name", 1);
    std::vector<std::shared_ptr<FieldMetaData>> meta_datas = {id_meta, name_meta};
    
    // meta data of data table "student"
    TableMetaData table_meta(0, "student", meta_datas);

    // store table meta data into the first page of the data table file
    table_meta.store(fileID, bpm);

    // load table meta data from the first page of the data table file
    TableMetaData table_meta_read;
    table_meta_read.load(fileID, bpm);

    std::cout << table_meta_read.toString() << std::endl;

    Bitmap bitmap(table_meta_read.record_size());
    std::cout << "record size = " << table_meta_read.record_size() << " * 4 bytes" << std::endl;
    // store bitmap into the second page of the data table file
    bitmap.store(fileID, bpm, 1);

    Bitmap bitmap_read;
    bitmap_read.load(fileID, bpm, 1);
    std::cout << bitmap_read.toString() << std::endl;

    std::vector<std::shared_ptr<Field>> fields = {
        std::make_shared<IntField>(1), 
        std::make_shared<VarcharField>("Alice")
    };
    std::vector<std::shared_ptr<Field>> fields2 = {
        std::make_shared<IntField>(2), 
        std::make_shared<VarcharField>("Bob")
    };
    std::vector<std::shared_ptr<Field>> fields3 = {
        std::make_shared<IntField>(3), 
        std::make_shared<VarcharField>("Cindy")
    };
    Record record(fields);
    Record record2(fields2);
    Record record3(fields3);
    int record_size = table_meta.record_size();

    // add record1
    bitmap.addRecord();
    // max_addr: [1, ...]
    int pageID = (bitmap.max_addr - 1) / bitmap.record_num_per_page + 2; // [2, ...]
    int slot = (bitmap.max_addr - 1) % bitmap.record_num_per_page + 1; // [1, 341] -> 1
    record.store(fileID, bpm, pageID, (slot - 1) * record_size);

    // add record2
    bitmap.addRecord();
    int pageID2 = (bitmap.max_addr - 1) / bitmap.record_num_per_page + 2; // 2
    int slot2 = (bitmap.max_addr - 1) % bitmap.record_num_per_page + 1; // 2
    record2.store(fileID, bpm, pageID2, (slot2 - 1) * record_size);

    // add record3
    bitmap.addRecord();
    int pageID3 = (bitmap.max_addr - 1) / bitmap.record_num_per_page + 2; // 2
    int slot3 = (bitmap.max_addr - 1) % bitmap.record_num_per_page + 1; // 3
    record3.store(fileID, bpm, pageID3, (slot3 - 1) * record_size);

    // store record num and offset in the second page of the data table file
    bitmap.store(fileID, bpm, 1);

    bitmap_read.load(fileID, bpm, 1);
    std::cout << bitmap_read.toString() << std::endl;
    bitmap_read.printRecord(fileID, bpm, record_size, table_meta.fields);

    // delete record 2
    int delete_addr = 2; // [1 ... max_addr]
    bitmap.deleteRecord(delete_addr);
    bitmap.store(fileID, bpm, 1);

    bitmap_read.load(fileID, bpm, 1);
    std::cout << bitmap_read.toString() << std::endl;
    bitmap_read.printRecord(fileID, bpm, record_size, table_meta.fields);

    bpm->close();
    fm->closeFile(fileID);

    return 0;
}