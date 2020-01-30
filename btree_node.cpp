#include"btree_node.h"
#include"pf.h"
#include<cstdlib>


BtreeNode::BtreeNode(AttrType attrType,int attrLength,
                     PF_PageHandle& ph,bool newPage,
                     int pageSize
                    )
    :keys(NULL),rids(NULL),attrLength(attrLength),attrType(attrType) {

    order=floor( (pageSize+sizeof(numKeys)+2*sizeof(PageNum)) /
                 (sizeof(RID)+attrLength)
               );
    ///order就是阶数 要存order个key和order+1个指针还有一个key的数量，下面就是在计算order,2*sizeof(Pagenum)就是哪一个额外的指针
    while( order*(attrLength+sizeof(RID))+sizeof(numKeys)+2*sizeof(PageNum) > pageSize )
        order--;

    char* pData=NULL;
    RC rc;
    if(rc=ph.GetData(pData))return ;
    PageNum p;

    ph.GetPageNum(p);
    SetPageRID(RID(p,-1));

    ///这个就是存放key的起始位置
    keys=pData;
    ///存放指针的起始位置
    rids=(RID*)(pData+attrLength*(order));

    if(!newPage) {
        numKeys=0;
        GetNumKeys();
        GetLeft();
        GetRight();
    } else {
        SetNumKeys(0);
        SetLeft(-1);
        SetRight(-1);
    }
}

BtreeNode::~BtreeNode() {

}

///用传入的两个东西来重置当前节点
RC BtreeNode::ResetBtreeNode(PF_PageHandle& ph, const BtreeNode& rhs) {
    order=rhs.order;
    attrLength=rhs.attrLength;
    attrType=rhs.attrType;
    numKeys=rhs.numKeys;
    char* pData=NULL;
    RC rc;
    if(rc=ph.GetData(pData))
        return rc;

    PageNum p;
    if(rc=ph.GetPageNum(p))return rc;
    SetPageRID(RID(p,-1));
    keys=pData;
    rids=(RID*)(pData+attrLength*(order));

    GetNumKeys();
    GetLeft();
    GetRight();
    return 0;

}



int BtreeNode::Destroy() {
    if(numKeys!=0)return -1;
    keys=NULL;
    rids=NULL;
    return 0;
}



///看到这里应该就知道 numkey存在指针后面
int BtreeNode::GetNumKeys() {
    void* loc=(char*)rids + sizeof(RID) * order;
    return numKeys=*(int*)loc;
}

int BtreeNode::SetNumKeys(int newNumKeys) {
    memcpy((char*)rids+sizeof(RID)*order,&newNumKeys,sizeof(int));
    numKeys=newNumKeys;
    return 0;
}

///好像numkeys后面是一个叫做left的东西 left是 一个pagenum？？？
int BtreeNode::GetLeft() {
    void* loc=(char*)rids+sizeof(RID)*order+sizeof(int);
    return *(PageNum*)loc;
}

int BtreeNode::SetLeft(PageNum p) {
    memcpy((char*)rids+sizeof(RID)*order+sizeof(int),&p,sizeof(PageNum));
    return 0;
}

///所以再后面紧跟的是right
int BtreeNode::GetRight() {
    void * loc=(char*)rids+sizeof(RID)*order+sizeof(int)+sizeof(PageNum);
    return *(PageNum*)loc;
}

int BtreeNode::SetRight(PageNum p) {
    memcpy((char*)rids+sizeof(RID)*order+sizeof(int)+sizeof(PageNum),&p,sizeof(PageNum));
    return 0;
}


RID BtreeNode::GetPageRID()const {
    return pageRID;
}

void BtreeNode::SetPageRID(const RID& r) {
    pageRID=r;
}

RC BtreeNode::IsValid()const {
    if(order<=0)return IX_INVALIDSIZE;
    bool ret=(keys!=NULL)&&(rids!=NULL)&&(numKeys>=0)&&(numKeys<=order);
    if(!ret)cerr<<"b树节点 invalid order:"<<order<<" numKeys:"<<numKeys<<endl;
    return ret?0:IX_BADIXPAGE;
}

///其实就是返回order
int BtreeNode::GetMaxKeys() const{
    return order;
}

void* BtreeNode::LargestKey() const{
    void* key=NULL;
    if(numKeys>0){
        GetKey(numKeys-1,key);
        return key;
    }else{
        cerr<<"调用了largestKey当b树节点key个数<=0"<<endl;
        return NULL;
    }
}

RC BtreeNode::GetKey(int pos, void*& key) const{
    if(pos>=0&&pos<numKeys){
        key=keys+attrLength*pos;
        return 0;
    }else return -1;
}

int BtreeNode::CopyKey(int pos, void* toKey) const{
    if(toKey==NULL)return -1;
    if(pos>=0&&pos<order){
        memcpy(toKey,keys+attrLength*pos,attrLength);
        return 0;
    }else return -1;
}

int BtreeNode::SetKey(int pos, const void* newkey) {
    if(newkey==(keys+attrLength*pos))return 0;
    if(pos>=0&&pos<order){
        memcpy(keys+attrLength*pos,newkey,attrLength);
        return 0;
    }else return -1;
}



int BtreeNode::Insert(const void* newKey, const RID& rid) {
    if(numKeys>=order)return -1;
    ///没位置

    ///下面是个插入排序
    int i=-1;
    void *currKey=NULL;
    for(i=numKeys-1;i>=0;i--){
        GetKey(i,currKey);
        if(CmpKey(newKey,currKey)>=0)break;
        rids[i+1]=rids[i];
        SetKey(i+1,currKey);
    }
    rids[i+1]=rid;
    SetKey(i+1,newKey);

    if(!isSorted()){
        cerr<<"插入排序没拍好"<<endl;
    }
    SetNumKeys(GetNumKeys()+1);
    return 0;

}

///就是给定这个节点内的一个key，删掉它，把后面的前移
int BtreeNode::Remove(const void* newKey, int kpos) {

    int pos=-1;
    if(kpos!=-1){
        if(kpos<0||kpos>=numKeys)return -2;
        pos=kpos;
    }else{
        pos=FindKey(newKey);
        if(pos<0)return -2;
    }

    for(int i=pos;i<numKeys-1;i++){
        void* p;
        GetKey(i+1,p);
        SetKey(i,p);
        rids[i]=rids[i+1];
    }
    SetNumKeys(GetNumKeys()-1);
    if(numKeys==0)return -1;
    return 0;
}

///给一个key 查找返回它的rid 当然如果没有这个key则返回第一个>key的rid
RID BtreeNode::FindAddrAtPosition(const void*& key) const{
    int pos=FindKeyPosition(key);
    if(pos==-1||pos>=numKeys)return RID(-1,-1);
    return rids[pos];
}

///如果有== 返回==的最后一个位置，如果没有，返回>key的最左位置
int BtreeNode::FindKeyPosition(const void*& key) const{
    for(int i=numKeys-1;i>=0;i--){
        void* k;
        if(GetKey(i,k)!=0)return -1;
        if(CmpKey(key,k)==0)return i;
        if(CmpKey(key,k)>0)return i+1;
    }
    return 0;
}

RID BtreeNode::GetAddr(const int pos) const{
    if(pos<0||pos>=numKeys)return RID(-1,-1);
    return rids[pos];
}

RID BtreeNode::FindAddr(const void*& key)const {
    int pos=FindKey(key);
    if(pos==-1)return RID(-1,-1);
    return rids[pos];
}

///给一个key 查找pos
///如果传入的rid不是规定的 那就是要求key==key rid==r 否则找到最后一个key相等的即刻
int BtreeNode::FindKey(const void*& key, const RID& r) const{
    for(int i=numKeys-1;i>=0;i--){
        void* k;
        if(GetKey(i,k))return -1;
        if(CmpKey(key,k)==0){
            if(r==RID(-1,-1))return i;
            else {
                if(rids[i]==r)return i;
            }
        }
    }
    return -1;
}

int BtreeNode::CmpKey(const void* a,const void* b)const{
    if(attrType==STRING){
        return memcmp(a,b,attrLength);
    }
    if(attrType==FLOAT){
        if(*(float*)a>*(float*)b)return 1;
        if(*(float*)a==*(float*)b)return 0;
        if(*(float*)a<*(float*)b)return -1;
    }
    if(attrType==INT){
        if(*(int*)a>*(int*)b)return 1;
        if(*(int*)a==*(int*)b)return 0;
        if(*(int*)a<*(int*)b)return -1;
    }
    ///走到这里出错
    return 0;
}



bool BtreeNode::isSorted() const{
    for(int i=0;i<numKeys-2;i++){
        void *k1,*k2;
        GetKey(i,k1);
        GetKey(i+1,k2);
        if(CmpKey(k1,k2)>0)return false;
    }
    return true;
}



///分裂出一个节点存到rhs
RC BtreeNode::Split(BtreeNode* rhs) {
    ///第一个要移到rhs的位置
    ///这里保证原本这个节点剩余key数>=order/2
    int firstMovedPos=(numKeys+1)/2;
    ///移过去的key数
    int moveCount=numKeys-firstMovedPos;
    if(rhs->GetNumKeys()+moveCount>rhs->GetMaxKeys())return -1;
    for(int pos=firstMovedPos;pos<numKeys;pos++){
        RID r=rids[pos];
        void *k=NULL;this->GetKey(pos,k);
        RC rc=rhs->Insert(k,r);
        if(rc)return rc;
    }
    ///把原本的key删掉 这里改了一下
    for(int i=numKeys-1;i>=firstMovedPos;i--){
        RC rc=this->Remove(NULL,i);
        if(rc<-1)return rc;
    }
    rhs->SetRight(this->GetRight());
    this->SetRight(rhs->GetPageRID().Page());
    rhs->SetLeft(this->GetPageRID().Page());

    return 0;
}

///合并other到自己
///当然这个other肯定跟自己相邻 要么在左要么右
RC BtreeNode::Merge(BtreeNode* other) {
    if(numKeys+other->GetNumKeys()>order)return -1;
    for(int pos=0;pos<other->GetNumKeys();pos++){
        void* k=NULL;
        other->GetKey(pos,k);
        RID r=other->GetAddr(pos);
        RC rc=this->Insert(k,r);
        if(rc)return rc;
    }

    int moveCount=other->GetNumKeys();
    ///这里也改了一下
    for(int i=moveCount-1;i>=0;i--){
        RC rc=other->Remove(NULL,i);
        if(rc<-1)return rc;
    }

    if(this->GetPageRID().Page()==other->GetLeft())
        this->SetRight(other->GetRight());
    else this->SetLeft(other->GetLeft());

    return 0;
}

///打印节点上的key以及对应指针(叶子节点)
void BtreeNode::Print(ostream& os) {
    os<<GetLeft()<<"<--"<<pageRID.Page()<<"{";

    for(int pos=0;pos<GetNumKeys();pos++){
        void* k=NULL;
        GetKey(pos,k);
        os<<"(";
        if(attrType==INT)os<< *((int*)k);
        else if(attrType==FLOAT)os<< *((float*)k);
        else if(attrType==STRING){
            for(int i=0;i<attrLength;i++)os<<((char*)k)[i];
        }
        os<<","<<GetAddr(pos)<<"), ";
    }
    if(numKeys>0)os<<"\b\b";
    os<<"}"<<"-->"<<GetRight()<<"\n";
}




