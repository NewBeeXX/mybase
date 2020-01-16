#ifndef PF_BUFFERMGR_H
#define PF_BUFFERMGR_H


#include "pf_internal.h"
#include "pf_hashtable.h"

///pf buffermgr
///用来管理缓冲池的

///标记链表中无前/后节点的常量
#define INVALID_SLOT (-1)

struct PF_BufPageDesc{
    char* pData; ///数据在内存中的位置
    int next; ///链表中下一个节点索引
    int prev; ///上一个
    int bDirty; ///是否脏页
    short int pinCount; ///用来标记是否固定的计数器
    PageNum pageNum; ///分页id
    int fd; ///os的文件描述符
};

///缓冲区管理
class PF_BufferMgr{
public:
    PF_BufferMgr(int numPages);///传入缓冲区页数
    ~PF_BufferMgr();

    ///把id为pagenum的分页读进缓冲区
    RC GetPage(int fd,PageNum pageNum,char** ppBuffer,int bMultiplePins=TRUE);

    ///分配一个新页
    RC AllocatePage(int fd,PageNum pageNum,char** ppBuffer);

    ///标记脏页
    RC MarkDirty(int fd,PageNum pageNum);
    ///取消固定
    RC UnpinPage(int fd,PageNum pageNum);
    ///清除一个文件的所有页
    RC FlushPages(int fd);
    ///把一个页存到disk，但不在缓冲区中去除它
    RC ForcePages(int fd,PageNum pageNum);
    ///清空缓冲区
    RC ClearBuffer();
    ///显示缓冲区的所有实例
    RC PrintBuffer();
    ///改变缓冲区大小
    RC ResizeBuffer(int iNewSize);

    ///操控生内存的三个方法
    ///注意这个类跟file manager有点相似

    ///返回能够被分配的内存块大小
    RC GetBlockSize(int& length)const;

    ///分配内存块
    RC AllocateBlock(char *&buffer);

    ///处置内存块
    RC DisposeBlock(char* buffer);


private:
    RC InsertFree(int slot);///插入可用节点到可用空间链表
    RC LinkHead(int slot);///移存在的页面到头
    RC Unlink(int slot);///删点
    RC InternalAlloc(int &slot);///内部分配一个节点

    ///读一个页面
    RC ReadPage(int fd,PageNum pageNum,char* dest);

    ///写一个页面
    RC WritePage(int fd,PageNum pageNum,char *source);

    ///初始化分页描述符
    RC InitPageDesc(int fd,PageNum pageNum,int slot);

    PF_BufPageDesc *bufTable; ///分页描述表的数组
    PF_HashTable hashTable;
    int numPages; ///有效/总页面数？
    int pageSize; ///分页大小
    int first;///最常使用端
    int last;///最少使用端
    int free;///可用空间的头

};



#endif // PF_BUFFERMGR_H
