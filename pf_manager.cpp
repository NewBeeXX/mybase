#include<cstdio>
#include<unistd.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<sys/types.h>
#include"pf_internal.h"
#include"pf_buffermgr.h"

///程序开始时会运行一次
PF_Manager::PF_Manager() {
    pBufferMgr=new PF_BufferMgr(PF_BUFFER_SIZE);
}

PF_Manager::~PF_Manager() {
    delete pBufferMgr;
}

RC PF_Manager::CreateFile(const char* filename) {
    int fd;
    int numBytes;

    ///使用O_EXCL 发现同名文件会返回出错
    if((fd=open(filename,
#ifdef PC
                O_BINARY |
#endif // PC
                O_CREAT |O_EXCL | O_WRONLY,
                CREATION_MASK))<0)
        return PF_UNIX; ///有同名文件无法创建，返回unix的错误

    char hdrBuf[PF_FILE_HDR_SIZE];
    memset(hdrBuf,0,PF_FILE_HDR_SIZE);

    PF_FileHdr *hdr=(PF_FileHdr*)hdrBuf;
    hdr->firstFree=PF_PAGE_LIST_END;
    hdr->numPages=0;

    if((numBytes=write(fd,hdrBuf,PF_FILE_HDR_SIZE))!=PF_FILE_HDR_SIZE) {
        close(fd);
        unlink(filename);///系统调用 用来删文件s
        if(numBytes<0)return PF_UNIX;
        else return PF_HDRWRITE;
    }

    if(close(fd)<0)return PF_UNIX;
    return 0;
}

RC PF_Manager::DestroyFile(const char* filename) {
    if(unlink(filename)<0)return PF_UNIX;
    return 0;
}

RC PF_Manager::OpenFile(const char* fileName,PF_FileHandle &fileHandle) {
    int rc;

    if(fileHandle.bFileOpen)return PF_FILEOPEN;

    ///赋个fd
    if((fileHandle.unixfd=open(fileName,
#ifdef PC
                               O_BINARY|
#endif // PC
                               O_RDWR))<0 )
        return PF_UNIX;


    bool ok=1;
    int numBytes=read(fileHandle.unixfd,(char*)&fileHandle.hdr,sizeof(PF_FileHdr));
    if(numBytes!=sizeof(PF_FileHdr)) {
        rc=(numBytes<0)?PF_UNIX:PF_HDRREAD; ///后者表示未完整读取
        ok=0;
    }
    if(ok) {
        ///脏标记
        fileHandle.bHdrChanged=FALSE;
        fileHandle.pBufferMgr=pBufferMgr;
        fileHandle.bFileOpen=TRUE;
        return 0;
    } else {
        close(fileHandle.unixfd);
        fileHandle.bFileOpen=FALSE;
        return rc;
    }

}

///关文件,顺便删掉该文件在缓冲区的分页
RC PF_Manager::CloseFile(PF_FileHandle &fileHandle) {
    RC rc;
    if(!fileHandle.bFileOpen)return PF_CLOSEDFILE;
    if((rc=fileHandle.FlushPages()))return rc;///清除页面不正常
    if(close(fileHandle.unixfd)<0)return PF_UNIX;

    fileHandle.bFileOpen=FALSE;
    fileHandle.pBufferMgr=NULL;

    return 0;
}

///清缓冲区
RC PF_Manager::ClearBuffer() {
    return pBufferMgr->ClearBuffer();
}
///打印缓冲区
RC PF_Manager::PrintBuffer() {
    return pBufferMgr->PrintBuffer();
}
///重设缓冲区大小
RC PF_Manager::ResizeBuffer(int iNewSize) {
    return pBufferMgr->ResizeBuffer(iNewSize);
}


///下面三个操控内存打方法
RC PF_Manager::GetBlockSize(int &length) const {
    return pBufferMgr->GetBlockSize(length);
}

RC PF_Manager::AllocateBlock(char *&buffer) {
    return pBufferMgr->AllocateBlock(buffer);
}

RC PF_Manager::DisposeBlock(char *buffer) {
    return pBufferMgr->DisposeBlock(buffer);
}









