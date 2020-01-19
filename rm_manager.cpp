#include<cstdio>
#include<cstring>
#include<unistd.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<sys/types.h>
#include"rm.h"


RM_Manager::RM_Manager(PF_Manager& pfm):pfm(pfm) {
}


RM_Manager::~RM_Manager() {

}


RC RM_Manager::CreateFile(const char* filename, int recordSize) {
    ///要保证一条记录能在一个page中放下
    if(recordSize>=PF_PAGE_SIZE-(int)sizeof(RM_PageHdr))
        return RM_SIZETOOBIG;
    if(recordSize<=0)return RM_BADRECSIZE;
    RC rc=pfm.CreateFile(filename);
    if(rc<0) {
        PF_PrintError(rc);
        return rc;
    }
    PF_FileHandle pfh;
    rc=pfm.OpenFile(filename,pfh);
    if(rc<0) {
        PF_PrintError(rc);
        return rc;
    }

    PF_PageHandle headerPage;
    char* pData;
    rc=pfh.AllocatePage(headerPage);
    if(rc<0) {
        PF_PrintError(rc);
        return RM_PF;
    }

    rc=headerPage.GetData(pData);
    if(rc<0) {
        PF_PrintError(rc);
        return RM_PF;
    }

    RM_FileHdr hdr;
    hdr.firstFree=RM_PAGE_LIST_END;
    hdr.numPages=1;
    hdr.extRecordSize=recordSize;


    ///RM层的文件头就放在一个文件的第一个分页
    memcpy(pData,&hdr,sizeof(hdr));

    PageNum headerPageNum;
    headerPage.GetPageNum(headerPageNum);
    ///原代码中这有一个断言，其实就是这个headerPageNum 必然是0

    rc==pfh.UnpinPage(headerPageNum);

    if(rc<0) {
        PF_PrintError(rc);
        return RM_PF;
    }

    rc=pfm.CloseFile(pfh);

    if(rc<0) {
        PF_PrintError(rc);
        return RM_PF;
    }

    return 0;
}

RC RM_Manager::DestroyFile(const char* filename) {
    RC rc=pfm.DestroyFile(filename);
    if(rc<0) {
        PF_PrintError(rc);
        return RM_PF;
    }
    return 0;
}

RC RM_Manager::OpenFile(const char* filename, RM_FileHandle& rmh) {
    PF_FileHandle pfh;
    RC rc=pfm.OpenFile(filename,pfh);
    if(rc<0) {
        PF_PrintError(rc);
        return RM_PF;
    }

    ///把rm层的文件头拿出来
    PF_PageHandle ph;
    char *pData;
    if((rc=pfh.GetThisPage(0,ph))||
            (rc=ph.GetData(pData))
      )return rc;

    ///之前想知道为什么这里要copy出rm层文件头，后面rmh的open里面也要
    ///copy出文件头
    ///其实这里只是读出一个记录size
    RM_FileHdr hdr;
    memcpy(&hdr,pData,sizeof(hdr));
    rc=rmh.Open(&pfh,hdr.extRecordSize);
    if(rc<0) {
        RM_PrintError(rc);
        return rc;
    }
    rc=pfh.UnpinPage(0);
    if(rc<0) {
        PF_PrintError(rc);
        return RM_PF;
    }
    return 0;
}

RC RM_Manager::CloseFile(RM_FileHandle& rfileHandle) {
    if(!rfileHandle.bFileOpen||rfileHandle.pfHandle==NULL)
        return RM_FNOTOPEN;
    if(rfileHandle.hdrChanged()){
        PF_PageHandle ph;
        rfileHandle.pfHandle->GetThisPage(0,ph);
        rfileHandle.SetFileHeader(ph);

        RC rc=rfileHandle.pfHandle->MarkDirty(0);

        if(rc<0){
            PF_PrintError(rc);
            return rc;
        }

        rc=rfileHandle.pfHandle->UnpinPage(0);
        if(rc<0){
            PF_PrintError(rc);
            return rc;
        }

        ///把这个文件的所有在缓冲区的页存进disk
        ///感觉这里只是需要把头给存进去,不过它把所有页都force了
        rc=rfileHandle.ForcePages();
        if(rc<0){
            RM_PrintError(rc);
            return rc;
        }

    }

    RC rc=pfm.CloseFile(*(rfileHandle.pfHandle));
    if(rc<0){
        PF_PrintError(rc);
        return rc;
    }
    delete rfileHandle.pfHandle;
    rfileHandle.pfHandle=NULL;
    rfileHandle.bFileOpen=false;
    return 0;
}








