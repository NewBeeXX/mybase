#include<cerrno>
#include<cstring>
#include<cmath>
#include<cstdio>
#include<iostream>
#include<cassert>
#include"rm.h"

using namespace std;









RM_FileHandle::RM_FileHandle():pfHandle(NULL),bFileOpen(false),bHdrChanged(false) {
}

RM_FileHandle::~RM_FileHandle() {
    if(pfHandle)delete pfHandle;
}

///这个size是记录的size
///传入一个一打开的pf_filehandle？
///但是这里面还要判断这个rmfilehandle是否bOpen，只有原本未open的才能用
///但是你又不是在这里面打开这个文件，而是用一个已经打开的pfhandle。。。

///###后来看明白了
///这个open不打开文件，这个打开的pfh是在rm manager那里
///这里做的事 把pf的filehandle拷贝一份放到这个rm层的filehandle中(作为成员变量)
///然后把这个文件中存有的rm层的fileheader存进rm层filehandle的成员变量中
RC RM_FileHandle::Open(PF_FileHandle* pfh, int size) {
    if(bFileOpen||pfHandle!=NULL)return RM_HANDLEOPEN;
    if(pfh==NULL||size<=0)return RM_FCREATEFAIL;
    bFileOpen=true;
    pfHandle=new PF_FileHandle;
    *pfHandle=*pfh;

    PF_PageHandle ph;
    pfHandle->GetThisPage(0,ph);
    pfHandle->UnpinPage(0);


    {
        ///测试一下
        char* pData;
        ph.GetData(pData);
        RM_FileHdr hdr;
        memcpy(&hdr,pData,sizeof(hdr));
    }

    this->GetFileHeader(ph);
    bHdrChanged=true;
    RC invalid =IsValid();
    if(invalid)return invalid;
    return 0;
}

///把rmhandle的pfhandle拿一份
RC RM_FileHandle::GetPF_FileHandle(PF_FileHandle& lvalue) const {
    RC invalid=IsValid();
    if(invalid)return invalid;
    lvalue=*pfHandle;
    return 0;
}

///传进的三个参数都是相当于设置返回值而用的
///不是用来规定什么的
RC RM_FileHandle::GetNextFreeSlot(PF_PageHandle& ph, PageNum& pageNum, SlotNum& slotNum) {
    RC invalid=IsValid();
    if(invalid)return invalid;
    RM_PageHdr pHdr(this->GetNumSlots());
    RC rc;
///一个疑惑是为什么要unpin？
///猜想是现在这个函数要做的只是取一个空位置,并不需要存入，只有再次调用存入的函数才需要再次固定
    if((rc=this->GetNextFreePage(pageNum))||
            (rc=pfHandle->GetThisPage(pageNum,ph))||
            (rc=pfHandle->UnpinPage(pageNum))||
            (rc=this->GetPageHeader(ph,pHdr))
      )return rc;

    bitmap b(pHdr.freeSlotMap,this->GetNumSlots());
    for(int i=0; i<this->GetNumSlots(); i++) {
        if(b.test(i)) {
            slotNum=i;
            return 0;
        }
    }

    ///一个空的slot都找不到??不是说好的freepage吗
    return -1;
}

RC RM_FileHandle::GetNextFreePage(PageNum& pageNum) {
///rm的freelist也许并不是完全空的页，而是只是有空slot的页
    RC invalid=IsValid();
    if(invalid)return invalid;
    PF_PageHandle ph;
    RM_PageHdr pHdr(this->GetNumSlots());
    PageNum p;

    if(hdr.firstFree!=RM_PAGE_LIST_END) {
        RC rc;
        if((rc=pfHandle->GetThisPage(hdr.firstFree,ph))||
                (rc=ph.GetPageNum(p))||


///这是我唯一找到的markdirty的地方了
///就是因为有了这里的markdirty 对记录的修改才会从内存到disk
                (rc=pfHandle->MarkDirty(p))||
                (rc=pfHandle->UnpinPage(hdr.firstFree))||
                (rc=this->GetPageHeader(ph,pHdr))
          )
            return rc;
    }
    if(hdr.numPages==0||
            hdr.firstFree==RM_PAGE_LIST_END||
            ///这个判断pHdr也就是上面从freelist上取下来的分页有没有freeslot的操作是真的迷
            ///既然没有freeslot为何还要挂在freelist上？？
            pHdr.numFreeSlots==0
      ) {
        ///freelist上没有分页或者整个文件都没有分页
        char *pData;
        RC rc;

        ///来个分页 空的 新的
        if((rc=pfHandle->AllocatePage(ph))||
                (rc=ph.GetData(pData))||
                (rc=ph.GetPageNum(pageNum))
          )return rc;

        RM_PageHdr phdr(this->GetNumSlots());
        pHdr.nextFree=RM_PAGE_LIST_END;
        bitmap b(this->GetNumSlots());
        b.set();
        ///拷贝一份b的字符数组到pagehdr的位图数组
        b.to_char_buf(phdr.freeSlotMap,b.numChars());
        phdr.to_buf(pData);
        if(rc=pfHandle->UnpinPage(pageNum))return rc;

        hdr.firstFree=pageNum;
        hdr.numPages++;
        bHdrChanged=true;
        return 0;
    }

    pageNum = hdr.firstFree;
    return 0;
}

///看到这里就明白了
///RM层的分页header存有nextfree(这个freelist可能跟pf的不同)并且
///存有slots数等 位置就位于一个分页的pf层header之后

RC RM_FileHandle::GetPageHeader(PF_PageHandle ph, RM_PageHdr& pHdr) const {
    char *buf;
    RC rc=ph.GetData(buf);
    pHdr.from_buf(buf);
    return rc;
}


RC RM_FileHandle::SetPageHeader(PF_PageHandle ph, const RM_PageHdr& pHdr) {
    char* buf;
    RC rc;
    if(rc=ph.GetData(buf))return rc;
    pHdr.to_buf(buf);
    return 0;
}

///这里get的是rm层的fileheader
///它其实就是一个分页，当然可以只是一个分页的前缀,作为rm层的一个文件header
RC RM_FileHandle::GetFileHeader(PF_PageHandle ph) {
    char* buf;
    RC rc=ph.GetData(buf);
    memcpy(&hdr,buf,sizeof(hdr));
    return rc;
}


RC RM_FileHandle::SetFileHeader(PF_PageHandle ph)const {
    char* buf;
    RC rc=ph.GetData(buf);
    memcpy(buf,&hdr,sizeof(hdr));
    return rc;
}


RC RM_FileHandle::GetSlotPointer(PF_PageHandle ph, SlotNum s, char*& pData) const {
    RC invalid=IsValid();
    if(invalid)return invalid;
    RC rc=ph.GetData(pData);
    if(rc>=0) {
        bitmap b=(this->GetNumSlots());
        pData=pData+RM_PageHdr(this->GetNumSlots()).size();
        pData=pData+s*this->fullRecordSize();
        ///fullre这个是一条记录的size 可以认为是一个slot的size
    }
    return rc;
}

int RM_FileHandle::GetNumSlots()const {
    if(this->fullRecordSize()!=0) {
        int bytes=PF_PAGE_SIZE-sizeof(RM_PageHdr);
        int slots=bytes/this->fullRecordSize();
        int r=sizeof(RM_PageHdr)+bitmap(slots).numChars();
        while(slots*this->fullRecordSize()+r>PF_PAGE_SIZE) {
            slots--;
        }
        return slots;
    } else return RM_RECSIZEMISMATCH;
}

RC RM_FileHandle::GetRec(const RID& rid, RM_Record& rec) const {
    RC invalid=IsValid();
    if(invalid)return invalid;

    if(!this->IsValidRID(rid))return RM_BAD_RID;
    PageNum p;
    SlotNum s;
    rid.GetPageNum(p),rid.GetSlotNum(s);
    RC rc=0;
    PF_PageHandle ph;
    RM_PageHdr pHdr(this->GetNumSlots());
    if((rc=pfHandle->GetThisPage(p,ph))||

            ///说实话在这里就unpin不太稳妥,后面似乎是还是要把这个page
            ///中的位图赋值到b这个位图的
            ///这里它给你换出了怎么办??
            ///不过可能这里并不会多线程！！！
            ///那样不是减少了很多问题？？

            (rc=pfHandle->UnpinPage(p))||
            (rc=this->GetPageHeader(ph,pHdr))
      )return rc;
    bitmap b(pHdr.freeSlotMap,this->GetNumSlots());
    ///这个位置没有记录
    if(b.test(s))return RM_NORECATRID;
    char *pData=NULL;
    if(rc=this->GetSlotPointer(ph,s,pData))return rc;
    rec.Set(pData,hdr.extRecordSize,rid);
    return 0;
}

RC RM_FileHandle::InsertRec(const char* pData, RID& rid) {
    RC invalid=IsValid();
    if(invalid)return invalid;
    if(pData==NULL)return RM_NULLRECORD;
    PF_PageHandle ph;

    RM_PageHdr pHdr(this->GetNumSlots());
    PageNum p;
    SlotNum s;
    RC rc;
    char* pSlot;
    if(rc=this->GetNextFreeSlot(ph,p,s))return rc;
    if(rc=this->GetPageHeader(ph,pHdr))return rc;
    bitmap b(pHdr.freeSlotMap,this->GetNumSlots());

    if(rc=this->GetSlotPointer(ph,s,pSlot))return rc;
    rid=RID(p,s);
    memcpy(pSlot,pData,this->fullRecordSize());
    b.reset(s);
    pHdr.numFreeSlots--;
    if(pHdr.numFreeSlots==0) {
        hdr.firstFree=pHdr.nextFree;
        pHdr.nextFree=RM_PAGE_FULLY_USED;
    }
    b.to_char_buf(pHdr.freeSlotMap,b.numChars());
    rc=this->SetPageHeader(ph,pHdr);
    return 0;
}

RC RM_FileHandle::DeleteRec(const RID& rid) {
    RC invalid=IsValid();
    if(invalid)return invalid;
    if(!this->IsValidRID(rid))return RM_BAD_RID;
    PageNum p;
    SlotNum s;
    rid.GetPageNum(p),rid.GetSlotNum(s);
    RC rc=0;
    PF_PageHandle ph;
    RM_PageHdr pHdr(this->GetNumSlots());

    if((rc=pfHandle->GetThisPage(p,ph))||
///好吧这是第二个markdirty 因为这里并没有调找freeslot这个函数
            (rc=pfHandle->MarkDirty(p))||
            (rc=pfHandle->UnpinPage(p))||
            (rc=this->GetPageHeader(ph,pHdr))
      ) return rc;

    bitmap b(pHdr.freeSlotMap,this->GetNumSlots());

    if(b.test(s))return RM_NORECATRID;

    b.set(s);

    ///原本不在freelist
    if(pHdr.numFreeSlots==0) {
        pHdr.nextFree=hdr.firstFree;
        hdr.firstFree=p;
    }
    pHdr.numFreeSlots++;
    b.to_char_buf(pHdr.freeSlotMap,b.numChars());

///这里的set rm pagehandle进到pf分页头之后
///其实这一个分页也已经unpin 就是上面的unpin
///这里敢这么写个人认为也是因为单线程
///因为才刚刚换入 这个函数也不会中断去做其他事
    rc=this->SetPageHeader(ph,pHdr);
    return rc;

}

RC RM_FileHandle::UpdataRec(const RM_Record& rec) {
    RC invalid = IsValid();
    if(invalid) return invalid;
    RID rid;
    rec.GetRid(rid);
    PageNum p;
    SlotNum s;
    rid.GetPageNum(p),rid.GetSlotNum(s);
    if(!this->IsValidRID(rid))
        return RM_BAD_RID;

    PF_PageHandle ph;
    char * pSlot;
    RC rc;
    RM_PageHdr pHdr(this->GetNumSlots());
    if((rc=pfHandle->GetThisPage(p,ph))||
            (rc=pfHandle->MarkDirty(p))||
            (rc=pfHandle->UnpinPage(p))||
            (rc=this->GetPageHeader(ph,pHdr))
      )return rc;

    bitmap b(pHdr.freeSlotMap,this->GetNumSlots());

    if(b.test(s))return RM_NORECATRID;

    char * pData = NULL;
    rec.GetData(pData);
    if(rc=this->GetSlotPointer(ph,s,pSlot))return rc;
    memcpy(pSlot,pData,this->fullRecordSize());
    return 0;
}


RC RM_FileHandle::ForcePages(PageNum pageNum) {
    RC invalid = IsValid();
    if(invalid) return invalid;
    if(!this->IsValidPageNum(pageNum)&&pageNum!=ALL_PAGES)return RM_BAD_RID;
    return pfHandle->ForcePages(pageNum);
}

bool RM_FileHandle::IsValidPageNum(const PageNum pageNum) const {
    return bFileOpen&&pageNum>=0&&pageNum<hdr.numPages;
}

bool RM_FileHandle::IsValidRID(const RID rid) const {
    PageNum p;
    SlotNum s;
    rid.GetPageNum(p);
    rid.GetSlotNum(s);
    return IsValidPageNum(p)&&s>=0&&s<this->GetNumSlots();
}




RC RM_FileHandle::IsValid() const{
    if(pfHandle==NULL||!bFileOpen)return RM_FNOTOPEN;
    if(GetNumSlots()<=0)return RM_RECSIZEMISMATCH;
    return 0;
}


