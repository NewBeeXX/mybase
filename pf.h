#ifndef PF_H
#define PF_H

#include "mybase.h"

//页码
typedef int PageNum;
//每页存储的最大字节数
const int PF_PAGE_SIZE=4096-sizeof(int);

class PF_BufferMgr;


///##分页的句柄
class PF_PageHandle{
    friend class PF_FileHandle;
public:
    PF_PageHandle();
    ~PF_PageHandle();

    PF_PageHandle(const PF_PageHandle& pageHandle);
    PF_PageHandle& operator=(const PF_PageHandle& pageHandle);

    RC GetData(char *&pData)const;
    RC GetPageNum(PageNum& pageNum)const;

private:
    int pageNum;
    char *pPageData;
};

///##文件头
struct PF_FileHdr{
    int firstFree; ///链表中的第一个可用空间
    int numPages; ///文件中的分页数
};

///##文件的句柄
///能让你访问文件的page的类
///文件页面控制类
class PF_FileHandle{
    friend class PF_Manager;
public:
    PF_FileHandle();
    ~PF_FileHandle();

    ///拷贝构造
    PF_FileHandle(const PF_FileHandle& fileHandle);
    PF_FileHandle& operator=(const PF_FileHandle& fileHandle);

    ///取第一页,传进分页句柄
    RC GetFirstPage(PF_PageHandle &pageHandle)const;

    ///取下一个有效页
    RC GetNextPage(PageNum current,PF_PageHandle& pageHandle)const;

    ///取传进标号的页面，必须是有效页面
    RC GetThisPage(PageNum pageNum,PF_PageHandle& pageHandle)const;

    RC GetLastPage(PF_PageHandle& pageHandle)const;

    ///取上一个有效页面
    RC GetPrevPage(PageNum current,PF_PageHandle& pageHandle)const;

    ///给文件分配一个新页
    RC AllocatePage(PF_PageHandle& pageHandle);
    ///让一个页面消失
    RC DisposePage(PageNum pageNum);
    ///标记脏页面，在他从缓冲区中删除的时候，它会被写回磁盘，只有脏的会
    RC MarkDirty(PageNum pageNum)const;
    ///取消固定
    RC UnpinPage(PageNum pageNum)const;
    ///清缓冲区 会写回脏页
    RC FlushPages()const;
    ///将脏页写回磁盘
    RC ForcePages(PageNum pageNum=ALL_PAGES)const;

private:
    ///页面是否有效??
    int IsValidPageNum(PageNum PageNum)const;

    PF_BufferMgr *pBufferMgr;
    PF_FileHdr hdr;///文件头
    int bFileOpen;///文件flag
    int bHdrChanged;///脏标记
    int unixfd;/// 系统的文件描述符



};



//整个PF层的管理类，包括管理缓冲区
class PF_Manager{
public:
    PF_Manager();
    ~PF_Manager();
    RC CreateFile(const char *fileName); ///创建新文件
    RC DestroyFile(const char *fileName); ///删除一个文件

    ///打开一个文件
    RC OpenFile(const char *filename,PF_FileHandle& fileHandle);
    ///关闭一个文件
    RC CloseFile(PF_FileHandle &fileHandle);

    ///操控bufferManager的三个方法
    RC ClearBuffer();
    RC PrintBuffer();
    RC ResizeBuffer(int iNewSize);

    ///操控生内存的是三个方法

    ///返回可分配的块大小,注意这里返回值都是RC，即函数是否正常执行，所以要引用
    RC GetBlockSize(int &length)const;

    ///分配内存
    RC AllocateBlock(char *&buffer);

    ///处置一块内存
    RC DisposeBlock(char* buffer);

private:
    PF_BufferMgr *pBufferMgr;

};

void PF_PrintError(RC rc);

#define PF_PAGEPINNED      (START_PF_WARN + 0) // page pinned in buffer
#define PF_PAGENOTINBUF    (START_PF_WARN + 1) // page isn't pinned in buffer
#define PF_INVALIDPAGE     (START_PF_WARN + 2) // invalid page number
#define PF_FILEOPEN        (START_PF_WARN + 3) // file is open
#define PF_CLOSEDFILE      (START_PF_WARN + 4) // file is closed
#define PF_PAGEFREE        (START_PF_WARN + 5) // page already free
#define PF_PAGEUNPINNED    (START_PF_WARN + 6) // page already unpinned
#define PF_EOF             (START_PF_WARN + 7) // end of file
#define PF_TOOSMALL        (START_PF_WARN + 8) // Resize buffer too small
#define PF_LASTWARN        PF_TOOSMALL

#define PF_NOMEM           (START_PF_ERR - 0)  // no memory
#define PF_NOBUF           (START_PF_ERR - 1)  // no buffer space
#define PF_INCOMPLETEREAD  (START_PF_ERR - 2)  // incomplete read from file
#define PF_INCOMPLETEWRITE (START_PF_ERR - 3)  // incomplete write to file
#define PF_HDRREAD         (START_PF_ERR - 4)  // incomplete read of header
#define PF_HDRWRITE        (START_PF_ERR - 5)  // incomplete write to header

// Internal errors
#define PF_PAGEINBUF       (START_PF_ERR - 6) // new page already in buffer
#define PF_HASHNOTFOUND    (START_PF_ERR - 7) // hash table entry not found
#define PF_HASHPAGEEXIST   (START_PF_ERR - 8) // page already in hash table
#define PF_INVALIDNAME     (START_PF_ERR - 9) // invalid PC file name

// Error in UNIX system call or library routine
#define PF_UNIX            (START_PF_ERR - 10) // Unix error
#define PF_LASTERROR       PF_UNIX





#endif // PF_H
