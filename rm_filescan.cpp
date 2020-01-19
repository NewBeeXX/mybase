#include<cerrno>
#include<cstdio>
#include<iostream>
#include "rm.h"

using namespace std;


RM_FileScan::RM_FileScan():bOpen(false) {
    pred=NULL;
    prmh=NULL;
    current=RID(1,-1);
    ///为什么分页号是1??  因为第0页存的是RM的文件头
}
RM_FileScan::~RM_FileScan() {

}
RC RM_FileScan::OpenScan(const RM_FileHandle& fileHandle,
                         AttrType attrType,
                         int attrLength,
                         int attrOffset,
                         CompOp compOp,
                         void* value,
                         ClientHint pinHint) {

    if(bOpen)return RM_HANDLEOPEN;
    prmh=const_cast<RM_FileHandle*>(&fileHandle);
    if(prmh==NULL||
       prmh->IsValid()!=0
       ) return RM_FCREATEFAIL;

    if(value){
        if(compOp<NO_OP||compOp>GE_OP)return RM_FCREATEFAIL;
        if(attrType<INT||attrType>STRING)return RM_FCREATEFAIL;
        if(attrLength>=PF_PAGE_SIZE-sizeof(RID)||attrLength<=0)return RM_RECSIZEMISMATCH;
        if(attrType==INT&&attrLength!=sizeof(int)||
           attrType==FLOAT&&attrLength!=sizeof(float)||
           (attrType==STRING&&(attrLength<=0||attrLength>MAXSTRINGLEN))
           ) return RM_FCREATEFAIL;
        if(attrOffset>=prmh->fullRecordSize()||attrOffset<0)return RM_FCREATEFAIL;
    }

    bOpen=true;
    pred=new Predicate(attrType,attrLength,attrOffset,compOp,value,pinHint);
    return 0;
}

RC RM_FileScan::GotoPage(PageNum p) {
    if(!bOpen)return RM_FNOTOPEN;
    ///改变了scan的成员变量current
    current=RID(p,-1);
    RM_Record rec;
    RC rc=GetNextRec(rec);
    RID rid;
    rec.GetRid(rid);
    ///这里gotoPage还非要走到下一条合法记录之前的位置 haha
    current=RID(p,rid.Slot()-1);
    return 0;
}



RC RM_FileScan::GetNextRec(RM_Record& rec) {
    if(!bOpen)return RM_FNOTOPEN;
    PF_PageHandle ph;

    RM_PageHdr pHdr(prmh->GetNumSlots());
    RC rc;

    for(int j=current.Page();j<prmh->GetNumPages();j++){
        if((rc=prmh->pfHandle->GetThisPage(j,ph))||
           (rc=prmh->pfHandle->UnpinPage(j))
           ) return rc;
        if(rc=prmh->GetPageHeader(ph,pHdr))return rc;
        bitmap b(pHdr.freeSlotMap,prmh->GetNumSlots());
        int i=-1;
        ///因为每次都会把current设为已扫描位置的最后一个slot
        ///所以如果这次循环就是调这个函数的第一次循环
        ///让slot号+1 否则就说明这不是第一次循环 直接从0开始

        if(current.Page()==j)i=current.Slot()+1;
        else i=0;

        for(;i<prmh->GetNumSlots();i++){
            ///该位不可用 说明有rec
            if(!b.test(i)){
                current=RID(j,i);
                prmh->GetRec(current,rec);
                char* pData=NULL;
                rec.GetData(pData);
                if(pred->eval(pData,pred->initOp()))
                    return 0;
            }
        }
    }

    ///最后都没找到
    return RM_EOF;

}

RC RM_FileScan::CloseScan() {
    if(!bOpen)return RM_FNOTOPEN;
    bOpen=false;
    if(pred)delete pred;
    current=RID(1,-1);
    return 0;
}


