#ifndef BTREE_NODE_H
#define BTREE_NODE_H

#include"mybase.h"
#include"pf.h"
#include"rm_rid.h"
#include"ix_error.h"
#include<iosfwd>

class BtreeNode {
//    friend class BtreeNodeTest;
    friend class IX_IndexHandle;


public:

    BtreeNode(AttrType attrType,int attrLength,
              PF_PageHandle& ph,bool newPage=true,
              int pageSize=PF_PAGE_SIZE);
    RC ResetBtreeNode(PF_PageHandle& ph,const BtreeNode& rhs);

    ~BtreeNode();

    int Destroy();
    RC IsValid()const;
    int GetMaxKeys()const;

    int GetNumKeys();
    int SetNumKeys(int newNumKeys);
    int GetLeft();
    int SetLeft(PageNum p);
    int GetRight();
    int SetRight(PageNum p);

    RC GetKey(int pos, void* &key) const;
    int SetKey(int pos, const void* newkey);
    int CopyKey(int pos, void* toKey) const;

    int Insert(const void* newKey,const RID& newrid);
    int Remove(const void* newKey,int kpos=-1);

    int FindKey(const void* &key,const RID& r=RID(-1,-1))const;

    RID FindAddr(const void* &key)const;
    RID GetAddr(const int pos)const;

    int FindKeyPosition(const void* &key)const;
    RID FindAddrAtPosition(const void*& key)const;

    RC Split(BtreeNode* rhs);
    RC Merge(BtreeNode* rhs);

    void Print(ostream& os);
    RID GetPageRID()const;
    void SetPageRID(const RID&);

    int CmpKey(const void* k1,const void* k2)const;
    bool isSorted()const;
    void* LargestKey()const;

private:
    char* keys;
    RID* rids;
    int numKeys;
    int attrLength;
    AttrType attrType;
    int order;
    RID pageRID;





};


#endif // BTREE_NODE_H
