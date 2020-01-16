#ifndef PF_INTERNAL_H
#define PF_INTERNAL_H

#include<cstdlib>
#include<cstring>
#include"pf.h"

///缓冲池页面数
const int PF_BUFFER_SIZE=40;
///hash table 大小
const int PF_HASH_TBL_SIZE=20;

#define CREATION_MASK 0600 ///读写权限
#define PF_PAGE_LIST_END -1
#define PF_PAGE_USED -2

#ifndef L_SET
#define L_SET 0
#endif

///放在页面起始位置的东西
///分页头
struct PF_PageHdr{
    int nextFree; ///下一个可用空间
    // nextFree can be any of these values:
    //  - the number of the next free page
    //  - PF_PAGE_LIST_END if this is last free page
    //  - PF_PAGE_USED if the page is not free
};

///分页总大小
const int PF_FILE_HDR_SIZE=PF_PAGE_SIZE+sizeof(PF_PageHdr);





#endif
