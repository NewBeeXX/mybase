#include"pf_internal.h"
#define INVALID_PAGE (-1)

///用来访问页内容的一个类
///需要把自己传给filehandle
///使用完一个页面的内容记得unpinned
///内部通过查看pPageData是否为NULL来判断该页是否被固定
PF_PageHandle::PF_PageHandle(){
    pageNum=INVALID_PAGE;
    pPageData=NULL;
}

PF_PageHandle::~PF_PageHandle(){
}

PF_PageHandle::PF_PageHandle(const PF_PageHandle &pageHandle){
    this->pageNum=pageHandle.pageNum;
    this->pPageData=pageHandle.pPageData;
}

PF_PageHandle& PF_PageHandle::operator=(const PF_PageHandle &pageHandle){
    if(this!=&pageHandle){
        this->pageNum=pageHandle.pageNum;
        this->pPageData=pageHandle.pPageData;
    }
    return *this;
}

RC PF_PageHandle::GetData(char *&pData)const{
    if(pPageData==NULL)return PF_PAGEUNPINNED;
    pData=pPageData;
    return 0;
}

RC PF_PageHandle::GetPageNum(PageNum& _pageNum)const{
    if(pPageData==NULL)return PF_PAGEUNPINNED;
    _pageNum=this->pageNum;
    return 0;
}











