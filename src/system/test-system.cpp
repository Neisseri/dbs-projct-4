// #include <iostream>
// #include <cstring>

// #include "basemeta.hpp"
// #include "dbms.hpp"

// #include "../filesystem/bufmanager/BufPageManager.h"
// #include "../filesystem/fileio/FileManager.h"
// #include "../filesystem/utils/pagedef.h"

// int main()
// {
// 	// 这几个变量要全局唯一，在主函数中定义之后把指针传到各接口
//     MyBitMap::initConst();
// 	FileManager* fm = new FileManager();
// 	BufPageManager* bpm = new BufPageManager(fm);

// 	DBMS dbms(fm, bpm, ROOT_DIR);
// 	dbms.createDatabase("Database");
// 	dbms.createDatabase("Network");
// 	dbms.createDatabase("DSA");
// 	dbms.useDatabase("Network");
// 	dbms.deleteDatabase("Database");

// 	dbms.createTable("student");

//     return 0;
// }