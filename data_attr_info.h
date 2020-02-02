#ifndef DATA_ATTR_INFO_H_INCLUDED
#define DATA_ATTR_INFO_H_INCLUDED


#include<iostream>
#include<cstring>
#include"mybase.h"
#include"catalog.h"


struct DataAttrInfo{
    DataAttrInfo(){
        memset(relName,0,,MAXNAME+1);
        memset(attrName,0,MAXNAME+1);
        offset=-1;
        func=NO_F;
    }
    DataAttrInfo(const AttrInfo& a){
        memset(attrName,0,MAXNAME+1);
        strcpy(attrName,a.attrName);
        attrType=a.attrType;
        attrLength=a.attrLength;
        memset(relName,0,MAXNAME+1);
        indexNo=-1;
        offset=-1;
        func=NO_F;
    }
    DataAttrInfo(const DataAttrInfo& d){
        strcpy(relName,d.relName);
        strcpy(attrName,d.attrName);
        offset=d.offset;
        attrType=d.attrType;
        attrLength=d.attrLength;
        indexNo=d.indexNo;
        func=d.func;
    }
    DataAttrInfo& operator=(const DataAttrInfo& d){
        if(this!=&d){
            strcpy(relName,d.relName);
            strcpy(attrName,d.attrName);
            offset=d.offset;
            attrType=d.attrType;
            attrLength=d.attrLength;
            indexNo=d.indexNo;
//            func=d.func;
        }
        retur *this;
    }

    static unsigned int size(){
        return 2*(MAXNAME+1)+2*sizeof(int)+sizeof(AttrType)+sizeof(AggFun);
    }
    static unsigned int members(){
        return 7;
    }

    int offset; ///属性的偏移量
    AttrType attrType; ///属性类型
    int attrLength;
    int indexNo; ///属性的索引号
    char relName[MAXNAME+1];///关系名
    char attrName[MAXNAME+1];///属性名
    AggFun func; ///用于group by 的类型
};









#endif // DATA_ATTR_INFO_H_INCLUDED
