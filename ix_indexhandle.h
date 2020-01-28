#ifndef IX_FILE_HANDLE_H
#define IX_FILE_HANDLE_H

#include"mybase.h"
#include"rm_rid.h"
#include"pf.h"
#include"ix_error.h"
#include"btree_node.h"
#include<cassert>

struct IX_FileHdr{
    int numPages;///一个文件多少页
    int pageSize; ///一般来说一个pagesize就是一个index节点(b+树节点?)的size
    PageNum rootPage;
    int pairSize; /// (key,RID)这样的对子的size
    int order; ///b树的阶?
    int height;///b树高度
    AttrType attrType;
    int attrLength;
};

const int IX_PAGE_LIST_END=-1;

class IX_IndexHandle{
    friend class IX_Manager;
//    friend class IX_IndexHandleTest;
//    friend class BtreeNodeTest;

public:
    IX_IndexHandle();
    ~IX_IndexHandle();

    RC InsertEntry(void *pData,const RID& rid);
    RC DeleteEntry(void *pData,const RID& rid);
    RC Search(void *pData,RID& rid);
    RC ForcePages();
    RC Open(PF_FileHandle* pfh,int pairSize,PageNum p,int pageSize);
    RC GetFileHeader(PF_PageHandle ph);
    RC SetFileHeader(PF_PageHandle ph)const;

    bool HdrChanged()const{return bHdrChanged;}
    int GetNumPages()const{return hdr.numPages;}
    AttrType GetAttrType()const{return hdr.attrType;}
    int GetAttrLength()const{return hdr.attrLength;}

    RC GetNewPage(PageNum& pageNum);
    RC DisposePage(const PageNum& pageNum);
    RC IsValid()const;

    BtreeNode* FindLeaf(const void *pData);
    BtreeNode* FindSmallestLeaf();
    BtreeNode* FindLargestLeaf();
    BtreeNode* DupScanLeftFind(BtreeNode* right,
                               void *pData,
                               const RID& rid
                               );
    BtreeNode* FindLeafForceRight(const void * pData);
    BtreeNode* FetchNode(RID r)const;
    BtreeNode* FetchNode(PageNum p)const;
    void ResetNode(BtreeNode*& old,PageNum p)const;
    void ResetNode(BtreeNode*& old,RID r)const;

    int GetHeight()const;
    void SetHeight(const int&);

    BtreeNode* GetRoot()const;
    void Print(ostream& os,int level=-1,RID r=RID(-1,-1))const;

    RC Pin(PageNum p);
    RC UnPin(PageNum p);

private:
    RC GetThisPage(PageNum p,PF_PageHandle& ph)const;
    IX_FileHdr hdr;
    bool bFileOpen;
    PF_FileHandle* pfHandle;
    bool bHdrChanged;
    BtreeNode* root;
    BtreeNode** path;///指针数组
    int * pathP;

    ///整棵树最大的键
    void * treeLargest;

};



#endif // IX_FILE_HANDLE_H
