#ifndef IX_INDEX_SCAN_H
#define IX_INDEX_SCAN_H

#include"mybase.h"
#include"rm_rid.h"
#include"pf.h"
#include"ix_indexhandle.h"
#include"predicate.h"

class IX_IndexScan{
public:
    IX_IndexScan();
    ~IX_IndexScan();

    RC OpenScan(const IX_IndexHandle &indexHandle,
                CompOp compOp,
                void * value,
                ClientHint pinHint=NO_HINT,
                bool desc=false
                );

    RC GetNextEntry(RID &rid);
    RC GetNextEntry(void*& key,RID& rid,int& numScanned);
    RC CloseScan();
    RC ResetState();

    bool IsOpen()const{return bOpen&&pred!=NULL&&pixh!=NULL;}
    bool IsDesc()const{return desc;}

private:
    RC OpOptimize();
    RC EarlyExitOptimize(void* now);

private:
    Predicate* pred;
    IX_IndexHandle* pixh;
    BtreeNode* currNode;
    int currPos;
    void* currKey;
    RID currRid;
    bool bOpen;
    bool desc; ///扫描是升序还是降序
    bool eof;
    bool foundOne;
    BtreeNode* lastNode;
    CompOp c;
    void* value;

};



#endif // IX_INDEX_SCAN_H
