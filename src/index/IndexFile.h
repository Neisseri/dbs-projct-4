#include <memory>
#include <cstring>
#include <math.h>
#include <queue>
#include "../filesystem/utils/pagedef.h"
#include "../filesystem/bufmanager/BufPageManager.h"
#include "IndexPage.h"
// #define MAX_CHILD_NUM MAX_RECORD_NUM
// #define MIN_CHILD_NUM ((MAX_CHILD_NUM + 1) >> 1)
#define MAX_CHILD_NUM 4
#define MIN_CHILD_NUM 2
#define RANGE_SEARCH_MAX 10000
class IndexFile 
{
public:
    IndexMetaPage *meta;
    int file_id;
    /*
    create_new=True:创建一个全新的索引文件（B+树）
    create_new=False:打开一个已经存在的索引文件
    */
    IndexFile(char *file_name, FileManager *fm, BufPageManager *bpm, bool create_new)
    {
        if (create_new)
        {
            printf("my fm addr %d\n", fm);
            if (!fm->createFile(file_name))
            {
                cout << "create file failed" << endl;
                return;
            }
            int file_id;
            if (!fm->openFile(file_name, file_id))
            {
                cout << "open file failed" << endl;
                return;
            }
            this->file_id = file_id;
            // 把metapage加载到缓存，并初始化
            int meta_index;
            BufType b = bpm->fetchPage(file_id, 0, meta_index);
            meta = reinterpret_cast<IndexMetaPage *>(b);
            meta->root_page_id = 1;
            memset(meta->bitmap, 0, sizeof(meta->bitmap));

            // 把rootpage加载到缓存，并初始化
            int root_index;
            b = bpm->fetchPage(file_id, 1, root_index);

            IndexPage *root_page = reinterpret_cast<IndexPage *>(b);
            root_page->page_id = 1; // 初始化的时候被定为1，但之后可能就不是1了
            root_page->used_slot_num = 0;
            root_page->is_leaf = false;
            root_page->prev_page_id = -1;
            root_page->next_page_id = -1;
            root_page->parent_page_id = -1;
            meta->set_page_id_used(0);
            meta->set_page_id_used(1);
            // 把metapage和rootpage mark dirty
            bpm->markDirty(meta_index);
            bpm->markDirty(root_index);
        }
        else
        {
            int file_id;
            if (!fm->openFile(file_name, file_id))
            {
                cout << "open file failed" << endl;
                return;
            }
            this->file_id = file_id;
            // 把metapage加载到缓存
            int meta_buf_index;
            BufType meta_buf = bpm->getPage(file_id, 0, meta_buf_index);
            meta = reinterpret_cast<IndexMetaPage *>(meta_buf);
            bpm->access(meta_buf_index);
            printf("finish load\n");
        }
    }

    /*
    查找第一个大于等于key的记录，如果没有大于等于key的记录，则返回最大的一个记录；
    如果是空树，返回nullptr
    */
    IndexPage *search(int key, BufPageManager *bpm, int &record_index)
    {   
        printf("in search\n");
        int page_id = meta->root_page_id;
        printf("root page id:%d\n", page_id);
        int root_index;
        IndexPage *root_page = reinterpret_cast<IndexPage *>(bpm->getPage(this->file_id, page_id, root_index));
        bpm->access(root_index);
        if (root_page->is_empty())
        {
            printf("root page is empty\n");
            return nullptr;
        }
        while (true)
        {
            int page_index;
            IndexPage *page = reinterpret_cast<IndexPage *>(bpm->getPage(this->file_id, page_id, page_index));
            bpm->access(page_index);
            IndexRecord *record = page->search(key, record_index);
            if (page->is_leaf)
            {
                return page;
            }
            page_id = record->page_id;
        }
    }
    /*
    把curpage分裂成两个页面，并且更新父节点的记录
    */
    void solve_overflow(IndexPage *cur_page, IndexPage *parent_page, BufPageManager *bpm)
    {
        int lc_page_id = meta->find_unused_page_id();
        int lc_child_num = int((MAX_CHILD_NUM + 2) / 2);
        int rc_child_num = MAX_CHILD_NUM + 1 - lc_child_num;
        meta->set_page_id_used(lc_page_id);
        int lc_page_buf_index;
        // 把lc_page加载到缓存，并初始化
        BufType lc_page_buf = bpm->getPage(this->file_id, lc_page_id, lc_page_buf_index);
        bpm->access(lc_page_buf_index);
        IndexPage *left_child_page = reinterpret_cast<IndexPage *>(lc_page_buf);
        left_child_page->page_id = lc_page_id;
        left_child_page->used_slot_num = lc_child_num;
        left_child_page->is_leaf = cur_page->is_leaf;
        left_child_page->prev_page_id = cur_page->prev_page_id;
        left_child_page->next_page_id = cur_page->page_id;
        left_child_page->parent_page_id = cur_page->parent_page_id;
        memcpy(left_child_page->records, cur_page->records, sizeof(IndexRecord) * lc_child_num);
        // 修改lc孩子的父节点信息
        for (int i = 0; i < lc_child_num; i++)
        {
            int child_page_id = left_child_page->records[i].page_id;
            int child_buf_index;
            IndexPage *child_page = reinterpret_cast<IndexPage *>(bpm->getPage(this->file_id, child_page_id, child_buf_index));
            bpm->access(child_buf_index);
            child_page->parent_page_id = lc_page_id;
            bpm->markDirty(child_buf_index);
        }
        bpm->markDirty(lc_page_buf_index);
        // 修改prev_page的next_page_id
        if (left_child_page->prev_page_id != -1)
        {
            int prev_page_buf_index;
            IndexPage *prev_page = reinterpret_cast<IndexPage *>(bpm->getPage(this->file_id, left_child_page->prev_page_id, prev_page_buf_index));
            bpm->access(prev_page_buf_index);
            prev_page->next_page_id = lc_page_id;
            bpm->markDirty(prev_page_buf_index);
        }
        // 修改cur_page的信息
        cur_page->used_slot_num = rc_child_num;
        cur_page->prev_page_id = lc_page_id;
        memcpy(cur_page->records, cur_page->records + lc_child_num, sizeof(IndexRecord) * rc_child_num);
        bpm->markDirty(cur_page->page_id);
        if (parent_page == nullptr)
        {
            int new_root_page_id = meta->find_unused_page_id();
            meta->set_page_id_used(new_root_page_id);
            int new_root_page_buf_index;
            // 把new_root_page加载到缓存，并初始化
            BufType new_root_page_buf = bpm->getPage(this->file_id, new_root_page_id, new_root_page_buf_index);
            bpm->access(new_root_page_buf_index);
            IndexPage *new_root_page = reinterpret_cast<IndexPage *>(new_root_page_buf);
            new_root_page->page_id = new_root_page_id;
            new_root_page->used_slot_num = 2;
            new_root_page->is_leaf = false;
            new_root_page->prev_page_id = -1;
            new_root_page->next_page_id = -1;
            new_root_page->parent_page_id = -1;
            IndexRecord *new_root_page_records = new_root_page->records;
            new_root_page_records[0].key = left_child_page->max_key();
            new_root_page_records[0].page_id = left_child_page->page_id;
            new_root_page_records[1].key = cur_page->max_key();
            new_root_page_records[1].page_id = cur_page->page_id;
            meta->root_page_id = new_root_page_id;
            // 修改lc和rc的父节点信息
            left_child_page->parent_page_id = new_root_page_id;
            cur_page->parent_page_id = new_root_page_id;
            bpm->markDirty(new_root_page_buf_index);
            return;
        }
        // 修改父节点的信息,插入lc
        int lc_record_index;
        IndexRecord *parent_record = parent_page->search(left_child_page->max_key(), lc_record_index);

        IndexRecord lc_record = {0, lc_page_id, 0, left_child_page->max_key()};
        parent_page->insert(lc_record, lc_record_index);
        bpm->markDirty(parent_page->page_id);
    }
    void insert(int key, int file_id, int page_id, int slot_id, BufPageManager *bpm)
    {   
        printf("before search\n");
        int record_index;
        IndexPage *leaf_page = search(key, bpm, record_index); // 查找到的大于等于key的第一个记录
        printf("after search\n");
        if (leaf_page == nullptr)
        {
            // 空树
            // 在root中插入一个record
            printf("no leaf tree ,emoty tree\n");
            int buf_root_page_index;

            IndexPage *root_page = reinterpret_cast<IndexPage *>(bpm->getPage(this->file_id, meta->root_page_id, buf_root_page_index));

            bpm->access(buf_root_page_index);
            root_page->records[0].key = key;

            root_page->used_slot_num = 1;

            // 加载一个叶子节点，并插入一个record
            int leaf_page_id = meta->find_unused_page_id();
            root_page->records[0].page_id = leaf_page_id;
            meta->set_page_id_used(leaf_page_id);
            int buf_leaf_page_index;
            IndexPage *leaf_page = reinterpret_cast<IndexPage *>(bpm->getPage(this->file_id, leaf_page_id, buf_leaf_page_index));
            bpm->access(buf_leaf_page_index);
            leaf_page->page_id = leaf_page_id;
            leaf_page->used_slot_num = 1;
            leaf_page->is_leaf = true;
            leaf_page->prev_page_id = -1;
            leaf_page->next_page_id = -1;
            leaf_page->parent_page_id = meta->root_page_id;
            leaf_page->records[0].key = key;
            leaf_page->records[0].page_id = page_id;
            leaf_page->records[0].slot_id = slot_id;
            leaf_page->records[0].file_id = file_id;
            bpm->markDirty(buf_root_page_index);
            bpm->markDirty(buf_leaf_page_index);
            return;
        }
        IndexRecord *next_record = leaf_page->records + record_index;

        if (next_record->key == key && next_record->page_id == page_id && next_record->slot_id == slot_id && next_record->file_id == file_id)
        {
            // 已经存在
            return;
        }
        bool update_parent = false;

        // 在叶子结点的record_index处插入记录
        IndexRecord record = {file_id, page_id, slot_id, key};
        if (next_record->key < key)
        {
            record_index++;
        }
        leaf_page->insert(record, record_index);
        printf("after insert in leaf\n");
        // 把叶子节点标记为dirty
        int leaf_buf_index;
        bpm->getPage(this->file_id, leaf_page->page_id, leaf_buf_index);
        bpm->markDirty(leaf_buf_index);
        // 如果叶结点最大关键字改变，可能需要逐级更新父结点的相应关键字。(似乎是只有插入全局最大关键字时才需要更新父结点的关键字)
        if (record_index == leaf_page->used_slot_num - 1)
        {
            update_parent = true;
        }
        if (update_parent)
        {
            int parent_page_id = leaf_page->parent_page_id;
            while (parent_page_id != -1)
            {
                int parent_buf_index;
                IndexPage *parent_page = reinterpret_cast<IndexPage *>(bpm->getPage(this->file_id, parent_page_id, parent_buf_index));
                bpm->access(parent_buf_index);
                if (parent_page->max_key() < key)
                {
                    // 更新
                    parent_page->records[parent_page->used_slot_num - 1].key = key;
                    bpm->markDirty(parent_buf_index);
                }
                else
                {
                    break;
                }
                parent_page_id = parent_page->parent_page_id;
            }
            printf("finish update parent\n");
        }

        // 如果某结点发生上溢，则将其分裂为两个分支数相近的结点，都作为该结点父结点的子结点，父结点也要处理可能的上溢
        IndexPage *cur_page = leaf_page;
        while (cur_page->used_slot_num > MAX_CHILD_NUM)
        {
            IndexPage *parent_page = cur_page->parent_page_id == -1 ? nullptr : reinterpret_cast<IndexPage *>(bpm->getPage(this->file_id, cur_page->parent_page_id, record_index));
            solve_overflow(cur_page, parent_page, bpm);
            cur_page = cur_page->parent_page_id == -1 ? nullptr : reinterpret_cast<IndexPage *>(bpm->getPage(this->file_id, cur_page->parent_page_id, record_index));
        }
    }
    void solve_underflow(IndexPage *cur_page, IndexPage *parent_page, BufPageManager *bpm)
    {
        // 如果某结点发生下溢，且其为根结点，其子结点在某些情况下可能成为新的根结点；这句话不是很懂qwq
        if (parent_page == nullptr)
        {
            // 根节点特殊处理
            if (cur_page->used_slot_num > 1)
            {
                return;
            }
            else
            {
                // 根节点只有一个孩子结点，如果孩子节点不是叶子节点，那么将其孩子结点作为新的根结点
                if (!cur_page->is_leaf)
                {
                    int new_root_page_id = cur_page->records[0].page_id;
                    meta->root_page_id = new_root_page_id;
                    int new_root_buf_index;
                    IndexPage *new_root_page = reinterpret_cast<IndexPage *>(bpm->getPage(this->file_id, new_root_page_id, new_root_buf_index));
                    bpm->access(new_root_buf_index);
                    new_root_page->parent_page_id = -1;
                    bpm->markDirty(new_root_buf_index);
                    // 删除旧的根结点
                    meta->set_page_id_unused(cur_page->page_id);
                    int old_root_buf_index;
                    bpm->fetchPage(file_id, cur_page->page_id, old_root_buf_index);
                    bpm->release(old_root_buf_index);
                    return;
                }
                else
                {
                    return;
                }
            }
        }
        // 如果其左/右兄弟有多余的孩子结点，可以过继给它
        int ls_page_id = cur_page->prev_page_id;
        int ls_buf_index;
        IndexPage *ls_page=nullptr;
        if(ls_page_id!=-1){
            ls_page = reinterpret_cast<IndexPage *>(bpm->getPage(this->file_id, ls_page_id, ls_buf_index));
            bpm->access(ls_buf_index);
            //判断当前节点和左兄弟的父亲是否相同
            if(ls_page->parent_page_id!=cur_page->parent_page_id){
                ls_page=nullptr;
            }
        }
        int rs_page_id = cur_page->next_page_id;
        int rs_buf_index;
        IndexPage *rs_page=nullptr;
        if(rs_page_id!=-1){
            rs_page = reinterpret_cast<IndexPage *>(bpm->getPage(this->file_id, rs_page_id, rs_buf_index));
            bpm->access(rs_buf_index);
            //判断当前节点和右兄弟的父亲是否相同
            if(rs_page->parent_page_id!=cur_page->parent_page_id){
                rs_page=nullptr;
            }
        }
        if (ls_page!=nullptr)
        {
            if (ls_page->used_slot_num > MIN_CHILD_NUM)
            {
                // 过继
                printf("borrow from ls\n");
                int record_index;
                IndexRecord *parent_record = parent_page->search(ls_page->max_key(), record_index);
                IndexRecord *ls_max_record = ls_page->records + ls_page->used_slot_num - 1;
                IndexRecord *cur_min_record = cur_page->records;
                memcpy(cur_min_record + 1, cur_min_record, sizeof(IndexRecord) * cur_page->used_slot_num);
                memcpy(cur_min_record, ls_max_record, sizeof(IndexRecord));
                ls_page->used_slot_num--;
                cur_page->used_slot_num++;
                //修改过继来的结点的父节点信息
                int child_page_id = ls_max_record->page_id;
                int child_buf_index;
                IndexPage *child_page = reinterpret_cast<IndexPage *>(bpm->getPage(this->file_id, child_page_id, child_buf_index));
                bpm->access(child_buf_index);
                child_page->parent_page_id = cur_page->page_id;
                bpm->markDirty(child_buf_index);
                // 修改父节点record
                parent_record->key = ls_page->max_key();
                // 把ls_page,parent_page,cur_page标记为dirty
                int parent_buf_index;
                bpm->getPage(this->file_id, parent_page->page_id, parent_buf_index);
                bpm->access(parent_buf_index);
                int cur_buf_index;
                bpm->getPage(this->file_id, cur_page->page_id, cur_buf_index);
                bpm->access(cur_buf_index);
                bpm->markDirty(ls_buf_index);
                bpm->markDirty(parent_buf_index);
                bpm->markDirty(cur_buf_index);
                return;
            }
        }
        if (rs_page!=nullptr)
        {
            if (rs_page->used_slot_num > MIN_CHILD_NUM)
            {
                // 过继
                int record_index;
                printf("borrow from rs\n");
                IndexRecord *parent_record = parent_page->search(cur_page->max_key(), record_index);
                IndexRecord *rs_min_record = rs_page->records;
                IndexRecord *cur_max_record = cur_page->records + cur_page->used_slot_num;
                memcpy(cur_max_record, rs_min_record, sizeof(IndexRecord));
                memcpy(rs_min_record, rs_min_record + 1, sizeof(IndexRecord) * (rs_page->used_slot_num - 1));
                rs_page->used_slot_num--;
                cur_page->used_slot_num++;
                //修改过继来的结点的父节点信息
                int child_page_id = rs_min_record->page_id;
                int child_buf_index;
                IndexPage *child_page = reinterpret_cast<IndexPage *>(bpm->getPage(this->file_id, child_page_id, child_buf_index));
                bpm->access(child_buf_index);
                child_page->parent_page_id = cur_page->page_id;
                bpm->markDirty(child_buf_index);
                // 修改父节点record
                parent_record->key = cur_page->max_key();
                // 把rs_page,parent_page,cur_page标记为dirty
                int parent_buf_index;
                bpm->getPage(this->file_id, parent_page->page_id, parent_buf_index);
                bpm->access(parent_buf_index);
                int cur_buf_index;
                bpm->getPage(this->file_id, cur_page->page_id, cur_buf_index);
                bpm->access(cur_buf_index);
                bpm->markDirty(rs_buf_index);
                bpm->markDirty(parent_buf_index);
                bpm->markDirty(cur_buf_index);
                return;
            }
        }
        // 否则可以将其与左/右兄弟合并成一个结点，删除或更新父结点的关键字
        if (ls_page!=nullptr)
        {   
            printf("merge with ls\n ");
            int ls_record_index;
            int cur_record_index;
            int cur_buf_index;
            int parent_buf_index;
            bpm->getPage(this->file_id, cur_page->page_id, cur_buf_index);
            bpm->getPage(this->file_id, parent_page->page_id, parent_buf_index);
            bpm->access(ls_buf_index);
            IndexRecord *parent_ls_record = parent_page->search(ls_page->max_key(), ls_record_index);
            IndexRecord *parent_cur_record = parent_page->search(cur_page->max_key(), cur_record_index);
            // 合并，把ls中的记录全部移到cur_page中
            memcpy(cur_page->records + ls_page->used_slot_num, cur_page->records, sizeof(IndexRecord) * cur_page->used_slot_num);
            memcpy(cur_page->records, ls_page->records, sizeof(IndexRecord) * ls_page->used_slot_num);
            //修改ls_page中的孩子的父节点信息
            for (int i = 0; i < ls_page->used_slot_num; i++)
            {
                int child_page_id = ls_page->records[i].page_id;
                int child_buf_index;
                IndexPage *child_page = reinterpret_cast<IndexPage *>(bpm->getPage(this->file_id, child_page_id, child_buf_index));
                bpm->access(child_buf_index);
                child_page->parent_page_id = cur_page->page_id;
                bpm->markDirty(child_buf_index);
            }
            cur_page->used_slot_num += ls_page->used_slot_num;
            // 删除父节点的记录
            parent_page->remove(ls_record_index);
            // 删除ls_page
            meta->set_page_id_unused(ls_page_id);
            // 修改cur_page的prev_page_id，修改左兄弟prev_page的next_page_id
            cur_page->prev_page_id = ls_page->prev_page_id;
            if (ls_page->prev_page_id != -1)
            {
                int prev_buf_index;
                IndexPage *prev_page = reinterpret_cast<IndexPage *>(bpm->getPage(this->file_id, ls_page->prev_page_id, prev_buf_index));
                bpm->access(prev_buf_index);
                prev_page->next_page_id = cur_page->page_id;
                bpm->markDirty(prev_buf_index);
            }
            bpm->release(ls_buf_index);
            // 把cur_page,parent_page标记为dirty
            bpm->markDirty(cur_buf_index);
            bpm->markDirty(parent_buf_index);

            return;
        }
        if (rs_page!=nullptr)
        {
            printf("merge with rs\n");
            int rs_record_index;
            int cur_buf_index;
            int cur_record_index;
            int parent_buf_index;
            bpm->getPage(this->file_id, cur_page->page_id, cur_buf_index);
            bpm->getPage(this->file_id, parent_page->page_id, parent_buf_index);
            IndexRecord *parent_rs_record = parent_page->search(rs_page->max_key(), rs_record_index);
            IndexRecord *parent_cur_record = parent_page->search(cur_page->max_key(), cur_record_index);
            // 合并，把rs_page中的记录全部移到cur_page中
            memcpy(cur_page->records + cur_page->used_slot_num, rs_page->records, sizeof(IndexRecord) * rs_page->used_slot_num);
            cur_page->used_slot_num += rs_page->used_slot_num;
            //修改rs_page中的孩子的父节点信息

            for (int i = 0; i < rs_page->used_slot_num; i++)
            {
                int child_page_id = rs_page->records[i].page_id;
                int child_buf_index;
                IndexPage *child_page = reinterpret_cast<IndexPage *>(bpm->getPage(this->file_id, child_page_id, child_buf_index));
                bpm->access(child_buf_index);
                child_page->parent_page_id = cur_page->page_id;
                bpm->markDirty(child_buf_index);
            }
            // 修改cur_page的next_page_id，修改右兄弟next_page的prev_page_id
            cur_page->next_page_id = rs_page->next_page_id;
            if (rs_page->next_page_id != -1)
            {
                int next_buf_index;
                IndexPage *next_page = reinterpret_cast<IndexPage *>(bpm->getPage(this->file_id, rs_page->next_page_id, next_buf_index));
                bpm->access(next_buf_index);
                next_page->prev_page_id = cur_page->page_id;
                bpm->markDirty(next_buf_index);
            }
            // 更改父节点的记录
            parent_page->remove(rs_record_index);
            parent_cur_record->key = rs_page->max_key();
            // 把cur_page,parent_page标记为dirty
            bpm->markDirty(cur_buf_index);
            bpm->markDirty(parent_buf_index);
            // 删除rs_page
            meta->set_page_id_unused(rs_page_id);
            bpm->release(rs_buf_index);
            return;
        }
        // 没有左右兄弟，说明其父结点是根结点，且只有一个孩子结点
        if (parent_page->parent_page_id == -1)
        {
            // 如果当前节点不是叶子节点，那么把当前节点作为新的根节点
            if (!cur_page->is_leaf)
            {
                meta->root_page_id = cur_page->page_id;
                cur_page->parent_page_id = -1;
                // 删除旧的根结点
                meta->set_page_id_unused(parent_page->page_id);
                int old_root_buf_index;
                bpm->fetchPage(file_id, parent_page->page_id, old_root_buf_index);
                bpm->release(old_root_buf_index);
                bpm->markDirty(cur_page->page_id);
                return;
            }
            else
            {
                // 当前节点是叶子节点，如果当前节点已经空了，那么直接删除当前节点
                if (cur_page->is_empty())
                {
                    meta->set_page_id_unused(cur_page->page_id);
                    int cur_buf_index;
                    bpm->fetchPage(file_id, cur_page->page_id, cur_buf_index);
                    bpm->release(cur_buf_index);
                    return;
                }
                else
                {
                    // 当前节点不是空的，那么不做任何处理
                    return;
                }
            }
            return;
        }
        else
        {
            cout << "error" << endl;
        }
    }
    /*
    删除所有key等于给定key的记录，返回是否存在给定的key
    */
    bool deleteRecords(int key,BufPageManager * bpm){
        bool find = false;
        while(deleteRecord(key,bpm)){
            printTree(bpm);
            find = true;
        }
        return find;
    }
    bool deleteRecord(int key, BufPageManager *bpm)
    {
        int record_index;
        IndexPage *leaf_page = search(key, bpm, record_index); // 查找到的大于等于key的第一个记录
        if (leaf_page == nullptr)
        {
            // 空树
            printf("empty tree,return false\n");
            return false;
        }
        if (leaf_page->records[record_index].key != key)
        {
            // 不存在
            printf("not exist,return false\n");
            return false;
        }
        // 删除记录
        leaf_page->remove(record_index);
        bool update_parent = false;
        if (key > leaf_page->max_key())
        {
            update_parent = true;
        }
        // 把叶子节点标记为dirty
        int leaf_buf_index;
        bpm->getPage(this->file_id, leaf_page->page_id, leaf_buf_index);
        bpm->markDirty(leaf_buf_index);

        int new_max_key = leaf_page->max_key();
        if (update_parent)
        {
            printf("update parent\n ");
            // 如果叶结点最大关键字改变，可能需要逐级更新父结点的关键字。
            int page_id = leaf_page->page_id;
            int parent_page_id = leaf_page->parent_page_id;
            while (parent_page_id != -1)
            {
                int parent_buf_index;
                IndexPage *parent_page = reinterpret_cast<IndexPage *>(bpm->getPage(this->file_id, parent_page_id, parent_buf_index));
                bpm->access(parent_buf_index);
                IndexRecord *parent_record = parent_page->search(key, record_index);
                parent_page->records[record_index].key = new_max_key;
                bpm->markDirty(parent_buf_index);
                if (record_index < parent_page->used_slot_num - 1)
                {
                    break;
                }
                page_id = parent_page_id;
                parent_page_id = parent_page->parent_page_id;
            }
        }
        IndexPage *cur_page = leaf_page;
        while (cur_page->used_slot_num < MIN_CHILD_NUM)
        {
            IndexPage *parent_page = cur_page->parent_page_id == -1 ? nullptr : reinterpret_cast<IndexPage *>(bpm->getPage(file_id, cur_page->parent_page_id, record_index));
            bpm->access(record_index);
            solve_underflow(cur_page, parent_page, bpm);
            if (cur_page->parent_page_id == -1)
            {
                break;
            }
            else
            {
                cur_page = reinterpret_cast<IndexPage *>(bpm->getPage(this->file_id, cur_page->parent_page_id, record_index));
            }
        }
        return true;
    }
    IndexPage* getNextRecord(IndexPage * leaf_page, int cur_record_index,int & next_record_index,BufPageManager * bpm){
        if(cur_record_index<leaf_page->used_slot_num-1){
            next_record_index = cur_record_index+1;
            return leaf_page;
        }
        else{
            if(leaf_page->next_page_id==-1){
                next_record_index = -1;
                return nullptr;
            }
            else{
                int next_buf_index;
                IndexPage *next_page = reinterpret_cast<IndexPage *>(bpm->getPage(this->file_id, leaf_page->next_page_id, next_buf_index));
                bpm->access(next_buf_index);
                next_record_index = 0;
                return next_page;
            }
        }
    }
    /*
    查找key在[a,b]区间内的记录，返回一个记录数组，record_num保存记录的数量
    */
    IndexRecord* rangeSearch(int key_a, int key_b,BufPageManager * bpm,int &record_num){
        //判断b>=a
        if(key_b<key_a){
            record_num = 0;
            return nullptr;
        }
        //先查找a
        int record_index_a;
        IndexPage *leaf_page_a = search(key_a, bpm, record_index_a); // 查找到的大于等于key的第一个记录
        if (leaf_page_a == nullptr || leaf_page_a->records[record_index_a].key > key_b)
        {
            //查找失败
            record_num = 0;
            return nullptr;
        }
        //沿着叶子节点的next_page_id查找，直到找到一个大于b的叶子节点
        IndexRecord * records = new IndexRecord[RANGE_SEARCH_MAX];
        record_num = 0;
        //先把第一个记录加入
        records[0].key = leaf_page_a->records[record_index_a].key;
        records[0].page_id = leaf_page_a->records[record_index_a].page_id;
        records[0].slot_id = leaf_page_a->records[record_index_a].slot_id;
        records[0].file_id = leaf_page_a->records[record_index_a].file_id;
        record_num++;
        int cur_record_index = record_index_a;
        int next_record_index;
        IndexPage * leaf_page = leaf_page_a;
        while(1){
            IndexPage * next_page = getNextRecord(leaf_page,cur_record_index,next_record_index,bpm);
            if(next_page==nullptr){
                return records;

            }else{
                IndexRecord * next_record = next_page->records+next_record_index;
                if(next_record->key>key_b){
                    return records;
                }
                records[record_num].key = next_record->key;
                records[record_num].page_id = next_record->page_id;
                records[record_num].slot_id = next_record->slot_id;
                records[record_num].file_id = next_record->file_id;
                record_num++;
                cur_record_index = next_record_index;
                leaf_page = next_page;
                
            }
        }


    }
    /*
    先打印meta
    然后从根节点开始，层次遍历打印
    */
    void printTree(BufPageManager *bpm)
    {
        cout << "########################################" << endl;
        cout << "meta:" << endl;
        cout << "root_page_id:" << meta->root_page_id << endl;
        cout << endl;
        // 然后从根节点开始，层次遍历打印
        int buf_index;
        IndexPage *root_page = reinterpret_cast<IndexPage *>(bpm->getPage(this->file_id, meta->root_page_id, buf_index));
        bpm->access(buf_index);
        queue<IndexPage *> q;
        q.push(root_page);
        while (!q.empty())
        {
            IndexPage *cur_page = q.front();
            q.pop();
            cout << "-----------------node begin-------------------" << endl;
            cout << "page_id:" << cur_page->page_id << endl;
            cout << "used_slot_num:" << cur_page->used_slot_num << endl;
            cout << "is_leaf:" << cur_page->is_leaf << endl;
            cout << "prev_page_id:" << cur_page->prev_page_id << endl;
            cout << "next_page_id:" << cur_page->next_page_id << endl;
            cout << "parent_page_id:" << cur_page->parent_page_id << endl;
            cout << "records:" << endl;
            for (int i = 0; i < cur_page->used_slot_num; i++)
            {
                cout << "key:" << cur_page->records[i].key << " page_id:" << cur_page->records[i].page_id << " slot_id:" << cur_page->records[i].slot_id << " file_id:" << cur_page->records[i].file_id << endl;
            }
            cout << "-----------------node end-------------------" << endl;
            if (!cur_page->is_leaf)
            {
                for (int i = 0; i < cur_page->used_slot_num; i++)
                {
                    int buf_index;
                    IndexPage *child_page = reinterpret_cast<IndexPage *>(bpm->getPage(this->file_id, cur_page->records[i].page_id, buf_index));
                    bpm->access(buf_index);
                    q.push(child_page);
                }
            }
        }
        cout << "########################################" << endl;
    }
};