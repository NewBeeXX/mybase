#ifndef PF_HASHTABLE_H
#define PF_HASHTABLE_H

#include "pf_internal.h"

struct PF_HashEntry{
    PF_HashEntry *next;
    PF_HashEntry *prev;
    int fd;
    PageNum pageNum;
    int slot; ///该页的在缓冲区中的索引
};

///PF_HashTable
///实现一个支持查找 插入 删除的哈希表
class PF_HashTable{
public:
    PF_HashTable(int numBuckets);
    ~PF_HashTable();
    ///fd和pagenum向slot的映射
    RC Find(int fd,PageNum pageNum,int &slot);
    ///插入
    RC Insert(int fd,PageNum pageNum,int slot);
    RC Delete(int fd,PageNum pageNum);

private:

    int Hash(int fd,PageNum pageNum)const{
        return (fd+pageNum)%numBuckets;
    }
    int numBuckets; ///桶数
    ///数组元素就是指针
    PF_HashEntry **hashTable;

};


#endif // PF_HASHTABLE_H
