#include<cstdio>
#include<iostream>
#include<fstream>
#include<sstream>
#include<set>
#include<string>
#include"mybase.h"
#include"sm.h"
#include"ix.h"
#include"rm.h"
#include"printer.h"
#include"catalog.h"

using namespace std;



SM_Manager::SM_Manager(IX_Manager& ixm_, RM_Manager& rmm_)
:rmm(rmm_),ixm(ixm_),bDBOpen(false)
{
    memset(cwd,0,1024);
}

SM_Manager::~SM_Manager() {

}

RC SM_Manager::OpenDb(const char* dbName) {
    if(dbName==NULL)return SM_BADOPEN;
    if(bDBOpen)return SM_DBOPEN;

    if(getcwd(cwd,1024)<0){
        cerr<<" getcwd error."<<endl;
        return SM_NOSUCHDB;
    }

    if(chdir(dbName)<0){
        cerr<<" chdir error to "<<dbName<<". Does the db exist ?\n";
        return SM_NOSUCHDB;
    }

    RC rc;
    if((rc=rmm.OpenFile("attrcat",attrfh))||
       (rc=rmm.OpenFile("relcat"),relfh)
       )return rc;
    bDBOpen=true;

    return 0;
}

RC SM_Manager::CloseDb() {
    if(!bDBOpen)return SM_NOTOPEN;
    RC rc;
    if((rc=rmm.CloseFile(attrfh))||
       (rc=rmm.CloseFile(relfh))
       ) return rc;
    if(chdir(cwd)<0){
        cerr<<" chdr error to "<<cwd<<".\n";
        return SM_NOSUCHDB;
    }
    bDBOpen=false;
    return 0;
}

RC SM_Manager::CreateTable(const char* relName, int attrCount, AttrInfo* attributes) {
    RC invalid=IsValid();if(invalid)return invalid;
    if(relName==NULL||attrCount<=0||attributes==NULL){
        return SM_BADTABLE;
    }
    if(strcmp(relName,"relcat")==0||strcmp(relName,"attrcat")==0)return SM_BADTABLE;
    RID rid;
    RC rc;
    set<string> uniq;
    DataAttrInfo* d=new DataAttrInfo[attrCount];
    int size=0;

    for(int i=0;i<attrCount;i++){
        d[i]=DataAttrInfo(attributes[i]);
        d[i].offset=size;
        size+=attributes[i].attrLength;
        strcpy(d[i].relName,relName);
        if(uniq.find(string(d[i].attrName))==uniq.end())uniq.insert(string(d[i].attrName));
        else return SM_BADATTR;

        if((rc=attrfh.InsertRec((char*)&d[i],rid))<0)return rc;
    }

    if(rc=rmm.CreateFile(relName,size))return rc;

    DataRelInfo rel;
    strcpy(rel.relName,relName);
    rel.attrCount=attrCount;
    rel.recordSize=size;
    rel.numPages=1;
    rel.numRecords=0;
    if((rc=relfh.InsertRec((char*)&rel,rid))<0)return rc;
    delete [] d;
    return 0;
}

///从关系表中找到名字为relname的第一个关系 返回datarelinfo 和 rid
RC SM_Manager::GetRelFromCat(const char* relName, DataRelInfo& rel, RID& rid) const{
    RC invalid=IsValid();if(invalid)return invalid;
    if(relName==NULL)return SM_BADTABLE;
    void* value=const_cast<char*>(relname);
    RM_FileScan rfs;
    RC rc=rfs.OpenScan(relfh,STRING,MAXNAME+1,offsetof(DataRelInfo,relName),EQ_OP,value,NO_HINT);
    if(rc)return rc;
    RM_Record rec;
    rc=rfs.GetNextRec(rec);
    if(rc==RM_EOF)return SM_NOSUCHTABLE;
    if(rc=rfs.CloseScan())return rc;
    DataRelInfo* prel;
    rec.GetData((char*&)prel);
    rel=*prel;
    rec.GetRid(rid);
    return 0;
}

RC SM_Manager::GetAttrFromCat(const char* relName,
                              const char* attrName,
                              DataAttrInfo& attr,
                              RID& rid) {
    RC invalid=IsValid();if(invalid)return invalid;
    if(relName==NUMM||attrName==NULL)return SM_BADTABLE;
    RC rc;
    RM_FileScan rfs;
    RC_Record rec;
    DataAttrInfo *data;
    if(rc=rfs.OpenScan(attrfh,STRING,MAXNAME+1,offsetof(DataAttrInfo,relName),EQ_OP,(void*)relName))
        return rc;
    bool attrFound=false;
    while(rc!=RM_EOF){
        rc=rfs.GetNextRec(rec);
        if(rc!=0&&rc!=RM_EOF)return rc;
        if(rc!=RM_EOF){
            rec.GetData((char*&)data);
            if(strcmp(data->attrName,attrName)==0){
                attrFound=true;
                break;
            }
        }
    }
    if(rc=rfs.CloseScan())return rc;
    if(!attrFound)return SM_BADATTR;
    attr=*data;
    attr.func=NO_F;
    rec.GetRid(rid);
    return 0;
}

RC SM_Manager::DropTable(const char* relName) {
    RC invalid=IsValid();if(invalid)return invalid;
    if(relName==NULL)return SM_BADTABLE;
    if(strcmp(relName,"relcat")==0||strcmp(relName,"attrcat")==0)return SM_BADTABLE;
    RM_FileScan rfs;
    RM_Record rec;
    DataRelInfo* data;
    RC rc;
    if(rc=rfs.OpenScan(relfh,STRING,MAXNAME+1,offsetof(DataRelInfo,relName),EQ_OP,(void*)relName))
        return rc;
    bool relFound=false;
    while(rc!=RM_EOF){
        rc=rfs.GetNextRec(rec);
        if(rc!=RM_EOF&&rc!=0)return rc;
        if(rc!=RM_EOF){
            rec.GetData((char*&)data);
            if(strcmp(data->relName,relName)==0){
                relFound=true;
                break;
            }
        }
    }
    if(rc=rfs.CloseScan())return rc;
    if(!relFound)return SM_NOSUCHTABLE;

    RID rid;
    rec.GetRid(rid);
    if(rc=rmm.DestroyFile(relName))return rc;
    if(rc=relfh.DeleteRec(rid))return rc;

    {
        RM_Record rec;
        DataAttrInfo* data;
        if(rc=rfs.OpenScan(attrfh,STRING,MAXNAME+1,offsetof(DataAttrInfo,relName),EQ_OP,(void*)relName))
            return rc;
        while(rc!=RM_EOF){
            rc=rfs.GetNextRec(rec);
            if(rc!=RM_EOF&&rc!=0)return rc;
            if(rc!=RM_EOF){
                rec.GetData((char*&)data);
                ///其实这里已经比较过relname是相等的了 不用再比一次 所以删去了relname的比较
                if(data->indexNo!=-1){
                    ///有索引
                    this->DropIndex(relName,data->attrName);
                }
                RID rid;
                rec.GetRid(rid);
                if(rc=attrfh.DeleteRec(rid))return rc;
            }
        }
        if(rc=rfs.CloseScan())return rc;
    }
    return 0;

}

RC SM_Manager::CreateIndex(const char* relName, const char* attrName) {
    RC invalid=IsValid();if(invalid)return invalid;
    if(relName==NULL||attrName==NULL)return SM_BADTABLE;
    DataAttrInfo attr;
    DataAttrInfo* data=&attr;

    RC rc;
    RID rid;
    rc=GetAttrFromCat(relName,attrName,attr,rid);
    if(rc)return rc;
    ///该属性已经有索引了
    if(data->indexNo!=-1)return SM_INDEXEXISTS;
    data->indexNo=data->offset;
    if(rc=ixm.CreateIndex(relName,data->indexNo,data->attrType,data->attrLength))
        return rc;
    RM_Record rec;
    rec.Set((char*)data,DataAttrInfo::size(),rid);
    if(rc=attrfh.UpdataRec(rec))return rc;
    IX_IndexHandle ixh;
    ///打开 关系relname的编号为indexno的索引
    if(rc=ixm.OpenIndex(relName,data->indexNo,ixh))return rc;


    RM_FileHandle rfh;
    ///这个rmfile就是元组存储的文件 rfh就是他的句柄
    if(rc=rmm.OpenFile(relName,rfh))return rc;
    RM_FileHandle* prfh=&rfh;

    int attrCount;
    DataAttrInfo* attributes;
    if(rc=GetFromTable(relName,attrCount,attributes))return rc;

    RM_FileScan rfs;
    if(rc=rfs.OpenScan(*prfh,data->attrType,data->attrLength,data->offset,NO_OP,NULL))return rc;
    ///把这个关系的这个属性全部插入索引
    while(rc!=RM_EOF){
        RM_Record rec;
        rc=rfs.GetNextRec(rec);
        if(rc!=0&&rc!=RM_EOF)return rc;
        if(rc!=RM_EOF){
            char* pData;
            rec.GetData((char*&)pData);
            ///insert这个函数必须传入属性的地址而不是整条 记录的地址
            ixh.InsertEntry(pData+data->offset,rid);
        }
    }
    if(rc=rfs.CloseScan())return rc;
    if(!rfh.IsValid()){
        if(rc=rmm.CloseFile(rfh))return rc;
    }
    if(rc=ixm.CloseIndex(ixh))return rc;

    ///所以这个attributes到底干了什么
    delete [] attributes;
    return 0;
}

RC SM_Manager::DropIndex(const char* relName, const char* attrName) {
    RC invalid=IsValid();if(invalid)return invalid;
    if(relName==NULL||attrName==NULL)return SM_BADTABLE;
    RM_FileScan rfs;
    RM_Record rec;
    DataAttrInfo* data;
    RC rc;
    if(rc=rfs.OpenScan(attrfh,STRING,MAXNAME+1,offsetof(DataAttrInfo,relName),EQ_OP,(void*)relName))
        return rc;
    bool attrFound=false;
    while(rc!=RM_EOF){
        rc=rfs.GetNextRec(rec);
        if(rc!=0&&rc!=RM_EOF)return rc;
        if(rc!=RM_EOF){
            rec.GetData((char*&)data);
            if(strcmp(data->attrName,attrName)==0){
                attrFound=true;
                data->indexNo=-1;
                break;
            }
        }
    }
    if(rc=rfs.CloseScan())return rc;
    if(!attrFound)return SM_BADATTR;

    ///去掉数据库relname的编号为data->offset的索引
    if(rc=ixm.DestroyIndex(relName,data->offset))return rc;

    RID rid;
    rec.GetRid(rid);
    rec.Set((char*)data,DataAttrInfo::size(),rid);
    ///写回attrcat
    if(rc=attrfh.UpdateRec(rec))return rec;
    return 0;

}


RC SM_Manager::DropIndexFromAttrCatAlone(const char* relName, const char* attrName) {
    RC invalid=IsValid();if(invalid)return invalid;
    cerr<<"attrName "<<attrName<<" early in DropIndexFromAttrCatAlone\n";
    if(relName==NULL||attrName==NULL)return SM_BADTABLE;
    RM_FileScan rfs;
    RM_Record rec;
    DataAttrInfo* data;
    RC rc;
    if(rc=rfs.OpenScan(attrfh,STRING,MAXNAME+1,offsetof(DataAttrInfo,relName),EQ_OP,(void*)relName))
        return rc;
    bool attrFound=false;
    while(rc!=RM_EOF){
        rc=rfs.GetNextRec(rec);
        if(rc!=0&&rc!=RM_EOF)return rc;
        if(rc!=RM_EOF){
            rec.GetData(data);
            if(strcmp(data->attrName,attrName)==0){
                attrFound=true;
                data->indexNo=-1;
                break;
            }
        }
    }
    if(rc=rfs.CloseScan())return rc;
    if(!attrFound)return SM_BADATTR;

    ///去掉数据库relname的编号为data->offset的索引
//    if(rc=ixm.DestroyIndex(relName,data->offset))return rc;

    RID rid;
    rec.GetRid(rid);
    rec.Set((char*)data,DataAttrInfo::size(),rid);
    ///写回attrcat
    if(rc=attrfh.UpdateRec(rec))return rec;
    return 0;

}


RC SM_Manager::ResetIndexFromAttrCatAlone(const char* relName, const char* attrName) {
    RC invalid=IsValid();if(invalid)return invalid;
    if(relName==NULL||attrName==NULL)return SM_BADTABLE;
    RM_FileScan rfs;
    RM_Record rec;
    DataAttrInfo* data;
    RC rc;
    if(rc=rfs.OpenScan(attrfh,STRING,MAXNAME+1,offsetof(DataAttrInfo,relName),EQ_OP,(void*)relName))
        return rc;
    bool attrFound=false;
    while(rc!=RM_EOF){
        rc=rfs.GetNextRec(rec);
        if(rc!=0&&rc!=RM_EOF)return rc;
        if(rc!=RM_EOF){
            rec.GetData((char*&)data);
            if(strcmp(data->attrName,attrName)==0){
                attrFound=true;
                data->indexNo=-1;
                break;
            }
        }
    }
    if(rc=rfs.CloseScan())return rc;
    if(!attrFound)return SM_BADATTR;

    ///去掉数据库relname的编号为data->offset的索引
//    if(rc=ixm.DestroyIndex(relName,data->offset))return rc;

    RID rid;
    rec.GetRid(rid);
    rec.Set((char*)data,DataAttrInfo::size(),rid);
    ///写回attrcat
    if(rc=attrfh.UpdateRec(rec))return rec;
    return 0;
}


///把一条记录插进relName这个表
RC SM_Manager::LoadRecord(const char* relName, int buflen, const char buf[]) {
    RC invalid=IsValid();if(invalid)return invalid;
    if(relName==NULL||buf==NULL)return SM_BADTABLE;
    RM_FileHandle rfh;
    RC rc;
    if(rc=rmm.OpenFile(relName,rfh))return rc;
    int attrCount=-1;
    DataAttrInfo *attributes;
    rc=GetFromTable(relName,attrCount,attributes);
    if(rc)return rc;
    IX_IndexHandle* indexes=new IX_IndexHandle[attrCount];

    int size=0;
    for(int i=0;i<attrCount;i++){
        size+=attributes[i].attrLength;
        if(attributes[i].indexNo!=-1){
            ixm.OpenIndex(relName,attributes[i].indexNo,indexes[i]);
        }
    }

    if(size!=buflen)return SM_BADTABLE;

    ///首先插进relName这个表中 然后更新这个表的所有索引
    {
        RID rid;
        if((rc=rfh.InsertRec(buf,rid))<0)
            return rc;
        for(int i=0;i<attrCount;i++){
            if(attributes[i].indexNo!=-1){
                char* ptr=const_cast<char*>(buf+attributes[i].offset);
                if(rc=indexes[i].InsertEntry(ptr,rid));
            }
        }
    }

    DataRelInfo r;
    RID rid;
    if(rc=GetRelFromCat(relName,r,rid))return rc;

    r.numRecords+=1;
    r.numPages=rfh.GetNumPages();
    RM_Record rec;
    rec.Set((char*)&r,DataRelInfo::size(),rid);
    if(rc=relfh.UpdataRec(rec))return rc;
    if(rc=rmm.CloseFile(rfh))return rc;

    for(int i=0;i<attrCount;i++){
        if(attributes[i].indexNo!=-1){
            if(rc=ixm.CloseIndex(indexes[i]))return rc;
        }
    }

    delete [] attributes;
    delete [] indexes;
    return 0;
}

RC SM_Manager::Load(const char* relName, const char* fileName) {



}


bool SM_Manager::IsAttrIndexed(const char* relname, const char* attrName) {

}



RC SM_Manager::FindRelForAttr(RelAttr& ra, int nRelations, const char* const possibleRelations[]) {

}

RC SM_Manager::SemCheck(const Condition& cond) {

}

RC SM_Manager::SemCheck(const AggRelAttr& ra) {

}

RC SM_Manager::SemCheck(const RelAttr& ra) {

}

RC SM_Manager::SemCheck(const char* relName) {

}

RC SM_Manager::GetNumRecords(const char* relName) {

}

RC SM_Manager::GetNumPages(const char* relName) {

}





RC SM_Manager::GetFromTable(const char* relName, int& attrCount, DataAttrInfo*& attributes) {

}

RC SM_Manager::IsValid() {

}

RC SM_Manager::Get(const string& paramName, string& value) {

}

RC SM_Manager::Set(const char* paramName, const char* value) {

}

RC SM_Manager::Print(const char* relName) {

}

RC SM_Manager::Help(const char* relName) {

}

RC SM_Manager::Help() {

}
























