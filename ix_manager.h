#ifndef IX_INDEX_MANAGER_H
#define IX_INDEX_MANAGER_H

#include"mybase.h"
#include"rm_rid.h"
#include"pf.h"
#include"ix_indexhandle.h"


class IX_Manager{
public:
    IX_Manager(PF_Manager &pfm);
    ~IX_Manager();

    RC CreateIndex(const char* fileName,int indexNo,
                   AttrType attrType,int attrLength,
                   int pageSize=PF_PAGE_SIZE
                   );
    RC DestroyIndex(const char* fileName,int indexNo);
    RC OpenIndex(const char* fileName,int indexNo,IX_IndexHandle& indexHandle);
    RC CloseIndex(IX_IndexHandle& indexHandle);
private:
    PF_Manager& pfm;
};

#endif // IX_INDEX_MANAGER_H
