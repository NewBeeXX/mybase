#include"ix_indexscan.h"
#include<cerrno>
#include<cstdio>
#include<cassert>
#include<iostream>

using namespace std;

IX_IndexScan::IX_IndexScan():bOpen(false),desc(false),eof(false),lastNode(NULL) {
    pred=NULL;
    pixh=NULL;
    currNode=NULL;
    currPos=-1;
    currKey=NULL;
    currRid=RID(-1,-1);
    c=EQ_OP;
    value=NULL;
}

IX_IndexScan::~IX_IndexScan() {
        if(pred)delete pred;
        if(pixh&&pixh->GetHeight()>1){
            if(currNode!=NULL)delete currNode;
            if(lastNode!=NULL)delete lastNode;
        }
}

RC IX_IndexScan::OpenScan(const IX_IndexHandle& indexHandle, CompOp compOp, void* value, ClientHint pinHint = NO_HINT, bool desc = false) {

}

RC IX_IndexScan::GetNextEntry(RID& rid) {

}

RC IX_IndexScan::GetNextEntry(void*& key, RID& rid, int& numScanned) {

}

RC IX_IndexScan::CloseScan() {

}

RC IX_IndexScan::ResetState() {

}

bool IX_IndexScan::IsOpen() {

}

bool IX_IndexScan::IsDesc() {

}

RC IX_IndexScan::OpOptimize() {

}

RC IX_IndexScan::EarlyExitOptimize(void* now) {

}


