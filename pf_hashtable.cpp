#include "pf_internal.h"
#include "pf_hashtable.h"

///这个hash表用挂链来解决冲突

PF_HashTable::PF_HashTable(int _numBuckets) {
    this->numBuckets=_numBuckets;
    ///数组元素是指针
    hashTable=new PF_HashEntry*[numBuckets];
    for(int i=0; i<numBuckets; i++) {
        hashTable[i]=NULL;
    }
}

PF_HashTable::~PF_HashTable() {
    for(int i=0; i<numBuckets; i++) {
        PF_HashEntry* entry=hashTable[i];
        while(entry!=NULL) {
            PF_HashEntry *next=entry->next;
            delete entry;
            entry=next;
        }
    }
    delete [] hashTable;
}


RC PF_HashTable::Find(int fd,PageNum pageNum,int &slot) {
    int bucket=Hash(fd,pageNum);///桶号
    if(bucket<0)return PF_HASHNOTFOUND;
    for(PF_HashEntry *entry=hashTable[bucket];
            entry;
            entry=entry->next
       ) {
        if(entry->fd==fd&&entry->pageNum==pageNum) {
            slot=entry->slot;
            return 0;
        }
    }
    return PF_HASHNOTFOUND;
}

RC PF_HashTable::Insert(int fd,PageNum pageNum,int slot) {
    int bucket=Hash(fd,pageNum);
    PF_HashEntry *entry;
    for(entry=hashTable[bucket];
            entry;
            entry=entry->next
       ) {
        if(entry->fd==fd&&entry->pageNum==pageNum)
            return PF_HASHPAGEEXIST;
    }
    if((entry=new PF_HashEntry)==NULL)return PF_NOMEM;
    entry->fd=fd;
    entry->pageNum=pageNum;
    entry->slot=slot;
    ///前向星的插法
    entry->next=hashTable[bucket];
    entry->prev=NULL;
    if(hashTable[bucket]!=NULL)hashTable[bucket]->prev=entry;
    hashTable[bucket]=entry;
    return 0;
}


RC PF_HashTable::Delete(int fd,PageNum pageNum) {
    int bucket=Hash(fd,pageNum);
    PF_HashEntry *entry;
    for (entry = hashTable[bucket];
            entry != NULL;
            entry = entry->next) {
        if (entry->fd == fd && entry->pageNum == pageNum)
            break;
    }

    if(entry==NULL)return PF_HASHNOTFOUND;
    if (entry == hashTable[bucket])
        hashTable[bucket] = entry->next;
    if (entry->prev != NULL)
        entry->prev->next = entry->next;
    if (entry->next != NULL)
        entry->next->prev = entry->prev;

    delete entry;
    return 0;
}



