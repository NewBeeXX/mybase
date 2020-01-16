///操控文件及其分页的类
///经常需要传引用给pf_manager才能行使功能

#include<unistd.h>
#include<sys/types.h>
#include"pf_internal.h"
#include"pf_buffermgr.h"
#include<iostream>
#include<cstdio>
using namespace std;

PF_FileHandle::PF_FileHandle() {
    bFileOpen=FALSE;
    pBufferMgr=NULL;
}

PF_FileHandle::~PF_FileHandle() {
///nothing
}
///拷贝构造
PF_FileHandle::PF_FileHandle(const PF_FileHandle &fileHandle) {
    this->pBufferMgr  = fileHandle.pBufferMgr;
    this->hdr         = fileHandle.hdr;
    this->bFileOpen   = fileHandle.bFileOpen;
    this->bHdrChanged = fileHandle.bHdrChanged;
    this->unixfd      = fileHandle.unixfd;
}

PF_FileHandle& PF_FileHandle::operator=(const PF_FileHandle &fileHandle){
    if(this!=&fileHandle){
        this->pBufferMgr  = fileHandle.pBufferMgr;
        this->hdr         = fileHandle.hdr;
        this->bFileOpen   = fileHandle.bFileOpen;
        this->bHdrChanged = fileHandle.bHdrChanged;
        this->unixfd      = fileHandle.unixfd;
    }
    return *this;
}

RC PF_FileHandle::GetFirstPage(PF_PageHandle& pageHandle)const{
    return GetNextPage(-1,pageHandle);///这里规范写应该强转-1为pagenum
}

RC PF_FileHandle::GetLastPage(PF_PageHandle& pageHandle)const{
    return GetPrevPage(hdr.numPages,pageHandle);///这里规范写应该强转-1为pagenum
}

///传进的current必须是一个有效页
///一个文件中有些页面是valid的，一些invalid的，这里会遍历查找一个valid页面后面的第一个valid页面，返回它
RC PF_FileHandle::GetNextPage(PageNum current,PF_PageHandle& pageHandle)const{
    int rc;
    if(!bFileOpen)return PF_CLOSEDFILE;
    if(current!=-1&&!IsValidPageNum(current))return PF_INVALIDPAGE;
    for(current++;current<hdr.numPages;current++){
        if(!(rc=GetThisPage(current,pageHandle)))///返回0,成功get
            return 0;
        if(rc!=PF_INVALIDPAGE)return rc;
    }
    return PF_EOF;///最后都没找到有效页
}

RC PF_FileHandle::GetPrevPage(PageNum current,PF_PageHandle& pageHandle)const{
    int rc;
    if(!bFileOpen)return PF_CLOSEDFILE;
    if (current != hdr.numPages &&  !IsValidPageNum(current))
      return PF_INVALIDPAGE;
    for(current--;current>=0;current--){
        if(!(rc=GetThisPage(current,pageHandle)))
            return 0;
        if(rc!=PF_INVALIDPAGE)return rc;
    }
    return PF_EOF;
}



///有效页是什么意思,free页有是什么意思
///被成功get的页会pinned
///其实还是从buffer_mgr那里get
RC PF_FileHandle::GetThisPage(PageNum pageNum,PF_PageHandle& pageHandle)const{
    int rc;
    char* pPageBuf;
    if(!bFileOpen)return PF_CLOSEDFILE;
    if(!IsValidPageNum(pageNum))return PF_INVALIDPAGE;
    ///bufferMgr告诉你get不了
    if((rc=pBufferMgr->GetPage(unixfd,pageNum,&pPageBuf)))return rc;
    ///nextfree似乎并不仅仅是next的意思,他可以指当前页已被占用
    ///used就是固定的意思
    if( ((PF_PageHdr*)pPageBuf)->nextFree == PF_PAGE_USED){
        pageHandle.pageNum=pageNum;
        ///pPageData是一个内存中地址
        pageHandle.pPageData=pPageBuf+sizeof(PF_PageHdr);
        return 0;
    }
    ///不是used就解pined,不是used标识当前页是在文件的free链中
    if(rc=UnpinPage(pageNum))
        return rc;///解pined不成功
    return PF_INVALIDPAGE;
}


///请求分配一个属于该文件的一个可用的分页，一个没有附带历史数据的，可以用作全新用途的分页
///并即刻可以用来存东西，即这个分页分配之后会在内存中，即被固定在缓冲区中
///这个分页可能来自该文件的废弃分页链(freelist)，或者是新增加的页
RC PF_FileHandle::AllocatePage(PF_PageHandle& pageHandle){
    int rc;
    int pageNum;
    char *pPageBuf;



    if(!bFileOpen)return PF_CLOSEDFILE;
    if(hdr.firstFree!=PF_PAGE_LIST_END){
        ///有可用空间（之前开辟过又弃用的），相当于栈一样
        ///free是未被pinned的意思？
        pageNum=hdr.firstFree;
        if(rc=pBufferMgr->GetPage(unixfd,pageNum,&pPageBuf))
            return rc;///出错了
///!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 看好下一句
///由此可以推断出,pageHdr中的nexFree是用来组织一个文件的free页的，如果该页是pin的，则用used标识
///只有它是一个id或end才说明他在该文件的free链中
        hdr.firstFree=((PF_PageHdr*)pPageBuf)->nextFree;
    }else{
        pageNum=hdr.numPages;
        if(rc=pBufferMgr->AllocatePage(unixfd,pageNum,&pPageBuf))return rc;
        hdr.numPages++;
    }



    bHdrChanged=TRUE;
    ((PF_PageHdr*)pPageBuf)->nextFree=PF_PAGE_USED;
    memset(pPageBuf+sizeof(PF_PageHdr),0,PF_PAGE_SIZE);
    if(rc=MarkDirty(pageNum))return rc;
    pageHandle.pageNum=pageNum;
    pageHandle.pPageData=pPageBuf+sizeof(PF_PageHdr);
    return 0;
}

///dispose一般指处置(废物)的意思
///这个函数用来把一个页加进文件的freelist 用类似前向星那种加边
RC PF_FileHandle::DisposePage(PageNum pageNum){
    int rc;
    char *pPageBuf;

    if(!bFileOpen)return PF_CLOSEDFILE;
    if(!IsValidPageNum(pageNum)) return PF_INVALIDPAGE;

    ///这个函数就是用来取内存首地址的
    ///这里不允许多重固定，即此时是要dispose，所以get的时候不允许有其他固定
    if(rc=pBufferMgr->GetPage(unixfd,pageNum,&pPageBuf,FALSE))return rc;

    if( ((PF_PageHdr*)pPageBuf)->nextFree!=PF_PAGE_USED ){
        if(rc=UnpinPage(pageNum))return rc;
        return PF_PAGEFREE;
    }

    ///这两句让我感觉 这个nextfree 如果是used 表示该页不在freelist中，
    ///如果是 end或一个int，那么说明这页是free的，在freelist中，这个nextfree就指向下一个id，end表示他是链尾
    ((PF_PageHdr*)pPageBuf)->nextFree=hdr.firstFree;
    hdr.firstFree=pageNum;
    bHdrChanged=TRUE;

    if(rc=MarkDirty(pageNum))return rc;
    if(rc=UnpinPage(pageNum))return rc;

    return 0;
}

RC PF_FileHandle::MarkDirty(PageNum pageNum)const{
    if(!bFileOpen)return PF_CLOSEDFILE;
    if(!IsValidPageNum(pageNum))return PF_INVALIDPAGE;

    return pBufferMgr->MarkDirty(unixfd,pageNum);
}

RC PF_FileHandle::UnpinPage(PageNum pageNum)const{
    if(!bFileOpen)return PF_CLOSEDFILE;
    if(!IsValidPageNum(pageNum))return PF_INVALIDPAGE;
    return pBufferMgr->UnpinPage(unixfd,pageNum);
}

///该函数让缓冲区中该文件的所有脏的非固定的页 存到磁盘
RC PF_FileHandle::FlushPages()const{
    if(!bFileOpen)return PF_CLOSEDFILE;
    if(bHdrChanged){
        if(lseek(unixfd,0,L_SET)<0)return PF_UNIX;
        int numBytes=write(unixfd,(char*)&hdr,sizeof(PF_FileHdr));
        if(numBytes<0)return PF_UNIX;
        if(numBytes!=sizeof(PF_FileHdr))return PF_HDRWRITE;
        ///最奇葩的地方：非要定义成const函数，但是又需要改变成员变量
        PF_FileHandle *t=(PF_FileHandle*)this;
        t->bHdrChanged=FALSE;
    }
    return pBufferMgr->FlushPages(unixfd);
}


///让该文件的 一个/所有 分页存到磁盘，但是不会让分页离开缓冲区
///如果文件头有更改 也会写入disk
RC PF_FileHandle::ForcePages(PageNum pageNum)const{
    if(!bFileOpen)return PF_CLOSEDFILE;
    if(bHdrChanged){
        if(lseek(unixfd,0,L_SET)<0)return PF_UNIX;
        int numBytes=write(unixfd,(char*)&hdr,sizeof(PF_FileHdr));
        if(numBytes<0)return PF_UNIX;
        if(numBytes!=sizeof(PF_FileHdr))return PF_HDRWRITE;
        ///最奇葩的地方：非要定义成const函数，但是又需要改变成员变量
        PF_FileHandle *t=(PF_FileHandle*)this;
        t->bHdrChanged=FALSE;
    }
    return pBufferMgr->ForcePages(unixfd,pageNum);

}

///其实就是判断文件有没有打开 id是否在0~numPages-1的范围内
int PF_FileHandle::IsValidPageNum(PageNum pageNum)const{
    return bFileOpen&&pageNum>=0&&pageNum<hdr.numPages;
}


