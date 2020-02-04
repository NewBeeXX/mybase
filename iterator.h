#ifndef ITERATOR_H_INCLUDED
#define ITERATOR_H_INCLUDED


#include"mybase.h"
#include"data_attr_info.h"
#include"rm_rid.h"
#include<sstream>
#include"predicate.h"

using namespace std;

class DataAttrInfo;

///元组
class Tuple{
public:
    Tuple(int ct,int length_):count(ct),length(length_),rid(-1,-1){
        data=new char[length];
    }
    Tuple(const Tuple& rhs):count(rhs.count),length(rhs.length),rid(-1,-1){
        data=new char[length];
        memcpy(data,rhs.data,length);
        SetAttr(rhs.GetAttributes());
    }
    Tuple& operator=(const Tuple& rhs){
        if(this!=&rhs){
            memcpy(data,rhs.data,length);
            SetAttr(rhs.GetAttributes());
        }
        return *this;
    }
    ~Tuple(){delete [] data;}
    int GetLength()const{return length;}
    int GetAttrCount()const{return count;}
    DataAttrInfo* GetAttributes()const{return attrs;}
    void Set(const char* buf){
        memcpy(data,buf,length);
    }
    void GetData(const char*& buf)const{buf=data;}
    void GetData(char*& buf)const{buf=data;}
    void SetAttr(DataAttrInfo* pa){attrs=pa;}
    void Get(const char* attrName,void*& p)const{
        for(int i=0;i<count;i++){
            if(strcmp(attrs[i].attrName,attrName)==0){
                p=data+attrs[i].offset;
                return ;
            }
        }
    }
    void Get(int attrOffset,void*& p)const{
        p=data+attrOffset;
    }
    void Set(int attrOffset,void* p){
        int attrLength=0;
        for(int i=0;i<count;i++){
            if(attrs[i].offset==attrOffset){
                attrLength=attrs[i].attrLength;
            }
        }
        memcpy(data+attrOffset,p,attrLength);
    }
    void Get(const char* attrName,int& intAttr)const{
        for(int i=0;i<count;i++){
            if(strcmp(attrs[i].attrName,attrName)==0){
                intAttr=*(int*)(data+attrs[i].offset);
                return ;
            }
        }
    }
    void Get(const char* attrName,float& floatAttr)const{
        for(int i=0;i<count;i++){
            if(strcmp(attrs[i].attrName,attrName)==0){
                floatAttr=*(float*)(data+attrs[i].offset);
                return ;
            }
        }
    }
    void Get(const char* attrName,char strAttr[])const{
        for(int i=0;i<count;i++){
            if(strcmp(attrs[i].attrName,attrName)==0){
                strncpy(strAttr,(char*)(data+attrs[i].offset),attrs[i].attrLength);
                return ;
            }
        }
    }
    RID GetRid()const{return rid;}
    void SetRid(RID r){rid=r;}

private:
    char* data;
    DataAttrInfo* attrs;
    int count;
    int length;
    RID rid;
};





class Iterator{
public:
    Iterator():bIterOpen(false),indent(""),bSorted(false),desc(false){}
    virtual ~Iterator(){}
    virtual RC Open()=0;
    virtual RC GetNext(Tuple& t)=0;
    virtual RC Close()=0;

    virtual RC Eof()const=0;
    virtual DataAttrInfo* GetAttr()const{return attrs;}
    virtual int GetAttrCount()const{return attrCount;}
    virtual Tuple GetTuple()const{
        Tuple t(GetAttrCount(),TupleLength());
        t.SetAttr(this->GetAttr());
        return t;
    }
    virtual int TupleLength()const{
        int l=0;
        DataAttrInfo* a=GetAttr();
        for(int i=0;i<GetAttrCount();i++)l+=a[i].attrLength;
        return l;
    }
    virtual string Explain()=0;
    virtual void SetIndent(const string& indent_){indent=indent_;}
    virtual bool IsSorted()const{return bSorted;}
    virtual bool IsDesc()const{return desc;}
    virtual string GetSortRel()const{return sortRel;}
    virtual string GetSortAttr()const{return sortAttr;}
protected:
    bool bIterOpen;
    DataAttrInfo* attrs;
    int attrCount;
    stringstream explain;
    string indent;
    bool bSorted;
    bool desc;
    string sortRel;
    string sortAttr;
};

///这是用来传进sort的函数?
class TupleCmp{
public:
    TupleCmp(AttrType sortKeyType,
             int sortKeyLength,
             int sortKeyOffset,
             CompOp c
             ):c(c),p(sortKeyType,sortKeyLength,sortKeyOffset,c,NULL,NO_HINT),
             sortKeyOffset(sortKeyOffset){}
    TupleCmp():c(EQ_OP),p(INT,4,0,c,NULL,NO_HINT),sortKeyOffset(0){}

    bool operator()(const Tuple& lhs,const Tuple& rhs)const{
        void* b=NULL;
        rhs.Get(sortKeyOffset,b);
        const char* abuf;
        lhs.GetData(abuf);
        return p.eval(abuf,(char*)b,c);
    }
private:
    CompOp c;
    Predicate p;
    int sortKeyOffset;
};
















#endif // ITERATOR_H_INCLUDED
