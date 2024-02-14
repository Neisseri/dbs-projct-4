#include <cstring>
#include <fstream>
#include <iostream>

#include "antlr4-runtime.h"
#include "grammar/SQLLexer.h"
#include "grammar/SQLBaseVisitor.h"
#include "cxxopts.hpp"
#include "grammar/SQLVisitor.h"
#include "grammar/MyVisitor.hpp"
#include "system/dbms.hpp"
#include "system/constants.h"
#include "grammar/condition.h"

using namespace antlr4;
using namespace grammar;

auto parseLine(std::string sSQL, DBMS &dbms) { // 解析SQL语句sSQL的过程 
    // 转化为输入流 
    ANTLRInputStream sInputStream(sSQL);
    last_inst = inst;
    // 设置Lexer 
    SQLLexer iLexer(&sInputStream);
    CommonTokenStream sTokenStream(&iLexer); 
    // 设置Parser 
    SQLParser iParser(&sTokenStream); 
    auto iTree = iParser.program(); 
    // 构造你的visitor 
    // 检查sSQL里面是否含有ORDER BY DESC
    bool orderBy = false;
    bool desc = false; 
    int limit = -1;
    int offset = -1;
    if (sSQL.find("ORDER BY") != std::string::npos) { 
        orderBy = true;
    }
    if (sSQL.find("DESC") != std::string::npos) { 
        desc = true;
    }
    size_t pos = sSQL.find("LIMIT");
    if(pos != std::string::npos){
        //从LIMIT后面开始读，直到下一个空格
        int nextSpace = sSQL.find(" ", pos+6);
        limit = std::stoi(sSQL.substr(pos+6, nextSpace-pos-6));
        // std::cerr << "limit"<<limit << std::endl;
    }
    pos = sSQL.find("OFFSET");
    if(pos != std::string::npos){
        //从OFFSET后面开始读，直到下一个空格
        int nextSpace = sSQL.find(" ", pos+7);
        offset = std::stoi(sSQL.substr(pos+7, nextSpace-pos-7));
        // std::cerr << "offset"<<offset << std::endl;
    }
    MyVisitor iVisitor(dbms, orderBy, desc, limit, offset);
    // MyVisitor iVisitor(dbms); // 构造函数 要放东西进去
    inst = sSQL; 
    // visitor模式下执行SQL解析过程 
    auto iRes = iVisitor.visit(iTree); 
    return iRes; 
}
 
int main(int argc, char** argv)
{
    cxxopts::Options options("test", "A brief description");
 
    options.add_options()
        ("init","initiate database",cxxopts::value<bool>()->default_value("false"))
        ("b,branch", "enable branch", cxxopts::value<bool>()->default_value("false"))
        ("d,database", "use database", cxxopts::value<std::string>())
        ("f,filePath","import from filePath",cxxopts::value<std::string>())
        ("t,tableName","import tableName",cxxopts::value<std::string>())
        ("r,root","root path",cxxopts::value<std::string>()->default_value(ROOT_DIR))
    ;
    std::string database;
    std::string filePath;
    std::string tableName;
    auto result = options.parse(argc, argv);
    bool branch = result["branch"].as<bool>();
    bool init = result["init"].as<bool>();
    std::string root_path = result["root"].as<std::string>();

    // initialize dbms
    MyBitMap::initConst();
	FileManager* fm = new FileManager();
	BufPageManager* bpm = new BufPageManager(fm);
	DBMS dbms(fm, bpm, ROOT_DIR);
    
    if (init){
        // TODO:如果数据文件夹中已经有了数据则应该删除
        exit(0);
    }
    if (result.count("filePath")){
        if (branch==false){
            std::cout<<"invalid command!"<<std::endl;
            exit(1);
        }
        filePath = result["filePath"].as<std::string>();
        tableName = result["tableName"].as<std::string>();
        // TODO : 导入数据库

        exit(0);
    }
    if (result.count("database")){
        database = result["database"].as<std::string>();
        // TODO: use database


    }
    if(branch){
        // 批处理
        while(true){
            std::string buffer="";
            if(!std::getline(std::cin,buffer)){
                break;
            }
            if(buffer[0]=='-'||buffer.empty()){
                //注释
                continue;
            }
            // std::string res = parseLine(buffer);
            // std::cout<<res;
            //输出@开始的一行
            if (buffer == "exit") {
                bpm->close();
                return 0;
            }
            parseLine(buffer, dbms);
            std::cout << "@" << buffer << std::endl;
        }
    }else{
        //命令行输入
        std::cout<<"Each time enter a SQL command end with ';'"<<std::endl;
        std::cout<<"Enter q to exit"<<std::endl;
        while(true){
            std::cout<<"sql< ";
            std::string buffer;
            std::getline(std::cin,buffer);
            if (buffer=="q"){
                exit(0);
            }
            // std::string res = parseLine(buffer);
            // std::cout<<res;
            parseLine(buffer, dbms);
        }
    }

    bpm->close();
    
    return 0;
}
