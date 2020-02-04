#ifndef AGG_H_INCLUDED
#define AGG_H_INCLUDED

///聚集函数 输入一个集合 输出一个值 如minmax avg sum
///aggregate aggregation

#include"mybase.h"
#include"iterator.h"
#include"sm.h"
#include"ql_error.h"
#include"predicate.h"
#include<list>

using namespace std;



class Agg:public Iterator{
public:
    Agg(Iterator* lhsIt,
        RelAttr groupAttr,
        int nSelAttrs,
        const AggRelAttr selAttrs[],
        RC& status
        ):lhsIt(lhsIt){

        if(lhsIt==NULL||
           nSelAttrs<=0||
           groupAttr.attrName==NULL||
           !lhsIt->IsSorted()||
           lhsIt->GetSortRel()!=string(groupAttr.relName)||
           lhsIt->GetSortAttr()!=string(groupAttr.attrName)
           ){
                status=SM_NOSUCHTABLE;
                return ;
           }
        attrCount=nSelAttrs;
        attrs=new DataAttrInfo[attrCount];
        lattrs=new DataAttrInfo[attrCount];
        aggs=new AggFun[attrCount]
        attrCmps=new TupleCmp[attrCount];

        DataAttrInfo* itattrs=lhsIt->GetAttrs();

        int offsetsofar=0;
        for(int j=0;j<attrCount;j++){
            for(int i=0;i<lhsIt->GetAttrCount();i++){
                if(strcmp(selAttrs[j].relName,itattrs[i].relName)==0&&
                   strcmp(selAttrs[j].attrName,itattrs[i].attrName==0)
                   ){
                    lattrs[j]=itattrs[i];
                    attrs[j]=itattrs[i];
                    attrs[j].func=selAttrs[j].func;
                    if(attrs[j].func==COUNT_F)attrs[j].attrType=INT;
                    attrs[j].offset=offsetsofar;
                    offsetsofat+=itattrs[i].attrLength;
                    attrCmps[j]=TupleCmp(attrs[j].attrType,attrs[j].attrLength,attrs[j].offset,GT_OP);
                    break;

                }

            }
            aggs[j]=selAttrs[j].func;
        }

        DataAttrInfo gattr;
        for(int i=0;i<lhsIt->GetAttrCount();i++){
            if(strcmp(groupAttr.relName,itattrs[i].relName)==0&&
               strcmp(groupAttr.attrName,itattrs[i].attrName)==0
               ){
                gattr=itattrs[i];
                break;
               }
        }
        cmp=TupleCmp(gattr.attrType, gattr.attrLength, gattr.offset, EQ_OP);
        if(lhsIt->IsSorted()){
            bSorted=true;
            desc=lhsIt->IsDesc();
            sortRel=lhsIt->GetSortRel();
            sortAttr=lhsIt->GetSortAttr();
        }
        it=set.begin();
        explain<<"Agg\n";
        explain<<"   AggKey="<<groupAttr<<endl;
        for(int i=0;i<attrCount;i++){
            explain<<"   AggFun["<<i<<"]"<<aggs[i]<<endl;
        }
        status=0;
    }
    string Explain(){
        stringstream s;
        s<<indent<<explain.str();
        lhsIt->SetIndent(indent+"-----");
        s<<lhsIt->Explain();
        return s.str();
    }
    virtual ~Agg(){
        delete lhsIt;
        delete [] attrs;
        delete [] lattrs;
        delete [] aggs;
        delete [] attrCmps;
    }

    virtual RC Open(){
        if(bIterOpen)return RM_HANDLEOPEN;
        RC rc=lhsIt->Open();
        if(rc)return rc;
        Tuple prev=lhsIt->GetTuple();
        bool firstTime=true;
        Tuple lt=lhsIt->GetTuple();
        Tuple t=this->GetTuple();
        char* buf;
        t.GetData(buf);
        char* lbuf;
        lt.GetData(lbuf);
        while(lhsIt->GetNext(lt)!=lhsIt->Eof()){
            if(firstTime||cmp(prev,lt)){

                for(int i=0;i<attrCount;i++){
                    if(aggs[i]==NO_F){
                        memcpy(buf+attrs[i].offset,lbuf+lattrs[i].offset,attrs[i].attrLength);
                    }else if(aggs[i]==MAX_F){
                        if(attrCmps[i](prev,t)){
                            memcpy(buf+attrs[i].offset,lbuf+lattrs[i].offset,attrs[i].attrLength);
                        }
                    }else if(aggs[i]==MIN_F){
                        if(attrCmps[i](t,prev)){
                            memcpy(buf+attrs[i].offset,lbuf+lattrs[i].offset,attrs[i].attrLength);
                        }
                    }else if(aggs[i]==COUNT_F){
                        int count=*((int*)buf+attrs[i].offset);
                        if(firstTime){
                            count=0;
                        }
                        ++count;
                        memcpy(buf+attrs[i].offset,&count,attrs[i].attrLength);
                    }
                }

                firstTime=false;
            }else{
                set.push_back(t);
                for(int i=0;i<attrCount;i++){
                    if(aggs[i]==NO_F){
                        memcpy(buf+attrs[i].offset,lbuf+lattrs[i].offset,attrs[i].attrLength);
                    }else if(aggs[i]==MAX_F){
                        if(attrCmps[i](prev,t)){
                            memcpy(buf+attrs[i].offset,lbuf+lattrs[i].offset,attrs[i].attrLength);
                        }
                    }else if(aggs[i]==MIN_F){
                        if(attrCmps[i](t,prev)){
                            memcpy(buf+attrs[i].offset,lbuf+lattrs[i].offset,attrs[i].attrLength);
                        }
                    }else if(aggs[i]==COUNT_F){
                        int count=*((int*)buf+attrs[i].offset);
                        count=0;
                        ++count;
                    }
                }
            }
            prev=lt;
        }


        if(!firstTime){
            ///0条记录
            set.push_back(t);
        }
        it=set.begin();
        bIterOpen=true;
        return 0;
    }
    virtual RC GetNext(Tuple& t){
        if(!bIterOpen)return RM_FNOTOPEN;
        if(it==set.end())return Eof();
        t=*it;
        it++;
        return 0;
    }
    virtual RC Close(){
        if(!bIterOpen)return RM_FNOTOPEN;
        RC rc=lhsIt->Close();
        if(rc)return rc;
        set.clear();
        it=set.begin();
        bIterOpen=false;
        return 0;
    }
    virtual RC Eof()const{return QL_EOF;}

private:
    TupleCmp cmp;
    TupleCmp* attrCmps;
    Iterator* lhsIt;
    DataAttrInfo* lattrs;
    AggFun* aggs;
    list<Tuple> set;
    list<Tuple>::const_iterator it;
};
















#endif // AGG_H_INCLUDED
