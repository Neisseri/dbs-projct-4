#include <memory>
#include <cstring>
#include <math.h>
#include "../filesystem/utils/pagedef.h"
#include "../filesystem/bufmanager/BufPageManager.h"

#define MAX_RECORD_NUM int((PAGE_SIZE - 6*sizeof(int)) / sizeof(IndexRecord)) //slot最大数量
/*
对于叶子节点，file_id,page_id,slot_id记录的是数据页的信息
对于内部节点，file_id,page_id记录的是孩子节点（索引页）的信息，slot_id无意义
*/
struct IndexRecord{
    int file_id; 
    int page_id; //指向的是数据页，有可能还需要保存一个file_id
    int slot_id;
    int key;
};
/*
保存索引文件的元信息，包括索引文件的id，索引文件的根节点的页号，一个标记每个page_id是否被占用的bitmap
这个页面是0号页面
*/
#define INDEXFILE_BITMAP_SIZE 64
#define MAX_INDEXPAGE_NUM (INDEXFILE_BITMAP_SIZE * 8) //一个索引文件最多有多少个索引页

class IndexMetaPage{
public:
    int root_page_id;
    char bitmap[INDEXFILE_BITMAP_SIZE];
    int find_unused_page_id(){
        for(int i = 0;i < INDEXFILE_BITMAP_SIZE;i++){
            if(bitmap[i] != 0xff){
                for(int j = 0;j < 8;j++){
                    if((bitmap[i] & (1 << j)) == 0){
                        return i * 8 + j;
                    }
                }
            }
        }
        cout<<"cannot find unused page id"<<endl;
        return -1;
    }
    void set_page_id_used(int page_id){
        bitmap[page_id / 8] |= (1 << (page_id % 8));
    }
    void set_page_id_unused(int page_id){
        bitmap[page_id / 8] &= ~(1 << (page_id % 8));
    }
};
class IndexPage{
public:
    int page_id;
    int used_slot_num;//正在被占用的slot数量
    bool is_leaf;
    int prev_page_id;
    int next_page_id;
    int parent_page_id;
    IndexRecord records[MAX_RECORD_NUM];
public:
    /*
    判断是否为空
    */
    bool is_empty(){
        return used_slot_num == 0;
    }
    /*
    判断是否已满
    */
    bool is_full(){
        return used_slot_num == MAX_RECORD_NUM;
    }
    int max_key(){
        if (used_slot_num == 0)
        {
            cout<<"empty index page, no max key"<<endl;
            return -1;
        }
        return records[used_slot_num - 1].key;
    }
    /*
    使用二分查找，找到第一个大于等于key的记录，如果所有记录的key都小于key，则返回最后一个记录
    */
    IndexRecord* search(int key,int& index){
        
        int l = 0, r = used_slot_num - 1;
        while(l < r){
            int mid = (l + r) >> 1;
            if(records[mid].key >= key) r = mid;
            else l = mid + 1;
        }
        index = l;
        return &records[l];
    }
    /*
    调用前需确保仍有空间插入一条记录,在index处插入
    */
    void insert(IndexRecord record,int & index){
        //找到第一个大于等于record.key的记录，将其后的所有记录后移一位，然后将record插入
        IndexRecord* p = search(record.key,index);
        if(p->key<record.key){
            index++;
            //把record插入到最后
            records[used_slot_num] = record;
            used_slot_num++;
            return;
        }
        memcpy(records + index + 1,records + index,sizeof(IndexRecord) * (used_slot_num - index));
        records[index] = record;
        used_slot_num++;
    }
    /*
    删除第index条记录
    */
    void remove(int index){
        memcpy(records + index,records + index + 1,sizeof(IndexRecord) * (used_slot_num - index - 1));
        used_slot_num--;
    }
};
