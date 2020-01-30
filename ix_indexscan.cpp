#include"ix_indexscan.h"
#include<cerrno>
#include<cstdio>
#include<cassert>
#include<iostream>

using namespace std;


///desc不是附加描述符的意思  是表示升序还是降序
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
    if(pred!=NULL)delete pred;
    if(pixh!=NULL&&pixh->GetHeight()>1) {
        if(currNode!=NULL)delete currNode;
        if(lastNode!=NULL)delete lastNode;
    }
}

RC IX_IndexScan::OpenScan(const IX_IndexHandle& fileHandle,
                          CompOp compOp,
                          void* value_,
                          ClientHint pinHint,
                          bool desc) {
    if(bOpen)return IX_HANDLEOPEN;
    if((compOp<NO_OP)||(compOp>GE_OP))return IX_FCREATEFAIL;
    ///这个constcast可以把一个const类型对象转为非const类型
    pixh=const_cast<IX_IndexHandle*>(&fileHandle);
    if((pixh==NULL)||(pixh->IsValid()!=0))return IX_FCREATEFAIL;

    bOpen=true;
    if(desc)this->desc=true;
    foundOne=false;

    pred=new Predicate(pixh->GetAttrType(),
                       pixh->GetAttrLength(),
                       0,
                       compOp,
                       value_,
                       pinHint);
    c=compOp;
    if(value_!=NULL) {
        value=value_;
        ///一开始就会定位到一个节点
        OpOptimize();
    }
    return 0;
}

RC IX_IndexScan::GetNextEntry(RID& rid) {
    void* k=NULL;
    int i=-1;
    return GetNextEntry(k,rid,i);
}

RC IX_IndexScan::GetNextEntry(void*& k, RID& rid, int& numScanned) {
    if(!bOpen)return IX_FNOTOPEN;
    if(eof)return IX_EOF;


    bool currDeleted=false;
    ///第一次调用
    if(currNode==NULL&&currPos==-1) {
        if(!desc) {
            currNode=pixh->FetchNode(pixh->FindSmallestLeaf()->GetPageRID());
            currPos=-1;
        } else {
            currNode=pixh->FetchNode(pixh->FindLargestLeaf()->GetPageRID());
            currPos=currNode->GetNumKeys();
        }
        currDeleted=false;
    } else {
        ///这里就是比较一下curr的东西跟实际节点里面存的是否一致
        if(currKey!=NULL&&currNode!=NULL&&currPos!=-1) {
            char* key=NULL;
            int ret=currNode->GetKey(currPos,(void*&)key);
            if(ret==-1)return IX_PF;
            if(currNode->CmpKey(key,currKey)!=0 || !(currRid==currNode->GetAddr(currPos)) ) {
                currDeleted=true;
            }
        }
    }

    while(currNode!=NULL) {
        int i=-1;
        if(!desc) {
            ///表示升序
            if(!currDeleted) {
                i=currPos+1;
            } else {
                i=currPos;
                currDeleted=false;
            }
            for(; i<currNode->GetNumKeys(); i++) {
                char* key=NULL;
                int ret=currNode->GetKey(i,(void*&)key);
                numScanned++;
                if(ret==-1)return IX_PF;
                currPos=i;
                if(currKey==NULL)currKey=(void*) new char[pixh->GetAttrLength()];
                memcpy(currKey,key,pixh->GetAttrLength());
                currRid=currNode->GetAddr(i);

                if(pred->eval(key,pred->initOp())) {
                    k=key;
                    rid=currNode->GetAddr(i);
                    foundOne=true;
                    return 0;
                    ///找到了
                } else {
                    if(foundOne) {
                        RC rc=EarlyExitOptimize(key);
                        if(rc)return rc;
                        if(eof)return IX_EOF;
                    }
                }
            }

        }else{
            if(!currDeleted){
                i=currPos-1;
            }else{
                i=currPos;
                currDeleted=false;
            }
            for(;i>=0;i--){
                char* key=NULL;
                int ret=currNode->GetKey(i,(void*&)key);
                numScanned++;
                if(ret==-1)return IX_PF;
                currPos=i;
                if(currKey==NULL)currKey=(void*) new char[pixh->GetAttrLength()];
                memcpy(currKey,key,pixh->GetAttrLength());
                currRid=currNode->GetAddr(i);
                if(pred->eval(key,pred->initOp())){
                    k=key;
                    rid=currNode->GetAddr(i);
                    foundOne=true;
                    return 0;
                    ///找到了
                }else{
                    if(foundOne){
                        RC rc=EarlyExitOptimize(key);
                        if(rc)return rc;
                        if(eof)return IX_EOF;
                    }
                }
            }
        }


        if((lastNode!=NULL)&&(currNode->GetPageRID()==lastNode->GetPageRID()))
            break;

        ///初步估计这就是个在叶子节点层横向扫描的函数
        ///下面就是跳转到下一个节点
        if(!desc){
            PageNum right=currNode->GetRight();
            pixh->UnPin(currNode->GetPageRID().Page());
            delete currNode;
            currNode=NULL;
            currNode=pixh->FetchNode(right);
            if(currNode!=NULL)pixh->Pin(currNode->GetPageRID().Page());
            currPos=-1;
        }else{
            PageNum left=currNode->GetLeft();
            pixh->UnPin(currNode->GetPageRID().Page());
            delete currNode;
            currNode=NULL;
            currNode=pixh->FetchNode(left);
            if(currNode!=NULL){
                pixh->Pin(currNode->GetPageRID().Page());
                currPos=currNode->GetNumKeys();
            }
        }

    }
    return IX_EOF;

}

RC IX_IndexScan::CloseScan() {
    if(!bOpen)return IX_FNOTOPEN;
    bOpen=false;
    if(pred!=NULL)delete pred;
    pred=NULL;
    currNode=NULL;
    currPos=-1;
    if(currKey!=NULL){
        delete [] ((char*)currKey);
        currKey=NULL;
    }
    currRid=RID(-1,-1);
    lastNode=NULL;
    eof=false;
    return 0;
}

RC IX_IndexScan::ResetState() {
    currNode=NULL;
    currPos=-1;
    lastNode=NULL;
    eof=false;
    foundOne=false;
    return this->OpOptimize();
}

///这个函数是用来尽早终止枚举的 因为叶子节点有序 可以发现当前没有之后肯定没有的情况 直接置eof
///从下面equal的处理 猜测 这个scan的枚举只是找到第一个符合的节点然后向左或向右枚举 并不是全表那种
///当然 不等于 或 无比较 的情况除外
RC IX_IndexScan::EarlyExitOptimize(void* now) {
    if(!bOpen)return IX_FNOTOPEN;
    if(value==NULL)return 0;

    ///没得终止
    if(c==NE_OP||c==NO_OP)return 0;

    if(currNode!=NULL){
        int cmp=currNode->CmpKey(now,value);
        if(c==EQ_OP&&cmp!=0){
            eof=true;
            return 0;
        }else if(!desc){
            if( ((c==LT_OP)||(c==LE_OP)) && cmp>0 ){
                eof=true;
                return 0;
            }
        }else if(desc){
            if( ((c==GT_OP)||(c==GE_OP)) && cmp<0 ){
                eof=true;
                return 0;
            }
        }
    }
    return 0;
}



RC IX_IndexScan::OpOptimize() {
    if(!bOpen)return IX_FNOTOPEN;
    ///无值的不用优化
    if(value==NULL)return 0;
    ///不等于的不用优化
    if(c==NE_OP)return 0;
    RID r(-1,-1);

    if(currNode!=NULL)delete currNode;
    ///强行找到真正的最右节点
    currNode=pixh->FetchNode(pixh->FindLeafForceRight(value)->GetPageRID().Page());
    currPos=currNode->FindKey((const void*&)value);


    ///可以发现下面如果把currNode和currPos赋为NULL和-1的都是不能优化的

    ///降序扫<=的或<的
    if( desc ){
        if((c==LE_OP||c==LT_OP)){
            lastNode=NULL;
            currPos=currPos+1;
        }else if(c==EQ_OP){
            if(currPos==-1){
                ///根本就没有这个key
                delete currNode;
                eof=true;
                return 0;
            }
            lastNode=NULL;
            currPos=currPos+1;
        }else if(c==GE_OP){
            ///sb情况 要找>=的key 但是又往左边扫 优化不了
            ///可以仔细观察一下 这里其实可以优化 它原本这里不优化的话 扫描的第一次就会 找到叶子层最大/最小的节点 暴力枚举
            lastNode=NULL;
            delete currNode;
            currNode=NULL;
            currPos=-1;
        }else if(c==GT_OP){
            lastNode=pixh->FetchNode(currNode->GetPageRID());
            delete currNode;
            currNode=NULL;
            currPos=-1;
        }
    }else{
        ///升序扫

        if(c==LE_OP||c==LT_OP){
            lastNode=pixh->FetchNode(currNode->GetPageRID());
            delete currNode;
            currNode=NULL;
            currPos=-1;
        }else if(c==GT_OP){
            lastNode=NULL;
            ///可以优化的情况
        }else if(c==GE_OP){
            ///要找==的 不能优化
            delete currNode;
            currNode=NULL;
            currPos=-1;
            lastNode=NULL;
        }else if(c==EQ_OP){
            if(currPos==-1){///根本就没有key的
                delete currNode;
                eof=true;
                return 0;
            }
            lastNode=pixh->FetchNode(currNode->GetPageRID());
            delete currNode;
            currNode=NULL;
            currPos=-1;
        }
    }

    return 0;
}













