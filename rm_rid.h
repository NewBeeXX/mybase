#ifndef RM_RID_H
#define RM_RID_H


///存储一条记录的id的类
///他应该属于一个文件,不过这个类本身并没有记录它属于哪个文件

#include "mybase.h"
#include "iostream"
#include <cassert>
#include <cstring>

using namespace std;

typedef int PageNum;
typedef int SlotNum;

class RID{
public:
    static const PageNum NULL_PAGE=-1;
    static const SlotNum NULL_SLOT=-1;
    RID():page(NULL_PAGE),slot(NULL_SLOT){}
    RID(PageNum pageNum,SlotNum,slotNum):page(pageNum),slot(slotNum){}

    ~RID(){}
    RC GetPageNum(PageNum &pageNum)const{
        pageNum=page;return 0;
    }
    RC GetSlotNum(SlotNum &slotNum)const{
        slotNum=slot;return 0;
    }
    PageNum Page()const{
        return page;
    }
    SlotNum Slot()const{
        return slot;
    }

    bool operator==(const RID& rhs)const{
        PageNum p;
        SlotNum s;
        rhs.GetPageNum(p),rhs.GetSlotNum(s);
        return p==page&&s==slot;
    }

private:
    PageNum page;
    SlotNum slot;
};

ostream& operator<<(ostream& os,const RID& r);


#endif // RM_RID_H
