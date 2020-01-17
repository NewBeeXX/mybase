#ifndef RM_H
#define RM_H


#include "mybase.h"
#include "rm.error.h"
#include "rm.rid.h"
#include "pf.h"
#include "predicate.h"

///如何记录哪些位置可以插入记录？
///用list记录哪些分页上还有slot
///对于一个分页,用位图来记录该分页上哪些slot可用

struct RM_FileHdr {
    int firstFree;///文件中因为删除而空出来的分页
    int numPages;///分页数
    int extRecordSize;///记录的总size
};

///用位图来标记一个分页中的哪些slot有可用的空间
class bitmap {
public:
    bitmap(int numBits);
    bitmap(char* buf,int numBits);
    ~bitmap();
    void set(unsigned int bitNumber);
    void set();
    void reset(unsigned int bitNumber);
    void reset();
    bool test(unsigned int bitNumber)const;

    int numChars()const; ///返回位图的字节数
    int to_char_buf(char*,int len)const; ///拷贝一份到一个字符数组
    int getSize()const {
        return size;
    }

private:
    unsigned int size;
    char* buffer;

};


ostream& operator<<(ostream& os,const bitmap& b);

#define RM_PAGE_LIST_END -1
#define RM_PAGE_FULLY_USED -2 ///slot满了

struct RM_PageHdr {
    int nextFree; // nextFree can be any of these values:
    //  - the number of the next free page
    //  - RM_PAGE_LIST_END if this is last free page
    //  - RM_PAGE_FULLY_USED if the page is not free
    char *freeSlotMap;///位图
    int numSlots;///因为存储的记录类型不同，所以可能空间不同，所以slot数也会不同
    int numFreeSlots;


    RM_PageHdr(int numSlots):numSlots(numSlots),numFreeSlots(numSlots) {
        freeSlotMap=new char[this->mapsize()];
    }
    ~RM_PageHdr() {
        delete [] freeSlotMap;
    }
    int size()const {
        return sizeof(nextFree)+sizeof(numSlots)+sizeof(numFreeSlots)
               +bitmap(numSlots).numChars()*sizeof(char);
    }
    int mapsize()const {
        return this->size()-sizeof(nextFree)-sizeof(numSlots)-sizeof(numFreeSlots);
    }
    int to_buf(char* &buf)const {
        memcpy(buf, &nextFree, sizeof(nextFree));
        memcpy(buf + sizeof(nextFree), &numSlots, sizeof(numSlots));
        memcpy(buf + sizeof(nextFree) + sizeof(numSlots),
               &numFreeSlots, sizeof(numFreeSlots));
        memcpy(buf + sizeof(nextFree) + sizeof(numSlots) + sizeof(numFreeSlots),
               freeSlotMap, this->mapsize()*sizeof(char));
        return 0;
    }
    int from_buf(const char* buf) {
        memcpy(&nextFree, buf, sizeof(nextFree));
        memcpy(&numSlots, buf + sizeof(nextFree), sizeof(numSlots));
        memcpy(&numFreeSlots, buf + sizeof(nextFree) + sizeof(numSlots),
               sizeof(numFreeSlots));
        memcpy(freeSlotMap,
               buf + sizeof(nextFree) + sizeof(numSlots) + sizeof(numFreeSlots),
               this->mapsize()*sizeof(char));
        return 0;
    }
};


///表示记录的类--记录的句柄
///用RID和长度和一个指针来做句柄
class RM_Record{
    friend class RM_RecordTest;

public:
    RM_Record();
    ~RM_Record();

    RC GetData(char *&pData)const;
    RC Set(char* pData,int size,RID id);
    RC GetRid(RID &rid)const;
private:
    int recordSize;
    char* data;
    RID rid;
};


///注意这里面有几个插入记录修改记录删除记录的方法
class RM_FileHandle{
    friend class RM_FileHandleTest;
    friend class RM_FileScan;
    friend class RM_Manager;
public :
    RM_FileHandle();
    ~RM_FileHandle();
    RC Open(PF_FileHandle*,int recordSize);
    RC SetHdr(RM_FileHdr h){hdr=h;return 0;}

    RC GetRec(const RID& rid,RM_Record& rec);
    RC InsertRec(const char* pData,RID &rid);
    RC DeleteRec(const RID& rid);
    RC UpdataRec(const RM_Record& rec);

    ///分页写回disk
    RC ForcePages(PageNum pageNum=ALL_PAGES);

    RC GetPF_FileHandle(PF_FileHandle&) const;

    bool hdrChanged()const{return hdrChanged;}
    int fullRecordSize()const{reurn hdr.extRecordSize;}
    int GetNumPages()const{reutrn hdr.numPages;}
    int GetNumSlots()const;

    RC IsValid()const;

private:
    bool IsValidPageNum(const PageNum pageNum)const;
    bool IsValidRID(const RID rid)const;

    RC GetNextFreePage(PageNum& pageNum);
    RC GetNextFreeSlot(PF_PageHandle& ph,PageNum& pageNum,SlotNum&);
    RC GetPageHeader(PF_PageHandle ph,RM_PageHdr& pHdr)const;
    RC SetPageHeader(PF_PageHandle ph,const RM_PageHdr& pHdr);
    RC GetSlotPointer(PF_PageHandle ph,SlotNum s,char *&pData)const;

    RC GetFileHeader(PF_PageHandle ph);

    RC SetFileHeader(PF_PageHandle ph)const;

    PF_FileHandle *pfHandle;
    RM_FileHdr hdr;
    bool bFileOpen;
    bool bHdrChanged;

};


class RM_FileScan{

public:
    RM_FileScan();
    ~RM_FileScan();

    RC OpenScan(const RM_FileHandle &fileHandle,
                AttrType attrType,
                int attrLength,
                int attrOffset,
                CompOp,
                void *value,
                ClientHint pinHint=NO_HINT
                );
    RC GetNextRec(RM_Record& rec);
    RC CloseScan();
    bool IsOpen()const{return bOpen&&prmh!=NULL&&pred!=NULL};
    void resetState(){current=RID(1,-1);}
    RC GotoPage(PageNum p);

    ///获取每页的slot数
    int GetNumSlotsPerPage()const{return prmh->GetNumSlots();}
private:
    Predicate *pred;
    RM_FileHandle *prmh;
    RID current;
    bool bOpen;
};

class RM_Manager{
public:
    RM_Manager(PF_Manager &pfm);
    ~RM_Manager();
    RC CreateFile(const char* filename,int recordSize);
    RC DestroyFile(const char* filename);
    RC OpenFile(const char* filename,RM_FileHandle &fileHandle);
    RC CloseFile(RM_FileHandle &fileHandle);


private:
    PF_Manager& pfm;
};





#endif // RM_H
