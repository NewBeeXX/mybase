#include"ix_indexhandle.h"
#include"rm_error.h"



IX_IndexHandle::IX_IndexHandle():bFileOpen(false),pfHandle(NULL),
bHdrChanged(false)
 {
     root=NULL;
     path=NULL;
     pathP=NULL;
     treeLargest=NULL;
     hdr.height=0;
}

IX_IndexHandle::~IX_IndexHandle() {
    if(root!=NULL&&pfHandle!=NULL){
        pfHandle->UnpinPage(hdr.rootPage);
        delete root;
        root=NULL;
    }
    if(pathP!=NULL){
        delete [] pathP;
        pathP=NULL;
    }
    if(path!=NULL){
///        path[0]是根
        for(int i=1;i<hdr.height;i++){
            if(path[i]!=NULL&&pfHandle!=NULL){
                pfHandle->UnpinPage(path[i]->GetPageRID().Page());
            }
        }
        delete [] path;
        path=NULL;
    }

    if(pfHandle!=NULL){
        delete pfHandle;
        pfHandle=NULL;
    }
    if(treeLargest!=NULL){
        delete [] (char*)treeLargest;
        treeLargest=NULL;
    }
}

RC IX_IndexHandle::InsertEntry(void* pData, const RID& rid) {
    RC invalid=IsValid();
    if(invalid)return invalid;

    if(pData==NULL)return IX_BADKEY;

    bool newLargest=false;
    void * prevKey=NULL;
    int level=hdr.height-1;
    BtreeNode* node=FindLeaf(pData);
    BtreeNode* newNode=NULL;

    ///不能存在过这个rid
    int pos2=node->FindKey((const void*&)pData,rid);
    if(pos2!=-1)return IX_ENTRYEXISTS;

    ///有新的最大值
    if(node->GetNumKeys()==0||
       node->CmpKey(pData,treeLargest)>0
       ){
        newLargest=true;
        prevKey=treeLargest;
       }
    int result=node->Insert(pData,rid);
    if(newLargest){
        for(int i=0;i<hdr.height-1;i++){
            int pos=path[i]->FindKey((const void*&)prevKey);
            if(pos!=-1)path[i]->SetKey(pos,pData);
            else{
                cerr<<"什么鬼?? path维护的就是最大值的路径 居然会找不到了?"<<endl;
            }
        }

        memcpy(treeLargest,pData,hdr.attrLength);
    }


    ///没有空位
    void* failedKey=pData;
    RID failedRid=rid;
    while(result==-1){
        char* charPtr=new char[hdr.attrLength];
        void* oldLargest=charPtr;
        if(node->LargestKey()==NULL)oldLargest=NULL;
        else node->CopyKey(node->GetNumKeys()-1,oldLargest);

        delete [] charPtr;

        PF_PageHandle ph;
        PageNum p;
        RC rc;
        if((rc=GetNewPage(p))||
           (rc=GetThisPage(p,ph))
           )return rc;

        newNode=new BtreeNode(hdr.attrType,hdr.attrLength,ph,true,hdr.pageSize);
        ///从node分裂一个节点到newNode
        if(rc=node->Split(newNode))return IX_PF;
        BtreeNode* currentRight=FetchNode(newNode->GetRight());
        if(currentRight!=NULL){
            currentRight->SetLeft(newNode->GetPageRID().Page());
            delete currentRight;
        }

        BtreeNode* nodeInsertedInto=NULL;

        if(node->CmpKey(pData,node->LargestKey())>=0){
            ///大就插右边
            newNode->Insert(failedKey,failedRid);
            nodeInsertedInto=newNode;
        }else{
            node->Insert(failedKey,failedRid);
            nodeInsertedInto=node;
        }
        level--;
        if(level<0)break;
        int posAtParent=pathP[level];
        BtreeNode* parent=path[level];


        ///这里是想修改原本父节点key，但是它node没有写这个函数
        ///取而代之的是删了再插
        ///移除旧key
        parent->Remove(NULL,posAtParent);
        result=parent->Insert((const void*)node->LargestKey(),
                              node->GetPageRID()
                              );

        ///把右边这个节点的最大key也插到父亲
        result=parent->Insert(newNode->LargestKey(),newNode->GetPageRID());

        node=parent;
        failedKey=newNode->LargestKey();
        failedRid=newNode->GetPageRID();

        delete newNode;
        newNode=NULL;
    }


    if(level>=0){
        ///这里说明在根节点及以下都插入成功了
        return 0;
    }else{
        ///根节点分裂了。。。
        RC rc=pfHandle->UnpinPage(hdr.rootPage);
        if(rc<0)return rc;
        PF_PageHandle ph;
        PageNum p;
        rc=GetNewPage(p);
        if(rc)return IX_PF;
        rc=GetThisPage(p,ph);
        if(rc)return IX_PF;

        root=new BtreeNode(hdr.attrType,hdr.attrLength,
                           ph,true,
                           hdr.pageSize
                           );
        root->Insert(node->LargestKey(),node->GetPageRID());
        root->Insert(newNode->LargestKey(),newNode->GetPageRID());

        hdr.rootPage=root->GetPageRID().Page();
        PF_PageHandle rootph;
        rc=pfHandle->GetThisPage(hdr.rootPage,rootph);
        if(rc)return rc;
        if(newNode){
            delete newNode;
            newNode=NULL;
        }
        SetHeight(hdr.height+1);
        return 0;
    }

}

///暂且不知道实际用途
///这个传入的rid虽然是引用,但是是用来判断一个参数并不是返回值
///不过通过读代码猜测在right节点左边依次往左遍历key ，找到与right的某个key相等的 并且rid==rid的那个key
///返回的是该key所在的node指针
BtreeNode* IX_IndexHandle::DupScanLeftFind(BtreeNode* right, void* pData, const RID& rid) {
    BtreeNode * currNode=FetchNode(right->GetLeft());
    int currPos=-1;
    for(BtreeNode* j=currNode;j!=NULL;j=FetchNode(j->GetLeft())){
        currNode=j;
        for(int i=currNode->GetNumKeys()-1;i>=0;i--){
            currPos=i;
            char* key=NULL;
            ///注意这里是void* &
            int ret=currNode->GetKey(i,(void*&)key);
            if(ret==-1)break;
            int f=currNode->CmpKey(pData,key);
            if(f<0)return NULL;
            else if(f>0)return NULL;///不应该发生
            else{
                if(currNode->GetAddr(currPos)==rid)return currNode;
            }
        }
    }
    return NULL;
}

///删除一个
///这里虽然传入一个引用 但是它不是返回值
RC IX_IndexHandle::DeleteEntry(void* pData, const RID& rid) {
    if(pData==NULL)return IX_BADKEY;
    RC invalid = IsValid(); if(invalid) return invalid;
    bool nodeLargest=false;
    ///这里猜测这个findleaf是找到这个key的最右出现(有可能只是一个lowerbound)
    BtreeNode* node=FindLeaf(pData);
    int pos=node->FindKey((const void*&)pData,rid);
    ///pos==-1说明在这个节点没有找到key和rid都相等的，但是并不一定是没有
    ///可能出现在左边的节点中
    if(pos==-1){
        //往左边找
        ///这里就是为了找到最右边的那个 所以-1-1
        int p=node->FindKey((const void*&)pData,RID(-1,-1));
        ///如果可以说明存在这个key,就尝试一下往左边找找
        if(p!=-1){
            ///这个dupscanfind就是往左找rid和key都符合的第一个节点
            BtreeNode* other=DupScanLeftFind(node,pData,rid);
            if(other!=NULL){
                int pos=other->FindKey((const void*&)pData,rid);
                other->Remove(pData,pos);///这里不管是否达到下界
            }
        }
        ///这个key都没出现过，更别说要rid符合了
        return IX_NOSUCHENTRY;
    }else if(pos==node->GetNumKeys()-1){
        //用lowerbound就已经能够直接找到这一个key了,
        ///并且这里是这个节点的最大key
        nodeLargest=true;
    }

    ///个人觉得这个处理因删除下层节点最大值而造成的需要更改上层节点key的操作有部分错误
    ///首先你只有下层节点的最大值被删/改了，上层节点才需要更改key
    ///这个代码对于删掉叶子最大值 其父中对应key不是其父这个节点的最大值时，是正确的
    ///因为改掉其父的key就不用再继续了
    ///但是 这里为什么会sb的对于叶子的父亲的父亲及以上的节点,只有当该key是最大值的时候才改呢
    ///你这个key如果是在中间就不用改了吗，下层的最大值都已经改了 你上层必要改呀
    if(nodeLargest){
        for(int i=hdr.height-2;i>=0;i--){
            int pos=path[i]->FindKey((const void*&)pData);
            if(pos!=-1){
                void* leftKey=NULL;
                leftKey=path[i+1]->LargestKey();
                ///这里的if就是对叶子的父亲有用而已
                if(node->CmpKey(pData,leftKey)==0){
                    int pos=path[i+1]->GetNumKeys()-2;
                    if(pos<0)continue;///这tm都没有更小的key了，直接就不改？
                    path[i+1]->GetKey(pos,leftKey);
                }
                if(i==hdr.height-2||pos==path[i]->GetNumKeys()-1)
                    path[i]->SetKey(pos,leftKey);

            }
        }
    }

    int result=node->Remove(pData,pos);
    int level=hdr.height-1;
    while(result==-1){
        if(--level<0)break;///如果根节点都删除得只剩<=1个节点
        int posAtParent=pathP[level];
        BtreeNode* parent=path[level];
        result=parent->Remove(NULL,posAtParent);
        ///对于根节点 如果key只剩一个也要把result置为-1
        if(level==0&&parent->GetNumKeys()==1&&result==0)result=-1;


        BtreeNode *left=FetchNode(node->GetLeft());
        BtreeNode *right=FetchNode(node->GetRight());
        if(left!=NULL){
            if(right!=NULL)left->SetRight(right->GetPageRID().Page());
            else left->SetRight(-1);
        }
        if(right!=NULL){
            if(left!=NULL)right->SetLeft(left->GetPageRID().Page());
            else right->SetLeft(-1);
        }

        ///我算是看明白了，Fetch的节点是new出来的
        if(right!=NULL)delete right;
        if(left!=NULL)delete left;

        node->Destroy();
        RC rc=DisposePage(node->GetPageRID().Page());
        if(rc<0)return IX_PF;
        node=parent;

    }

    if(level>=0){
        ///正常完成删除 滚
        return 0;
    }else{
        ///如果高度就是1，那没有必要删这个root
        if(hdr.height==1)return 0;

        ///否则就让它的唯一的儿子作为root 把它删了
        root=FetchNode(root->GetAddr(0));///第一个儿子
        hdr.rootPage=root->GetPageRID().Page();
        PF_PageHandle rootph;
        RC rc=pfHandle->GetThisPage(hdr.rootPage,rootph);
        if(rc)return rc;
        node->Destroy();
        rc=DisposePage(node->GetPageRID().Page());
        if(rc<0)return IX_PF;
        SetHeight(hdr.height-1);
        return 0;
    }



}


RC IX_IndexHandle::Pin(PageNum p) {
    PF_PageHandle ph;
    RC rc=pfHandle->GetThisPage(p,ph);
    return rc;
}

RC IX_IndexHandle::UnPin(PageNum p) {
    RC rc=pfHandle->UnpinPage(p);
    return rc;
}

///这个getpage的作用就是先用phhandle get然后再markdirty再unpinpage
RC IX_IndexHandle::GetThisPage(PageNum p, PF_PageHandle& ph) const{
    RC rc=pfHandle->GetThisPage(p,ph);
    if(rc)return rc;
    rc=pfHandle->MarkDirty(p);
    if(rc)return rc;
    rc=pfHandle->UnpinPage(p);
    if(rc)return rc;
    return 0;
}

RC IX_IndexHandle::Open(PF_FileHandle* pfh, int pairSize,
                        PageNum rootPage, int pageSize) {
    if(bFileOpen||pfHandle!=NULL)return IX_HANDLEOPEN;
    if(pfh==NULL||pairSize<=0)return IX_BADOPEN;
    bFileOpen=true;
    pfHandle=new PF_FileHandle;
    *pfHandle=*pfh;

    PF_PageHandle ph;
    ///分页0是索引的头header
    GetThisPage(0,ph);
    this->GetFileHeader(ph);

    PF_PageHandle rootph;
    bool newPage=true;

    ///如果原本有根
    if(hdr.height>0){
        SetHeight(hdr.height);
        newPage=false;
        RC rc=GetThisPage(hdr.rootPage,rootph);
        if(rc)return rc;
    }else{
        PageNum p;
        RC rc=GetNewPage(p);
        if(rc)return rc;
        rc=GetThisPage(p,rootph);
        if(rc)return rc;
        hdr.rootPage=p;
        SetHeight(1);
    }

    RC rc=pfHandle->GetThisPage(hdr.rootPage,rootph);
    if(rc)return rc;

    root=new BtreeNode(hdr.attrType,hdr.attrLength,
                       rootph,newPage,
                       hdr.pageSize
                       );
    path[0]=root;
    hdr.order=root->GetMaxKeys();
    bHdrChanged=true;
    RC invalid = IsValid(); if(invalid) return invalid;
    treeLargest=(void*)new char[hdr.attrLength];
    if(!newPage){
        BtreeNode* node=FindLargestLeaf();
        if(node->GetNumKeys()>0)
            node->CopyKey(node->GetNumKeys()-1,treeLargest);
    }
    return 0;

}

RC IX_IndexHandle::GetFileHeader(PF_PageHandle ph) {
    char* buf;
    RC rc=ph.GetData(buf);
    if(rc)return rc;
    memcpy(&hdr,buf,sizeof(hdr));
    return 0;
}

RC IX_IndexHandle::SetFileHeader(PF_PageHandle ph)const {
    char* buf;
    RC rc=ph.GetData(buf);
    if(rc)return rc;
    memcpy(buf,&hdr,sizeof(hdr));
    return 0;
}

RC IX_IndexHandle::ForcePages() {
    RC invalid = IsValid(); if(invalid) return invalid;
    return pfHandle->ForcePages(ALL_PAGES);
}

RC IX_IndexHandle::IsValid() const{
    bool ret=pfHandle!=NULL;
    if(hdr.height>0){
        ret=ret&&(hdr.rootPage>0)&&(hdr.numPages>=hdr.height+1)&&(root!=NULL)&&(path!=NULL);
    }
    return ret?0:IX_BADIXPAGE;
}

RC IX_IndexHandle::GetNewPage(PageNum& pageNum) {
    RC invalid = IsValid(); if(invalid) return invalid;
    PF_PageHandle ph;
    RC rc;
    if((rc=pfHandle->AllocatePage(ph))||(rc=ph.GetPageNum(pageNum)))return rc;

    if(rc=pfHandle->UnpinPage(pageNum))return rc;
    hdr.numPages++;
    bHdrChanged=true;
    return 0;
}

RC IX_IndexHandle::DisposePage(const PageNum& pageNum) {
    RC invalid = IsValid(); if(invalid) return invalid;
    RC rc;
    if(rc=pfHandle->DisposePage(pageNum))return rc;
    hdr.numPages--;
    bHdrChanged=true;
    return 0;
}

BtreeNode* IX_IndexHandle::FindSmallestLeaf() {
    RC invalid = IsValid(); if(invalid) return NULL;
    if(root==NULL)return NULL;
    RID addr;
    if(hdr.height==1){
        path[0]=root;
        return root;
    }
    for(int i=1;i<hdr.height;i++){
        RID r=path[i-1]->GetAddr(0);
        if(r.Page()==-1){
            ///根本就没有指针
            return NULL;
        }
        if(path[i]!=NULL){///要把之前的节点给unpin回disk
            RC rc=pfHandle->UnpinPage(path[i]->GetPageRID().Page());
            if(rc<0){
                PrintErrorAll(rc);
                return NULL;
            }
            delete path[i];
            path[i]=NULL;
        }
        path[i]=FetchNode(r);
        PF_PageHandle dummy;
        RC rc=pfHandle->GetThisPage(path[i]->GetPageRID().Page(),dummy);
        if(rc)return NULL;
        pathP[i-1]=0;
    }
    return path[hdr.height-1];
}

BtreeNode* IX_IndexHandle::FindLargestLeaf() {
    RC invalid = IsValid(); if(invalid) return NULL;
    if(root==NULL)return NULL;
    RID addr;
    if(hdr.height==1){
        path[0]=root;
        return root;
    }
    for(int i=1;i<hdr.height;i++){
        RID r=path[i-1]->GetAddr(path[i-1]->GetNumKeys()-1);
        if(r.Page()==-1){
                ///按理说应该是不会进来这里的
            return NULL;
        }
        if(path[i]!=NULL){
            ///把原本的写回disk
            RC rc=pfHandle->UnpinPage(path[i]->GetPageRID().Page());
            if(rc<0)return NULL;
            delete path[i];
            path[i]=NULL;
        }
        path[i]=FetchNode(r);
        PF_PageHandle dummy;
        RC rc=pfHandle->GetThisPage(path[i]->GetPageRID().Page(),dummy);
        if(rc)return NULL;
        ///原代码中这里感觉写错了,它赋0? 最小赋0可以理解 最大也是??
        ///改了
        pathP[i-1]=path[i-1]->GetNumKeys()-1;
    }
    return path[hdr.height-1];
}


///这个find就是找这个pdata(key)的最右出现位置node
///其实这个find不一定是找一个一定存在的节点
///它也用于insert过程,所以它也是可以找一个应该插入的节点
BtreeNode* IX_IndexHandle::FindLeaf(const void* pData) {
    RC invalid = IsValid(); if(invalid) return NULL;
    if(root==NULL)return NULL;
    RID addr;
    if(hdr.height==1){
        path[0]=root;
        return root;
    }

    for(int i=1;i<hdr.height;i++){
        ///这个findaddratpos就是找到这个pdata的最右出现位置 如果没有 那么会返回>它的第一个位置
        ///可以想一下 这个函数 对于中间节点的查找是没错的
        RID r=path[i-1]->FindAddrAtPosition(pData);
        ///这个findkeypos就是被上面的findaddr调用的，也是有==找最右 没有找>的第一个
        int pos=path[i-1]->FindKeyPosition(pData);
        if(r.Page()==-1){
            ///pdata大于任何节点 找不到
            ///所以就还是走最大的路径
            ///!!!其实这样也是合理的因为这个函数也要用于插入时的find，这个key可以没存在过
            ///插入的key如果是最大的，那么明显还是得找到最大的那个节点
            const void* p=path[i-1]->LargestKey();
            r=path[i-1]->FindAddr((const void*&)p);
            pos=path[i-1]->FindKey((const void*&)p);
        }
        if(path[i]!=NULL){
                ///还是把原本的弄回disk
            RC rc=pfHandle->UnpinPage(path[i]->GetPageRID().Page());
            delete path[i];
            path[i]=NULL;
        }
        path[i]=FetchNode(r.Page());
        PF_PageHandle dummy;
        RC rc=pfHandle->GetThisPage(path[i]->GetPageRID().Page(),dummy);
        if(rc)return NULL;
        pathP[i-1]=pos;
    }
    return path[hdr.height-1];
}

BtreeNode* IX_IndexHandle::FetchNode(PageNum p)const {
    return FetchNode(RID(p,-1));
}

///果然是new出来的
BtreeNode* IX_IndexHandle::FetchNode(RID r)const {
    RC invalid = IsValid(); if(invalid) return NULL;
    if(r.Page()<0)return NULL;
    PF_PageHandle ph;
    RC rc=GetThisPage(r.Page(),ph);
    if(rc){
        PrintErrorAll(rc);
        return NULL;
    }
    ///创建b树节点会把pagehandle的东西copy过去
    return new BtreeNode(hdr.attrType,hdr.attrLength,
                         ph,false,
                         hdr.pageSize);
}


///这个是准确查找,必须要pdata一致 虽然它调用的不准确的findleaf
RC IX_IndexHandle::Search(void* pData, RID& rid) {
    RC invalid = IsValid(); if(invalid) return invalid;
    if(pData==NULL)return IX_BADKEY;
    BtreeNode* leaf=FindLeaf(pData);
    if(leaf==NULL)return IX_BADKEY;
    rid=leaf->FindAddr((const void*&)pData);
    if(rid==RID(-1,-1))return IX_KEYNOTFOUND;
    return 0;
}



int IX_IndexHandle::GetHeight()const {
    return hdr.height;
}

///改变height的话 需要更改path和pathP数组的大小
void IX_IndexHandle::SetHeight(const int& h) {
    for(int i=1;i<hdr.height;i++)if(path!=NULL&&path[i]!=NULL){
        delete path[i];
        path[i]=NULL;
    }
    if(path!=NULL)delete [] path;
    if(pathP!=NULL)delete [] pathP;
    hdr.height=h;
    path=new BtreeNode* [hdr.height];
    for(int i=1;i<hdr.height;i++)path[i]=NULL;
    path[0]=root;
    ///这里height-1是因为叶子节点不需要记录儿子是哪一个(没有儿子)
    pathP=new int[hdr.height-1];
    for(int i=0;i<hdr.height-1;i++)pathP[i]=-1;

}

BtreeNode* IX_IndexHandle::GetRoot()const {
    return root;
}

///这个print就是dfs这颗b+树
///十分sb 他的level首先传入-1标识开始递归打印这颗b+树
///然后一共height层 首先level是height 叶子是1层 用0层标识不再向下递归
void IX_IndexHandle::Print(ostream& os, int level, RID r)const {
    RC invalid = IsValid(); if(invalid) assert(invalid);
    BtreeNode* node=NULL;
    if(level==-1){
        level=hdr.height;
        node=FetchNode(root->GetPageRID());
    }else{
        ///这个人写的代码废话还是挺多的,level在1的时候就已经不会递归 更不用说0了
        if(level<1)return;
        else node=FetchNode(r);
    }

    node->Print(os);
    if(level>=2){
        for(int i=0;i<node->GetNumKeys();i++){
            RID newr=node->GetAddr(i);
            Print(os,level-1,newr);
        }
    }
    if(level==1&&node->GetRight()==-1)os<<endl;
    if(node!=NULL)delete node;
}

///原代码中说此函数是因为findleaf不能找到最右key所在节点
///所以用这个函数来保证找到最右key
///名字里有个force 估计是暴力
///一开始我不太相信findleaf不能找到这个key的最右节点
///后来发现我想错了

///!!!如果相同key没有跨越节点,即只在一个节点中出现 ,那是没错的，findleaf就是找到最右节点
///!!!又即便跨越了节点，假如这个key的最右出现是一个节点的最大值,那么findleaf也是没有错 找到最右
///唯一找不到最右的是这个最右key前面出现了，且它在它所在的节点中不是最大key
///所以findleaf会找到前面那个 节点，因为这个节点在它父亲那里 可以找到==， 查找算法就会找到==的最右
///但是却没有找到真正的最右
///我曾想过这种情况不会出现，因为假如一个key当前是一个节点的最大值，那插入一个与其相同的key，如果没满
///还是会插到这个点 但是满了？ 插到右边 那它在右边还是最大值，但是这时如果插入一个更大的值
///假设插入的是比正科树都大的值且就插进右边(这是必然 insert时调用findleaf就会找到这个点)
///那么就可以造成上面这种情况

BtreeNode* IX_IndexHandle::FindLeafForceRight(const void* pData) {
    FindLeaf(pData);
    if(path[hdr.height-1]->GetRight()!=-1){
        int pos=path[hdr.height-1]->FindKey(pData);
        if(pos!=-1&&pos==path[hdr.height-1]->GetNumKeys()-1){
            ///只有这样才说明有可能右边还有
            BtreeNode* r=FetchNode(path[hdr.height-1]->GetRight());
            if(r!=NULL){
                void* k=NULL;
                r->GetKey(0,k);
                if(r->CmpKey(k,pData)==0){
                    ///右边的确还有
                    RC rc=pfHandle->UnpinPage(path[hdr.height-1]->GetPageRID().Page());
                    if(rc<0){
                        PrintErrorAll(rc);
                        return NULL;
                    }
                    delete path[hdr.height-1];
                    path[hdr.height-1]=FetchNode(r->GetPageRID());
                    pathP[hdr.height-2]++;
                    cerr<<"让我看看右边还有相同key的情况是不是真的出现了"<<endl;
                }

            }
        }

    }
    return path[hdr.height-1];
}





void IX_IndexHandle::ResetNode(BtreeNode*& old, PageNum p) const{

}

void IX_IndexHandle::ResetNode(BtreeNode*& old, RID r) const{

}











