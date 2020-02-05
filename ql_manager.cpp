#include<cstdio>
#include<iostream>
#include<algorithm>
#include<sys/time.h>
#include<sys/types.h>
#include<cassert>
#include<unistd.h>
#include"mybase.h"
#include"ql.h"
#include"sm.h"
#include"ix.h"
#include"rm.h"
#include"iterator.h"
#include"index_scan.h"
#include"file_scan.h"
#include"nested_loop_join.h"
#include"nested_loop_index_join.h"
#include"merge_join.h"
#include"sort.h"
#include"agg.h"
#include"parser.h"
#include"projection.h"
#include<map>
#include<vector>
#include<functional>

using namespace std;

namespace {
bool strlt(char* a,char* b) {
    return strcmp(a,b)<0;
}
bool streq(char* a,char* b) {
    return strcmp(a,b)==0;
}

class npageslt: public std::binary_function<char*,char*,bool> {
public:
    npageslt(const SM_Manager& smm):psmm(&smm) {}
    inline bool operator()(char* a,char* b) {
        return psmm->GetNumPages(a)<psmm->GetNumPages(b);
    }
private:
    const SM_Manager* psmm;
};

class nrecslt:public std::binary_function<char*,char*,bool> {
public:
    nrecslt(const SM_Manager& smm):psmm(&smm) {}
    inline bool operator()(char* a,char* b) {
        return psmm->GetNumrecords(a)<psmm->GetNumRecords(b);
    }
private:
    const SM_Manager* psmm;
};

};


QL_Manager::QL_Manager(SM_Manager& smm, IX_Manager& ixm, RM_Manager& rmm)
    :rmm(rmm),ixm(ixm),smm(smm) {
}

RC QL_Manager::IsValid() const {
    return smm.IsValid()==0?0:QL_BADOPEN;
}

QL_Manager::~QL_Manager() {

}

RC QL_Manager::Select(int nSelAttrs, const AggRelAttr selAttrs_[], ///被选中的属性
                      int nRelations, const char* const relations_[], /// 关系
                      int nConditions, const Condition conditions_[], ///条件
                      int order, RelAttr orderAttr, bool group, RelAttr groupAttr) {
                      ///是否排序 以及要排序的关系 是否分组以及分组的属性

    RC invalid = IsValid();if(invalid)return invalid;
    int i;
    RC rc;

    RelAttr* selAttrs=new RelAttr[nSelAttrs];
    for(i=0;i<nSelAttrs;i++){
        selAttrs[i].relName=selAttrs_.relName;
        selAttrs[i].attrName=selAttrs_.attrName;
    }

    AggRelAttr* selAggAttrs=new AggRelAttr[nSelAttrs];
    for(i=0;i<nSelAttrs;i++){
        selAggAttrs[i].func=selAttrs_[i].func;
        selAggAttrs[i].relName=selAttrs_.relName;
        selAggAttrs[i].attrName=selAttrs_.attrName;
    }

    char** relations= new char*[nRelations];
    for(i=0;i<nRelations;i++)relations=strdup(relations_[i]);

    Condition* conditions_=new Condition[nConditions];
    for(i=0;i<nConditions;i++)conditions[i]=conditions_[i];

    for(i=0;i<nRelations;i++)
        if(rc=smm.SemCheck(relations[i]))return rc;

    sort(relations,relations+nRelations,strlt);

    ///这个adj find 是找到连续两个满足一定条件的元素 返回第一个的迭代器
    ///如果不传入第三个参数 默认找两个相同的
    char** dup=adjacent_find(relations,relations+nRelations,streq);

    ///有相同的关系
    if(dup!=relations+nRelations)return QL_DUPREL;

    bool SELECTSTAR=false;
    if(nSelAttrs==1&&strcmp(selAttrs[0].attrName,"*")==0){
        SELECTSTAR=true;
        nSelAttrs=0;
        for(int i=0;i<nRelations;i++){
            int ac;
            DataAttrInfo* aa;
            if(rc=smm.GetFromTable(relations[i],ac,aa))return rc;
            nSelAttrs+=ac;
            delete [] aa;
        }
        delete [] selAttrs;
        delete [] selAggAttrs;

        selAttrs= new RelAttr[nSelAttrs];
        selAggAttrs=new AggRelAttr[nSelAttrs];

        int j=0;
        for(int i=0;i<nRelations;i++){
            int ac;
            DataAttrInfo* aa;
            if(rc=smm.GetFromTable(relations[i],ac,aa))return rc;
            for(int k=0;k<ac;k++){
                selAttrs[j].attrName=strdup(aa[k].attrName);
                selAttrs[j].relName=relations[i];
                selAggAttrs[j].attrName=strdup(aa[k].attrName);
                selAggAttrs[j].relName=relations[i];
                selAttrs[j].func=NO_F;
                j++;
            }
            delete [] aa;
        }
    }

    if(order!=0){
        ///确保这个属性出现并只出现在其中一个关系仅1次
        if(rc=smm.FindRelForAttr(orderAttr,nRelations,relations))return rc;
        if(rc=smm.SemCheck(orderAttr))return rc;
    }

    if(group){
        if(rc=smm.FindRelForAttr(groupAttr,nRelations,relations))return rc;
        if(rc=smm.SemCheck(groupAttr))return rc;
    }else{
        ///如果没有group 就要确保都是NO_F
        for(i=0;i<nSelAttrs;i++)if(selAggAttrs[i].func!=NO_F)
            return SM_BADAGGFUN;
    }

    for(i=0;i<nSelAttrs;i++){
        if(strcmp(selAggAttrs[i].attrName,"*")==0&&
           selAggAttrs[i].func==COUNT_F){
            selAggAttrs[i].attrName=strdup(groupAttr.attrName);
            selAggAttrs[i].relName=strdup(groupAttr.relName);
            selAttrs[i].attrName=strdup(groupAttr.attrName);
            selAttrs[i].relName=strdup(groupAttr.relName);
        }
    }

    for(i=0;i<nSelAttrs;i++){
        ///对于不标明关系名的属性 检查是否唯一出现在一个关系中 无歧义
        if(selAttrs[i].relName==NULL){
            if(smm.FindRelForAttr(selAttrs[i],nRelations,relations))
                return rc;
        }else{
            selAttrs[i].relName=strdup(selAttrs[i].relName);
        }
        selAggAttrs[i].relName=strdup(selAttrs[i].relName);
        if(rc=smm.SemCheck(selAttrs[i]))return rc;
        if(rc=smm.SemCheck(selAggAttrs[i]))return rc;
    }



}

RC QL_Manager::Insert(const char* relName, int nValues, const Value values[]) {
    RC invalid=IsValid();
    if(invalid)return invalid;
    ///先检测一下存在这个relName吗
    RC rc=smm.SemCheck(relName);
    if(rc)return rc;
    int attrCount;
    DataAttrInfo* attr;
    rc=smm.GetFromTable(relName,attrCount,attr);

    ///比较一下属性个数对不对
    if(nValues!=attrCount) {
        delete [] attr;
        return QL_INVALIDSIZE;
    }

    ///看看类型对不对
    int size=0;
    for(int i=0; i<nValues; i++) {
        if(values[i].type!=attr[i].attrType) {
            delete [] attr;
            return QL_JOINKEYTYPEMISMATCH;
        }
        size+=attr[i].attrLength;
    }

    char* buf=new char[size];
    int offset=0;
    for(int i=0; i<nValues; i++) {
        memcpy(buf+offset,values[i].data,attr[i].attrLength);
        offset+=attr[i].attrLength;
    }

    ///这个函数就是将一条记录插进关系中
    rc=smm.LoadRecord(relName,size,buf);
    if(rc)return rc;
    Printer p(attr,attrCount);
    p.PrintHeader(cout);
    p.Print(cout,buf);
    p.PrintFooter(cout);

    delete [] attr;
    delete [] buf;

    return 0;
}

RC QL_Manager::Delete(const char* relName_, int nConditions, const Condition conditions_[]) {
    RC invalid=IsValid();
    if(invalid)return invalid;
    char relName[MAXNAME];
    strncpy(relName,relName_,MAXNAME);
    RC rc=smm.SemCheck(relName);
    if(rc)return rc;
    Condition* conditions=new Condition[nConditions];
    for(int i=0; i<nConditions; i++)conditions[i]=conditions_[i];
    for(int i=0; i<nConditions; i++) {
        if(conditions[i].lhsAttr.relName==NULL)
            conditions[i].lhsAttr.relName=relName;
        if(strcmp(conditions[i].lhsAttr.relName,relName!=0)) {
            ///delete语句只能出现一个关系
            delete [] conditions;
            return QL_BADATTR;
        }
        RC rc=smm.SemCheck(conditions[i].lhsAttr);
        if(rc!=0)return rc;
        if(conditions[i].bRhsIsAttr) {
            if(conditions[i].rhsAttr.relName==NULL)
                conditions[i].rhsAttr.relName=relName;
            if(strcmp(conditions[i].rhsAttr.relName,relName!=0)) {
                ///delete语句只能出现一个关系
                delete [] conditions;
                return QL_BADATTR;
            }
            RC rc=smm.SemCheck(conditions[i].rhsAttr);
            if(rc!=0)return rc;
        }
        rc=smm.SemCheck(conditions[i]);
        if(rc)return rc;
    }

    Iterator* it=GetLeafIterator(relName,nConditions,conditions);
    if(bQueryPlans) {
        cout<<"\n"<<it->Explain()<<endl;
    }

    Tuple t=it->GetTuple();
    if(rc=it->Open())return rc;

    Printer p(t);
    p.PrintHeader(cout);

    RM_FileHandle fh;
    if(rc=rmm.OpenFile(relName,fh))return rc;

    int attrCount=-1;
    DataAttrInfo* attributes;
    if(rc=smm.GetFromTable(relName,attrCount,attributes))return rc;
    IX_IndexHandle *indexes=new IX_IndexHandle[attrCount];
    for(int i=0; i<attrCount; i++) {
        if(attributes[i].indexNo!=-1)
            ixm.OpenIndex(relName,attributes[i].indexNo,indexes[i]);
    }

    ///至于如何找到符合condition的rid 应该就是这个迭代器里面的算法了
    while(1) {
        rc=it->GetNext(t);
        if(rc==it->Eof())break;
        if(rc)return rc;
        if(rc=fh.DeleteRec(t.GetRid()))return rc;
        for(int i=0; i<attrCount; i++) {
            if(attributes[i].indexNo!=-1) {
                void* pKey;
                t.Get(attributes[i].offset,pKey);
                indexes[i].DeleteEntry(pKey,t.GerRid());
            }
        }
        p.Print(cout,t);
    }

    p.PrintFooter(cout);

    for(int i=0; i<attrCount; i++) {
        if(attributes[i].indexNo!=-1) {
            RC rc=ixm.CloseIndex(indexes[i]);
            if(rc)return rc;
        }
    }
    delete [] indexes;
    delete [] attributes;
    if(rc=rmm.CloseFile(fh))return rc;

    delete [] conditions;
    if(rc=it->Close())return rc;
    return 0;
}



RC QL_Manager::Update(const char* relName_, const RelAttr& updAttr_, const int bIsValue,
                      const RelAttr& rhsRelAttr, const Value& rhsValue, int nConditions,
                      const Condition conditions_[]) {
    RC invalid=IsValid();
    if(invalid)return invalid;

    char relName[MAXNAME];
    strncpy(relName,relName_,MAXNAME);

    RC rc;
    if(rc=smm.SemCheck(relName))return rc;

    Condition* conditions=new Condition[nConditions];
    for(int i=0; i<nConditions; i++)conditions[i]=conditions_[i];

    RelAttr updAttr;
    updAttr.relName=relName;
    updAttr.attrName=updAttr_.attrName;

    if(smm.SemCheck(updAttr))return rc;

    ///估计是对这个要更新的属性构造一个条件 更新的等号的右值可以是一个元组的属性值也可以是用户输入的值
    Condition cond;
    cond.lhsAttr=updAttr;
    cond.bRhsIsAttr=!bIsValue;
    cond.rhsAttr.attrName=rhsRelAttr.attrName;
    cond.rhsAttr.relName=relName;
    cond.op=EQ_OP;
    cond.rhsValue.type=rhsValue.type;
    cond.rhsValue.data=rhsValue.data;

    ///右边是个已存在的属性
    //似乎也不需要左右是同名属性 只要属性的类型 长度 一致即可 但是似乎得是同一个关系里面的?
    if(bIsValue!=TRUE) {
        updAttr.attrName=rhsRelAttr.attrName;
        if(rc=smm.SemCheck(updAttr))return rc;
    }

    if(rc=smm.SemCheck(cond))return rc;

    for(int i=0; i<nConditions; i++) {
        if(conditions[i].lhsAttr.relName==NULL) {
            conditions[i].lhsAttr.relName=relName;
        }
        if(strcmp(conditions[i].lhsAttr.relName,relName)!=0) {
            delete [] conditions;
            return QL_BADATTR;
        }

        if(rc=smm.SemCheck(conditions[i].lhsAttr))return rc;

        if(conditions[i].bRhsIsAttr) {
            if(conditions[i].rhsAttr.relName==NULL) {
                conditions[i].rhsAttr.relName=relName;
            }
            if(strcmp(conditions[i].rhsAttr.relName,relName)!=0) {
                delete [] conditions;
                return QL_BADATTR;
            }

            if(rc=smm.SemCheck(conditions[i].rhsAttr))return rc;
        }
        if(rc=smm.SemCheck(conditions[i]))return rc;
    }

    Iterator* it;
    ///还没有想清楚为什么要先把要更新的属性废了 再get迭代器
    ///暂时想明白了 这里是一边 get  get到了就更新值以及索引 这样就会影响到还没有遍历完的索引
    ///不过这里的废了也就是 暂时标记为该属性无索引而已 然后get完迭代器就改回来
    if(smm.IsAttrIndexed(updAttr.relName,updAttr.attrName)) {
        if(rc=smm.DropIndexFromAttrCatAlone(updAttr.relName,updAttr.attrName))
            return rc;
        it=GetLeafIterator(relName,nConditions,conditions);
        if(rc=smm.ResetIndexFromAttrCatAlone(updAttr.relName,updAttr.attrName))return rc;
    } else it=GetLeafIterator(relName,nConditions,conditions);

    if(bQueryPlans)cout << "\n" << it->Explain() << "\n";

    ///it似乎是用来get某个属性的?
    Tuple t=it->GetTuple()；
            if(rc=it->Open())return rc;
    void* val=NULL;
    if(bIsValue)val=rhsValue.data;
    else t.Get(rhsRelAttr.attrName,val);

    Printer p(t);
    p.PrintHeader(cout);

    RM_FileHandle fh;
    if(rc=rmm.OpenFile(relName,fh))return rc;

    int attrCount=-1;
    int updAttrOffset=-1;
    DataAttrInfo* attributes;
    if(rc=smm.GetFromTable(relName,attrCount,attributes))return rc;
    IX_IndexHandle* indexes=new IX_IndexHandle[attrCount];
    ///把要更新的属性的索引打开 并找一下这个属性的offset
    for(int i=0; i<attrCount; i++) {

        if(attributes[i].indexNo!=-1&&
                strcmp(attributes[i].attrName,updAttr.attrName)==0
          ) {
            ixm.OpenIndex(relName,attributes[i].indexNo,indexes[i]);
        }
        if(strcmp(attributes[i].attrName,updAttr.attrName)==0)
            updAttrOffset=attributes[i].offset;
    }

    while(1) {
        rc=it->GetNext(t);
        if(rc==it->Eof())break;
        if(rc)return rc;
        RM_Record rec;
        for(int i=0; i<attrCount; i++) {
            if(attributes.indexNo!=-1&&
                    strcmp(attributes[i].attrName,updAttr.attrName)==0
              ) {
                void* pKey;
                t.Get(attributes[i].offset,pKey);
                ///更新索引应该都是先删后插 因为这个东西更新之后可能不在原位置了
                if(rc=indexes[i].DeleteEntry(pKey,t.GetRid()))return rc;
                if(indexes[i].InsertEntry(val,t.GetRid))return rc;
            }
        }
        ///还没有看清楚val是怎么get到的，val如果是常量可以看出过程 如果是某个元组的属性值，怎么拿到的?
        t.Set(updAttrOffset,val);
        char* newbuf;
        t.GetData(newbuf);
        rec.Set(newbuf,it->TupleLength(),t.GetRid());
        if(rc=fh.UpdateRec(rec))return rc;
        p.Print(cout,t);
    }

    p.PrintFooter(cout);
    if(rc=it->Close())return rc;
    for(int i=0; i<attrCount; i++) {
        if(attributes[i].indexNo!=-1&&strcmp(attributes[i].attrName, updAttr.attrName)==0) {
            if(rc=ixm.CloseIndex(indexes[i]))return rc;
        }
    }

    delete [] indexes;
    delete [] attributes;
    if(rc=rmm.CloseFile(fh))return rc;

    delete it;
    delete [] conditions;
    return 0;
}


///暂时有点看不懂
Iterator* QL_Manager::GetLeafIterator(const char* relName,
                                      int nConditions,
                                      const Condition conditions[],
                                      int nJoinConditions,
                                      const Condition jconditions[],
                                      int order,
                                      RelAttr* porderAttr) {
    RC invalid=IsValid();
    if(invalid)return NULL;
    if(relName==NULL)return NULL;
    int attrCount=-1;
    DataAttrInfo* attributes;
    RC rc;
    if(rc=smm.GetFromTable(relName,attrCount,attributes))return rc;
    int nIndexes=0;
    char* choseIndex=NULL;
    const Condition* chosenCond=NULL;
    Condition* filters=NULL;
    int nFilters=-1;
    Condition jBased=NULLCONDITION;

    ///估计这里只是找出与这一个关系relname相关联的用于join的条件
    map<string,const Condition*> jkeys;
    for(int j=0; j<nJoinConditions; j++) {
        if(strcmp(jconditions[j].lhsAttr.relName,relName)==0)
            jkeys[string(jconditions.[j].lhsAttr.attrName)]=&jconditions[j];
        if(jconditions[j].bRhsIsAttr&&
                strcmp(jconditions[j].rhsAttr.relName,relName)==0
          )
            jkeys[string(jconditions.[j].rhsAttr.attrName)]=&jconditions[j];
    }

    for(auto it=jkeys.begin(); it!=jkeys.end(); it++) {
        for(int i=0; i<attrCount; i++) {
            if(attributes[i].indexNo!=-1&&
                    strcmp(it->first.c_str(),attributes[i].attrName)==0) {
                nIndexes++;
                if(choseIndex==NULL||attributes[i].attrType==INT||
                        attributes[i].attrType==FLOAT
                  ) {
                    choseIndex=attributes[i].attrName;
                    jBased=*(it->second);
                    jBased.lhsAttr.attrName=choseIndex;
                    jBased.bRhsIsAttr=FALSE;
                    jBased.rhsValue.type=attributes[i].attrType;
                    jBased.rhsValue.data=NULL;
                    chosenCond=&jBased;
                }
            }
        }
    }

    if(chosenCond!=NULL) {
        nFilters=nConditions;
        filters=new Condition[nFilters];
        for(int j=0; j<nConditions; j++)if(choseCond!=&conditions[j])
                filters[j]=conditions[j];
    } else {
        map<string,const Condition*> keys;
        for(int j=0; j<nConditions; j++) {
            if(strcmp(conditions[j].lhsAttr.relName,relName)==0)
                keys[string(conditions[j].lhsAttr.attrName)]=&conditions[j];
            if(conditions[j].bRhsIsAttr&&strcmp(conditions[j].rhsAttr.relName,relName)==0)
                keys[string(conditions[j].rhsAttr.attrName)]=&conditions[j];
        }
        for(auto it=keys.begin(); it!=keys.end(); it++) {
            for(int i=0; i<attrCount; i++) {
                if(attributes[i].indexNo!=-1&&strcmp(it->first.c_str(),attributes[i].attrName)==0) {
                    nIndexes++;
                    if(choseIndex==NULL||attributes[i].attrType==INT||attributes[i].attrType==FLOAT) {
                        choseIndex=attributes[i].attrName;
                        chosenCond=it->second;
                    }
                }
            }
        }
        if(chosenCond==NULL){
            nFilters=nConditions;
            filters=new Condition[nFilters];
            for(int j=0;j<nConditions;j++){
                if(choseCond!=&conditions[j])
                    filters[j]=conditions[j];
            }
        }else {
            nFilters=nConditions-1;
            filters=new Condition[nFilters];
            for(int j=0,k=0;j<nConditions;j++)
                if(chosenCond!=&conditions[j])
                    filters[k]=conditions[j],k++;
        }
    }

    ///用文件扫描的
    if(chosenCond==NULL&&(nConditions==0||nIndexes==0)){
        Condition cond=NULLCONDITION;
        RC status=-1;
        Iterator* it=NULL;
        if(nConditions==0)it=new FileScan(smm,rmm,relName,status,cond);
        else it=it=new FileScan(smm,rmm,relName,status,cond,nConditions,conditions);
        if(status!=0){
            PrintErrorAll(status);
            return NULL;
        }
        delete [] filters;
        delete [] attributes;
        return it;
    }

    ///用索引扫描的
    RC status=-1;
    Iterator* it;
    bool desc=false;
    if(order!=0&&
       strcmp(porderAttr->relName,relName)==0&&
       strcmp(porderAttr->attrName,choseIndex)==0
       )
        desc=(order==-1?true:false);
    if(choseCond!=NULL){
        if((chosenCond->op==EQ_OP||
           chosenCond->op==GT_OP||
           chosenCond->op==GE_OP
           )&&
           order==0
            )
            desc=true;
        it=new IndexScan(smm,rmm,ixm,relName,choseIndex,status,*choseCond,nFilters,filters,desc);
    }else{///无条件的索引扫描
        it=new IndexScan(smm,rmm,ixm,relName,choseIndex,status,NULLCONDITION,nFilters,filters,desc);
    }

    if(status){
        PrintErrorAll(status);
        return NULL;
    }
    delete [] filters;
    delete [] attributes;
    return it;

}

RC QL_Manager::MakeRootIterator(Iterator*& newit, int nSelAttrs, const AggRelAttr selAttrs[], int nRelations, const char* const relations[], int order, RelAttr orderAttr, bool group, RelAttr groupAttr) {

}

RC QL_Manager::PrintIterator(Iterator* it) {

}

void QL_Manager::GetCondsForSingleRelation(int nConditions, Condition conditions[], char* relName, int& retCount, Condition*& retConds) {

}

void QL_Manager::GetCondsForTwoRelations(int nConditions, Condition conditions[], int nRelsSoFar, char* relations[], char* relName2, int& retCount, Condition*& retConds) {

}














