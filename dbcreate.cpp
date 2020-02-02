#include<iostream>
#include<cstdio>
#include<string>
#include<sstream>
#include<unistd.h>
#include"rm.h"
#include"sm.h"
#include"mybase.h"
#include"catalog.h"


using namespace std;

///static 函数只能在本文件中调用
static void PrintErrorExit(RC rc){
    PrintErrorAll(rc);
    exit(rc);
}

int main(int argc,char* argv[]){
    RC rc;

    ///必须以 该程序名 <dbname> 的方式调用
    if(argc!=2){
        cerr<<"Usage: "<<argv[0]<<" <dbname> \n";
        exit(1);
    }

    string dbname(argv[1]);
    stringstream command;
    command<<"mkdir "<<dbname;
    ///构造 mkdir dbname 的命令
    rc=system(command.str().c_str());
    if(rc!=0){
        cerr<<argv[0]<<" mkdir error for"<<dbname<<"\n";
        exit(rc);
    }
    ///相当于 cd
    if(chdir(dbname.c_str())<0){
        cerr << argv[0] << " chdir error to " << dbname << "\n";
        exit(1);
    }

    ///创建系统目录
    PF_Manager pfm;
    RM_Manager rmm(pfm);
    RM_FileHandle relfh,attrfh;

        ///datarel就是一个关系 这里返回的就是一个元组的字节数s
    if((rc=rmm.CreateFile("relcat",DataRelInfo::size()))||
       (rc=rmm.OpenFile("relcat",relfh))
       )PrintErrorExit(rc);
    if((rc=rmm.CreateFile("attrcat",DataAttrInfo::size()))||
       (rc=rmm.OpenFile("attrcat",attrfh))
       )PrintErrorExit(rc);

    DataRelInfo relcat_rel;
    strcpy(relcat_rel.relName,"relcat");
    relcat_rel.attrCount=DataRelInfo::members();
    relcat_rel.recordSize=DataRelInfo::size();
    relcat_rel.numPages=1;
    relcat_rel.numRecords=2; ///relcat&attrcat

    DataRelInfo attrcat_rel;
    strcpy(attrcat_rel.relName,"attrcat");
    attrcat_rel.attrCount=DataAttrInfo::members();
    attrcat_rel.recordSize=DataAttrInfo::size();
    attrcat_rel.numPages=1;
    attrcat_rel.numRecords=DataAttrInfo::members()+DataRelInfo::members(); ///relcat&attrcat


    ///下面把每个DataRelInfo当做一条记录 插进relfh
    RID rid;
    if((rc=relfh.InsertRec((char*)&relcat_rel,rid))<0||
       (rc=relfn.InsertRec((char*)&attrcat_rel,rid))<0
       ) PrintErrorExit(rc);

    ///上面的把两个关系（一个关于关系的关系 一个关于属性的关系） 插进 relfh


    ///下面把DataRelInfo的各种属性当做一条记录 插进attrfh
    DataAttrInfo a;
    strcpy(a.relName,"relcat");
    strcpy(a.attrName,"relName");
    a.offset=offsetof(DataRelInfo,relName);
    a.attrType=STRING;
    a.attrLength=MAXNAME+1;
    a.indexNo=-1;
    if((rc=attrfh.InsertRec((char*)&a,rid))<0)PrintErrorExit(rc);

    strcpy(a.relName,"relcat");
    strcpy(a.attrName,"recordSize");
    a.offset=offsetof(DataRelInfo,recordSize);
    a.attrType=INT;
    a.attrLength=sizeof(int);
    a.indexNo=-1;
    if((rc=attrfh.InsertRec((char*)&a,rid))<0)PrintErrorExit(rc);

    strcpy(a.relName,"relcat");
    strcpy(a.attrName,"attrCount");
    a.offset=offsetof(DataRelInfo,attrCount);
    a.attrType=INT;
    a.attrLength=sizeof(int);
    a.indexNo=-1;
    if((rc=attrfh.InsertRec((char*)&a,rid))<0)PrintErrorExit(rc);

    strcpy(a.relName,"relcat");
    strcpy(a.attrName,"numPages");
    a.offset=offsetof(DataRelInfo,numPages);
    a.attrType=INT;
    a.attrLength=sizeof(int);
    a.indexNo=-1;
    if((rc=attrfh.InsertRec((char*)&a,rid))<0)PrintErrorExit(rc);

    strcpy(a.relName,"relcat");
    strcpy(a.attrName,"numRecods");
    a.offset=offsetof(DataRelInfo,numRecods);
    a.attrType=INT;
    a.attrLength=sizeof(int);
    a.indexNo=-1;
    if((rc=attrfh.InsertRec((char*)&a,rid))<0)PrintErrorExit(rc);


    ///下面把DataAttrInfo的各个属性当做一条记录插入attrfh
    strcpy(a.relName,"attrcat");
    strcpy(a.attrName,"relName");
    a.offset=offsetof(DataAttrInfo,relName);
    a.attrType=STRING;
    a.attrLength=MAXNAME+1;
    a.indexNo=-1;
    if((rc=attrfh.InsertRec((char*)&a,rid))<0)PrintErrorExit(rc);


    strcpy(a.relName,"attrcat");
    strcpy(a.attrName,"attrName");
    a.offset=offsetof(DataAttrInfo,attrName);
    a.attrType=STRING;
    a.attrLength=MAXNAME+1;
    a.indexNo=-1;
    if((rc=attrfh.InsertRec((char*)&a,rid))<0)PrintErrorExit(rc);

    strcpy(a.relName,"attrcat");
    strcpy(a.attrName,"offset");
    a.offset=offsetof(DataAttrInfo,offset);
    a.attrType=INT;
    a.attrLength=sizeof(int);
    a.indexNo=-1;
    if((rc=attrfh.InsertRec((char*)&a,rid))<0)PrintErrorExit(rc);

    strcpy(a.relName,"attrcat");
    strcpy(a.attrName,"attrType");
    a.offset=offsetof(DataAttrInfo,attrType);
    a.attrType=INT;
    a.attrLength=sizeof(AttrType);
    a.indexNo=-1;
    if((rc=attrfh.InsertRec((char*)&a,rid))<0)PrintErrorExit(rc);

    strcpy(a.relName,"attrcat");
    strcpy(a.attrName,"attrLength");
    a.offset=offsetof(DataAttrInfo,attrLength);
    a.attrType=INT;
    a.attrLength=sizeof(int);
    a.indexNo=-1;
    if((rc=attrfh.InsertRec((char*)&a,rid))<0)PrintErrorExit(rc);


    strcpy(a.relName,"attrcat");
    strcpy(a.attrName,"indexNo");
    a.offset=offsetof(DataAttrInfo,indexNo);
    a.attrType=INT;
    a.attrLength=sizeof(int);
    a.indexNo=-1;
    if((rc=attrfh.InsertRec((char*)&a,rid))<0)PrintErrorExit(rc);

    strcpy(a.relName,"attrcat");
    strcpy(a.attrName,"func");
    a.offset=offsetof(DataAttrInfo,func);
    a.attrType=INT;
    a.attrLength=sizeof(AggFun);
    a.indexNo=-1;
    if((rc=attrfh.InsertRec((char*)&a,rid))<0)PrintErrorExit(rc);

    if((rc=rmm.CloseFile(attrfh))<0||
       (rc=rmm.CloseFile(relfh))<0
       ) PrintErrorExit(rc);

    return 0;
}







