#include<cstdio>
#include<unistd.h>
#include<iostream>
#include"pf_buffermgr.h"

using namespace std;

///用来记录mgr的一些数据
#ifdef STATS
#include "statistics.h"
StatisticsMgr* pStatisticsMgr;
#endif // STATS

///用来记录对buffermgr的函数调用
#ifdef PF_LOG

void WriteLog(const char* psMessage) {
    static FILE *fLog=NULL;
    if(fLog==NULL) {
        int iLogNum=-1;
        int bFound=FALSE;
        char psFileName[10];
        while(iLogNum<999&&bFound==FALSE) {
            iLogNum++;
            sprintf(psFileName,"PF_LOG.%d",iLogNum);
            fLog=fopen(psFileName,"r");
            if(fLog==NULL) { ///找到了可用的命名
                bFound=TRUE;
                fLog=fopen(psFileName,"w");
            } else delete fLog;///如果存在这个文件就删掉文件指针
        }
        if(!bFound) {
            cerr<<"Cannot create a new log file"<<endl;
            exit(1);
        }
    }

    fprintf(fLog,psMessage);
}
#endif // PF_LOG


///bufferMgr
///这个类就是pf的大脑,每当需要访问(get)一个页，它会先检查是否在缓冲区，是就固定他
///如果不在缓冲区,就先从文件中读出来，再固定
///如果想读入一个新页,但是缓冲区满了，就按照最久未访问的原则换出一个未固定页

///这里用了初始化列表，变量名()  调用的是构造函数
PF_BufferMgr::PF_BufferMgr(int _numPages):hashTable(PF_HASH_TBL_SIZE) {


    this->numPages=_numPages;
    pageSize=PF_PAGE_SIZE+sizeof(PF_PageHdr);

#ifdef PF_STATS
    pStatisticsMgr=new StatisticsMgr();
#endif // PF_STATS

#ifdef PF_LOG
    char psMessage[100];
    sprintf (psMessage, "Creating buffer manager. %d pages of size %d.\n",
             numPages, PF_PAGE_SIZE+sizeof(PF_PageHdr));
    WriteLog(psMessage);
#endif // PF_LOG

    ///主角再此，分页描述数组,即缓冲区...
    bufTable=new PF_BufPageDesc[numPages];

    ///free list的含义也很明显了,就是未固定的分页

    ///就是用数组来模拟链表



    for(int i=0; i<numPages; i++) {
        if((bufTable[i].pData=new char[pageSize]) == NULL) {
            cerr<<"Not enough memory for buffer"<<endl;
            exit(1);
        }
        memset((void*)bufTable[i].pData,0,pageSize);


        bufTable[i].prev=i-1;
        bufTable[i].next=i+1;
    }
    bufTable[0].prev=bufTable[numPages-1].next=INVALID_SLOT;
    free=0;
    first=last=INVALID_SLOT;
#ifdef PF_LOG
    WriteLog("Succesfully created the buffer manager.\n");
#endif
}

PF_BufferMgr::~PF_BufferMgr() {
    for(int i=0; i<this->numPages; i++)delete [] bufTable[i].pData;
    delete [] bufTable;

#ifdef PF_STATS
    delete pStatisticsMgr;
#endif

#ifdef PF_LOG
    WriteLog("Destroyed the buffer manager.\n");
#endif
}


///取得一个分页的内存地址
///bMultiplePinsr只有true才能够允许多次固定(两个地方同时取它地址)

RC PF_BufferMgr::GetPage(int fd,PageNum pageNum,char** ppBuffer,int bMultiplePins) {
    RC rc;
    int slot;
#ifdef PF_LOG
    char psMessage[100];
    sprintf (psMessage, "Looking for (%d,%d).\n", fd, pageNum);
    WriteLog(psMessage);
#endif

#ifdef PF_STATS
    pStatisticsMgr->Register(PF_GETPAGE, STAT_ADDONE);
#endif

    ///出现了除了 找到和未找到以外的错误
    if((rc=hashTable.Find(fd,pageNum,slot)) && (rc!=PF_HASHNOTFOUND)) {
        return rc;
    }

    ///不在缓冲区就要从文件读
    if(rc==PF_HASHNOTFOUND) {
#ifdef PF_STATS
        pStatisticsMgr->Register(PF_PAGENOTFOUND, STAT_ADDONE);
#endif
        ///分配一个可用的页空间,要把文件中的分页读进去了
        ///也许页的换出就是在internalAlloc中进行的s
        if(rc=InternalAlloc(slot))return rc;

        if((rc=ReadPage(fd,pageNum,bufTable[slot].pData)) ||
                (rc=hashTable.Insert(fd,pageNum,slot)) ||
                (rc=InitPageDesc(fd,pageNum,slot))
          ) {

            ///进来这里就是出错了
            ///把该页放回free链
            Unlink(slot);
            InsertFree(slot);
            return rc;
        }
#ifdef PF_LOG
        WriteLog("Page not found in buffer. Loaded.\n");
#endif
    } else {
        ///如果缓冲区就有该页

        ///如果不给多重固定,判断出先前有固定就err了
        if(!bMultiplePins&&bufTable[slot].pinCount>0)
            return PF_PAGEPINNED;

        bufTable[slot].pinCount++;
#ifdef PF_LOG
        sprintf (psMessage, "Page found in buffer.  %d pin count.\n",
                 bufTable[slot].pinCount);
        WriteLog(psMessage);
#endif
        ///把这个节点拆下来换到最常访问端
        if((rc=Unlink(slot)) ||
                (rc=LinkHead(slot))
          )
            return rc;
    }
    ///传回地址
    *ppBuffer=bufTable[slot].pData;

    return 0;
}



///这个函数的作用也只是分配一个跟fd pageNum对应的新页,发现存在缓冲区中就不干
///他跟InternalAlloc的区别就是这里会进行fd pageNum到slot的绑定
///InternalAlloc 还会进行页换出等复杂操作
///这个函数跟上一个函数的不同就是他不会给该页固定
RC PF_BufferMgr::AllocatePage(int fd,PageNum pageNum,char **ppBuffer) {
    RC rc;
    int slot;

#ifdef PF_LOG
    char psMessage[100];
    sprintf (psMessage, "Allocating a page for (%d,%d)....", fd, pageNum);
    WriteLog(psMessage);
#endif

    if(!(rc=hashTable.Find(fd,pageNum,slot)))
        return PF_PAGEINBUF;///发现在缓冲区中
    else if(rc!=PF_HASHNOTFOUND)
        return rc;///没有在缓冲区中，但出现了其他错误

    if(rc=InternalAlloc(slot))return rc;

    if((rc=hashTable.Insert(fd,pageNum,slot)) ||
            (rc=InitPageDesc(fd,pageNum,slot))
      ) {
        ///出错,还原
        Unlink(slot);
        InsertFree(slot);
        return rc;
    }


    *ppBuffer=bufTable[slot].pData;

    return 0;
}


RC PF_BufferMgr::MarkDirty(int fd,PageNum pageNum) {
    RC rc;
    int slot;
#ifdef PF_LOG
    char psMessage[100];
    sprintf (psMessage, "Marking dirty (%d,%d).\n", fd, pageNum);
    WriteLog(psMessage);
#endif

    ///只有未找到和出错会进入if内
    if(rc=hashTable.Find(fd,pageNum,slot)) {
        if(rc==PF_HASHNOTFOUND)return PF_PAGENOTINBUF;
        else return rc;
    }

    ///一个没有固定的页,mark dirty没有意义
    if(bufTable[slot].pinCount==0)return PF_PAGEUNPINNED;

    bufTable[slot].bDirty=TRUE;


    ///但是为什么要放到MRU (most recently used)端？
    if((rc=Unlink(slot))||
            (rc=LinkHead(slot))
      )
        return rc;

    return 0;
}


RC PF_BufferMgr::UnpinPage(int fd,PageNum pageNum) {
    RC rc;
    int slot;

    if(rc=hashTable.Find(fd,pageNum,slot)) {
        if(rc==PF_HASHNOTFOUND)
            return PF_PAGENOTINBUF;
        else return rc;
    }

    if(bufTable[slot].pinCount==0)return PF_PAGEUNPINNED;

#ifdef PF_LOG
    char psMessage[100];
    sprintf (psMessage, "Unpinning (%d,%d). %d Pin count\n",
             fd, pageNum, bufTable[slot].pinCount-1);
    WriteLog(psMessage);
#endif

    ///为什么解固定也要放到mru端？
    if(--(bufTable[slot].pinCount)==0) {
        if((rc=Unlink(slot)) ||
                (rc=LinkHead(slot))
          )
            return rc;
    }
    return 0;
}

///FlushPages 将一个文件的所有分页存回硬盘，并且都变成free空间
///如果有pin的页，返回警告
///暴力遍历来实现，缓冲区大小很小，不需要用更快的方法

RC PF_BufferMgr::FlushPages(int fd) {
    RC rc,rcWarn=0;

#ifdef PF_LOG
    char psMessage[100];
    sprintf (psMessage, "Flushing all pages for (%d).\n", fd);
    WriteLog(psMessage);
#endif
#ifdef PF_STATS
    pStatisticsMgr->Register(PF_FLUSHPAGES, STAT_ADDONE);
#endif

    ///找到属于该文件的分页
    int slot=first;
    while(slot!=INVALID_SLOT) {
        int next=bufTable[slot].next;
        if(bufTable[slot].fd==fd) {
            ///找到一个
#ifdef PF_LOG
            sprintf (psMessage, "Page (%d) is in buffer manager.\n", bufTable[slot].pageNum);
            WriteLog(psMessage);
#endif
            if(bufTable[slot].pinCount) {
                rcWarn=PF_PAGEPINNED;
                ///并不返回错误,只是跳过了被pin的页
            } else {
                if(bufTable[slot].bDirty) {
#ifdef PF_LOG
                    sprintf (psMessage, "Page (%d) is dirty\n",bufTable[slot].pageNum);
                    WriteLog(psMessage);
#endif
                    if(rc=WritePage(fd,bufTable[slot].pageNum,bufTable[slot].pData))
                        return rc;
                    bufTable[slot].bDirty=FALSE;
                }

                if((rc=hashTable.Delete(fd,bufTable[slot].pageNum)) ||
                        (rc=Unlink(slot))||
                        (rc=InsertFree(slot))
                  )
                    return rc;
            }
        }
        slot=next;
    }

#ifdef PF_LOG
    WriteLog("All necessary pages flushed.\n");
#endif

    return rcWarn;
}


///并不会把分页移出缓冲区
RC PF_BufferMgr::ForcePages(int fd,PageNum pageNum) {
    RC rc;
#ifdef PF_LOG
    char psMessage[100];
    sprintf (psMessage, "Forcing page %d for (%d).\n", pageNum, fd);
    WriteLog(psMessage);
#endif

    int slot=first;
    while(slot!=INVALID_SLOT) {
        int next=bufTable[slot].next;
        if(bufTable[slot].fd==fd&&(pageNum==ALL_PAGES||pageNum==bufTable[slot].pageNum)) {
#ifdef PF_LOG
            sprintf (psMessage, "Page (%d) is in buffer pool.\n", bufTable[slot].pageNum);
            WriteLog(psMessage);
#endif
            if(bufTable[slot].bDirty) {
#ifdef PF_LOG
                sprintf (psMessage, "Page (%d) is dirty\n",bufTable[slot].pageNum);
                WriteLog(psMessage);
#endif
                if(rc=WritePage(fd,bufTable[slot].pageNum,bufTable[slot].pData))
                    return rc;
                bufTable[slot].bDirty=FALSE;
            }

        }
        slot=next;
    }

    return 0;
}


///打印缓冲区的分页
///debug用

RC PF_BufferMgr::PrintBuffer() {
    cout << "Buffer contains " << numPages << " pages of size "
         << pageSize <<".\n";
    cout << "Contents in order from most recently used to "
         << "least recently used.\n";

    ///以下是自己改了一部分的代码
    for(int i=first;i!=INVALID_SLOT;i=bufTable[i].next){
        cout << i << " :: \n";
        cout << "  fd = " << bufTable[i].fd << "\n";
        cout << "  pageNum = " << bufTable[i].pageNum << "\n";
        cout << "  bDirty = " << bufTable[i].bDirty << "\n";
        cout << "  pinCount = " << bufTable[i].pinCount << "\n";
    }
    if(first==INVALID_SLOT)cout << "Buffer is empty!\n";
    else cout << "All remaining slots are free.\n";

    return 0;
}


///debug用
RC PF_BufferMgr::ClearBuffer(){
    RC rc;
    for(int i=first;i!=INVALID_SLOT;i=bufTable[i].next){
        if(bufTable[i].pinCount==0){
            if((rc=hashTable.Delete(bufTable[i].fd,bufTable[i].pageNum))||
               (rc=Unlink(i))||
               (rc=InsertFree(i))
               )
                return rc;
        }
    }
    return 0;
}


///debug用
RC PF_BufferMgr::ResizeBuffer(int iNewSize){
    RC rc;
    ClearBuffer();
    PF_BufPageDesc *pNewBufTable=new PF_BufPageDesc[iNewSize];
    for(int i=0;i<iNewSize;i++){
        if((pNewBufTable[i].pData=new char[pageSize])==NULL){
            cout<<"Not enough memory for buffer\n";
            exit(1);
        }
        memset((void*)pNewBufTable[i].pData,0,pageSize);
        pNewBufTable[i].prev=i-1;
        pNewBufTable[i].next=i+1;
    }
    pNewBufTable[0].prev=pNewBufTable[iNewSize-1].next=INVALID_SLOT;

    int oldFirst=first;
    PF_BufPageDesc *pOldBufTable=bufTable;

    numPages=iNewSize;
    first=last=INVALID_SLOT;
    free=0;

    bufTable=pNewBufTable;

    for(int i=oldFirst;i!=INVALID_SLOT;i=pOldBufTable[i].next){
        if(rc=hashTable.Delete(pOldBufTable[i].fd,pOldBufTable[i].pageNum))
            return rc;
    }
    int newSlot;
    for(int i=oldFirst;i!=INVALID_SLOT;i=pOldBufTable[i].next){
        if(rc=InternalAlloc(newSlot))return rc;
        if((rc=hashTable.Insert(pOldBufTable[i].fd,pOldBufTable[i].pageNum,newSlot))||
           (rc=InitPageDesc(pOldBufTable[i].fd,pOldBufTable[i].pageNum,newSlot))
           )
            return rc;
        Unlink(newSlot);
        InsertFree(newSlot);
    }

    delete [] pOldBufTable;

    return 0;
}

///插一个slot到free链的头

RC PF_BufferMgr::InsertFree(int slot){
    bufTable[slot].next=free;
    free=slot;
    return 0;
}

///插一个slot到used链，让他在最近最常用端 MRU
RC PF_BufferMgr::LinkHead(int slot){
    bufTable[slot].next=first;
    bufTable[slot].prev=INVALID_SLOT;
    if(first!=INVALID_SLOT)bufTable[first].prev=slot;
    first=slot;
    ///原本是空链的话，将该slot设为last
    if(last==INVALID_SLOT)last=first;
    return 0;
}


///只对used链中的slot调用

RC PF_BufferMgr::Unlink(int slot){
    if(first==slot)first=bufTable[slot].next;
    if(last==slot)last=bufTable[slot].prev;

    if(bufTable[slot].next!=INVALID_SLOT)
        bufTable[bufTable[slot].next].prev = bufTable[slot].prev;
    if(bufTable[slot].prev!=INVALID_SLOT)
        bufTable[bufTable[slot].prev].next = bufTable[slot].next;

    bufTable[slot].prev=bufTable[slot].next=INVALID_SLOT;
    return 0;
}

///LRU最近最久未使用
///此函数分配一个节点,也是lru的体现,主要是在free链空时,会选择used的非pin节点中least used的节点换出去
///lru其实是选一个离当前时间点最久未使用(least recently used)的节点换出去
///具体来说,取 当前所有存在于buffer中的节点,按照最迟访问时间从小到大排序，最小的那个就是要被换出的
///如果当成思维题来做，最终你就会推出下面的用链表维护这个序列的方法
RC PF_BufferMgr::InternalAlloc(int &slot){
    RC rc;
    ///free链非空
    if(free!=INVALID_SLOT){
        slot=free;
        free=bufTable[slot].next;
        ///原来一个节点在freelist还是usedlist用的都是同样的指针
    }else{
        ///找到一个最久未使用的非固定的节点
        for(slot=last;slot!=INVALID_SLOT;slot=bufTable[slot].prev)
            if(bufTable[slot].pinCount==0)break;
        ///全都固定了，一页都换不出
        if(slot==INVALID_SLOT)return PF_NOBUF;
        if(bufTable[slot].bDirty){
            if(rc=WritePage(bufTable[slot].fd,bufTable[slot].pageNum,bufTable[slot].pData))
                return rc;
            bufTable[slot].bDirty=FALSE;
        }
        if((rc=hashTable.Delete(bufTable[slot].fd,bufTable[slot].pageNum))||
           (rc=Unlink(slot))
           )
            return rc;
    }
    ///不管是从freelist取下来的还是换出usedlist的，
    if(rc=LinkHead(slot))return rc;
    return 0;
}

///从disk读一个分页到内存
RC PF_BufferMgr::ReadPage(int fd,PageNum pageNum,char* dest){

#ifdef PF_LOG
   char psMessage[100];
   sprintf (psMessage, "Reading (%d,%d).\n", fd, pageNum);
   WriteLog(psMessage);
#endif

#ifdef PF_STATS
   pStatisticsMgr->Register(PF_READPAGE, STAT_ADDONE);
#endif

    ///文件头的信息也是写入disk的，它在一个文件的头部
    long offset=pageNum*(long)pageSize + PF_FILE_HDR_SIZE;
    ///lseek返回文件指针偏移量,-1为出错
    if(lseek(fd,offset,L_SET)<0)return PF_UNIX;
    int numBytes=read(fd,dest,pageSize);
    if(numBytes<0)return PF_UNIX;
    else if(numBytes!=pageSize)return PF_INCOMPLETEREAD;
    else return 0;

}


///写到disk
RC PF_BufferMgr::WritePage(int fd,PageNum pageNum,char *source){
#ifdef PF_LOG
   char psMessage[100];
   sprintf (psMessage, "Writing (%d,%d).\n", fd, pageNum);
   WriteLog(psMessage);
#endif

#ifdef PF_STATS
   pStatisticsMgr->Register(PF_WRITEPAGE, STAT_ADDONE);
#endif

    long offset=pageNum*(long)pageSize+PF_FILE_HDR_SIZE;
    if(lseek(fd,offset,L_SET)<0)return PF_UNIX;
    int numBytes=write(fd,source,pageSize);
    if(numBytes<0)return PF_UNIX;
    else if(numBytes!=pageSize)return PF_INCOMPLETEWRITE;
    else return 0;
}



RC PF_BufferMgr::InitPageDesc(int fd,PageNum pageNum,int slot){
    bufTable[slot].fd=fd;
    bufTable[slot].pageNum=pageNum;
    bufTable[slot].bDirty=FALSE;
    bufTable[slot].pinCount=1;
    return 0;
}


#define MEMORY_FD -1

///似乎下面的函数都是随便玩玩

RC PF_BufferMgr::GetBlockSize(int& length)const{
    length=pageSize;
    return OK_RC;
}

RC PF_BufferMgr::AllocateBlock(char *&buffer){
    RC rc=OK_RC; ///是0
    int slot;
    if((rc=InternalAlloc(slot))!=OK_RC)return rc;
    ///这句有点问题
    int t=rand();
    PageNum pageNum=t;//(int)(bufTable[slot].pData);
//    memcpy(bufTable[slot].pData,&t,sizeof(int));
    *((int*)bufTable[slot].pData)=t;

    if((rc=hashTable.Insert(MEMORY_FD,pageNum,slot))!=OK_RC||
       (rc=InitPageDesc(MEMORY_FD,pageNum,slot)!=OK_RC)
       ){
        Unlink(slot);
        InsertFree(slot);
        return rc;
    }
    buffer=bufTable[slot].pData;
    return OK_RC;
}

RC PF_BufferMgr::DisposeBlock(char* buffer){
    return UnpinPage(MEMORY_FD,*((int*)buffer));
}


















