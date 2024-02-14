#pragma once
#include <cstring>
#include <iostream>
#include <vector>
#define LINESIZE 50

bool LineItem[40000000];

// ORDER
int getPositionOrder(int key){
    int base = 20 * key;
    for (int i = 0; i < 10; i++)
    {
        if (LineItem[i + base] == false)
        {
            LineItem[i + base] = true;
            return i + base;
        }
    }
    // std::cerr << "No space for index" << std::endl;
}





// LINTITEM
int getPositionCustomer(int key)
{
    int base = 20 * key;
    for (int i = 0; i < 20; i++)
    {
        if (LineItem[i + base] == false)
        {
            LineItem[i + base] = true;
            return i + base;
        }
    }
    // std::cerr << "No space for index" << std::endl;
}

int getPositionDefault(int key){
    int base = 20 * key;
    for (int i = 0; i < 10; i++)
    {
        if (LineItem[i + base] == false)
        {
            LineItem[i + base] = true;
            return i + base;
        }
    }
    // std::cerr << "No space for index" << std::endl;
}
int getPosition(int key, std::string fieldName){
    if (fieldName == "O_ORDERKEY"){
        return getPositionOrder(key);
    }else if(fieldName == "L_ORDERKEY"){
        return getPositionCustomer(key);
    }else {
        return getPositionDefault(key);
    }
}
void saveIndex(FILE *file, int position, int key, int page_id, int slot_id)
{
    // 在file文件的第position行保存索引
    long offset = position * LINESIZE;
    if (fseek(file, offset, SEEK_SET) != 0)
    {
        std::cerr << "fseek error" << std::endl;
    }
    char buf[LINESIZE];
    // 把key, page_id和slot_id使用“ ”拼接起来，然后保存到buf中
    std::string s = std::to_string(key) + " " + std::to_string(page_id) + " " + std::to_string(slot_id);
    strcpy(buf, s.c_str());
    // 把buf写入到file中
    if (fwrite(buf, LINESIZE, 1, file) != 1)
    {
        std::cerr << "fwrite error" << std::endl;
    }
}
void eraseIndex(FILE * file, int from_pos, int to_pos){
    // 删除file文件中第from_pos行到第to_pos行的索引
    for (int i = from_pos; i <= to_pos; i++)
    {
        saveIndex(file, i, -1, -1, -1); 
    }
}
void getIndex(FILE *file, int position, int &key, int &page_id, int &slot_id)
{
    try
    {
        // 从file文件的第position行读取索引
        long offset = position  * LINESIZE;
        if (fseek(file, offset, SEEK_SET) != 0)
        {
            std::cerr << "fseek error" << std::endl;
        }
        char buf[LINESIZE];
        // 从file中读取一行到buf中
        if (fread(buf, LINESIZE, 1, file) != 1)
        {
            std::cerr << "fread error" << std::endl;
        }
        // 把buf中的内容转化为key, page_id和slot_id
        std::string s(buf);
        // 把s按照“ ”分割成三个字符串
        std::vector<std::string> v;
        std::string::size_type pos1, pos2;
        // std::cerr << "here" << std::endl;
        // std::cerr << s[0] << std::endl;
        
        if (s[0] < '0' || s[0] > '9'){
            // std::cerr << "getIndex error" << std::endl;
            slot_id = -1;
            return;
        }
        pos1 = 0;
        pos2 = s.find(" ");
        while (std::string::npos != pos2)
        {
            v.push_back(s.substr(pos1, pos2 - pos1));
            pos1 = pos2 + 1;
            pos2 = s.find(" ", pos1);
        }
        if (pos1 != s.length())
            v.push_back(s.substr(pos1));
        // 把v中的三个字符串转化为key, page_id和slot_id
        key = std::stoi(v[0]);
        page_id = std::stoi(v[1]);
        slot_id = std::stoi(v[2]);
    }
    catch(const std::exception& e)
    {
        // std::cerr << "getIndex error" << std::endl;
        slot_id = -1;
    }
}

FILE *createIndexFile(std::string dataBaseName, std::string tableName, std::string indexName)
{
    // 创建索引文件
    std::string file_path = tableName + "." + indexName + ".index";
    FILE *file = fopen(file_path.c_str(), "w+");
    std::cerr << file_path << std::endl;
    if (file == NULL)
    {
        std::cerr << "fopen error" << std::endl;
    }
    return file;
}
void DeleteIndexFile(std::string dataBaseName, std::string tableName, std::string indexName)
{
    // 删除索引文件
    std::string file_path = dataBaseName + "." +  tableName + "." + indexName + ".index";
    if (remove(file_path.c_str()) != 0)
    {
        std::cerr << "remove error" << std::endl;
    }
}
void closeIndexFile(FILE *file)
{
    // 关闭索引文件
    if (fclose(file) != 0)
    {
        std::cerr << "fclose error" << std::endl;
    }
}

int getInsertPositionCustomer(FILE * f , int key){
    int base = 20 * key;
    for (int i=0;i<20;i++){
        int pos = i + base;
        int k, p, s;
        getIndex(f, pos, k, p, s);
        if (s == -1){
            return pos;
        }
    }
    // std::cerr << "No space for index" << std::endl;
}
// 可以插入的位置
bool inserted[10];
int getInsertPositionOrder(FILE * f , int key){
    int base = 10 * key;
    for (int i=10;i>0;i--){
        if (inserted[i] == false){
            inserted[i] = true;
            return i + base;
        }
    }
    // std::cerr << "No space for index" << std::endl;
}

int getInsertPosition(FILE * f , int key,std::string fieldName){
    if (fieldName == "O_ORDERKEY"){
        return getInsertPositionOrder(f, key);
    }else if(fieldName == "L_ORDERKEY"){
        return getInsertPositionCustomer(f, key);
    }
}