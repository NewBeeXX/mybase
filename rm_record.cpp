#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include "rm.h"
#include "rm_rid.h"

using namespace std;



RM_Record::RM_Record():recordSize(-1),data(NULL),rid(-1,-1){
}

RM_Record::~RM_Record() {
    if(data)delete [] data;
}


///给该record对象重新设置一个size长度不变的新data
///或者该对象未初始化可以随意长度
RC RM_Record::Set(char* pData, int size, RID rid_) {
    ///需要该
    if(recordSize!=-1&&(size!=recordSize)){
        return RM_RECSIZEMISMATCH;
    }
    recordSize=size;
    this->rid=rid_;
    if(data==NULL)data=new char[recordSize];
    memcpy(data,pData,size);
    return 0;
}

RC RM_Record::GetData(char*& pData)const {
    if(data!=NULL&&recordSize!=-1){
        pData=data;
        return 0;
    }else return RM_NULLRECORD;
}

RC RM_Record::GetRid(RID& rid) const{
    if(data!=NULL&&recordSize!=-1){
        rid=this->rid;
        return 0;
    }else return RM_NULLRECORD;
}

ostream& operator<<(ostream& os,const RID& r){
    PageNum p;
    SlotNum s;
    r.GetPageNum(p),r.GetSlotNum(s);
    os<<"["<<p<<","<<s<<"]";
    return os;
}







