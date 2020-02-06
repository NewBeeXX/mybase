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
#include"unistd.h"

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
       (rc=rmm.OpenFile("relcat",relfh))
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
    void* value=const_cast<char*>(relName);
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
                              RID& rid) const{
    RC invalid=IsValid();if(invalid)return invalid;
    if(relName==NULL||attrName==NULL)return SM_BADTABLE;
    RC rc;
    RM_FileScan rfs;
    RM_Record rec;
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
    if(rc=attrfh.UpdateRec(rec))return rc;
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
    if(rc=attrfh.UpdateRec(rec))return rc;

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
    if(rc=attrfh.UpdateRec(rec))return rc;
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
                data->indexNo=data->offset;
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
    if(rc=attrfh.UpdateRec(rec))return rc;
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
    if(rc=relfh.UpdateRec(rec))return rc;
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
    RC invalid=IsValid();if(invalid)return invalid;
    if(relName==NULL||fileName==NULL)return SM_BADTABLE;
    ifstream ifs(fileName);
    if(ifs.fail())return SM_BADTABLE;
    RM_FileHandle rfh;
    RC rc;
    if(rc=rmm.OpenFile(relName,rfh))return rc;
    int attrCount=-1;
    DataAttrInfo* attributes;
    if(rc=GetFromTable(relName,attrCount,attributes))return rc;
    IX_IndexHandle *indexes=new IX_IndexHandle[attrCount];
    int size=0;
    for(int i=0;i<attrCount;i++){
        size+=attributes[i].attrLength;
        if(attributes[i].indexNo!=-1){
            ixm.OpenIndex(relName,attributes[i].indexNo,indexes[i]);
        }
    }



    char* buf=new char[size];
    int numLines=0;
    ///下面这个循环 从filename这个文件中读取每一行 把每一行看成一个要插入的元组
    ///先构造出一块内存 这块内存保存的就是这个元组的数据 然后插入rm
    while(!ifs.eof()){
        memset(buf,0,size);
        string line;
        getline(ifs,line);
        if(line.length()==0)continue;
        numLines++;
        string token;
        istringstream iss(line);
        int i=0;
        ///getline函数默认分割符为'\n' 所以你可以get到一行 这里改成用,分割
        while(getline(iss,token,',')){
            istringstream ss(token);
            if(attributes[i].attrType==INT){
                int val;
                ss>>val;
                memcpy(buf+attributes[i].offset,&val,attributes[i].attrLength);
            }
            if(attributes[i].attrType==FLOAT){
                float val;
                ss>>val;
                memcpy(buf+attributes[i].offset,&val,attributes[i].attrLength);
            }
            if(attributes[i].attrType==STRING){
                string& val=token;
                if(val.length()>attributes[i].attrLength){
                    cerr << "SM_Manager::Load truncating to "
                        << attributes[i].attrLength << " - " << val << endl;
                    ///截断了
                    memcpy(buf+attributes[i].offset,val.c_str(),attributes[i].attrLength);
                }else{
                    memcpy(buf+attributes[i].offset,val.c_str(),val.length());
                }
            }

            ++i;
        }
        RID rid;
        if((rc=rfh.InsertRec(buf,rid))<0)return rc;
        for(int i=0;i<attrCount;i++){
            if(attributes[i].indexNo!=-1){
                rc=indexes[i].InsertEntry(buf+attributes[i].offset,rid);
                if(rc!=0)return rc;
            }
        }
    }


    DataRelInfo r;
    RID rid;
    if(rc=GetRelFromCat(relName,r,rid))return rc;

    r.numRecords+=numLines;
    r.numPages=rfh.GetNumPages();
    RM_Record rec;
    rec.Set((char*)&r,DataRelInfo::size(),rid);
    if(rc=relfh.UpdateRec(rec))return rc;
    if(rc=rmm.CloseFile(rfh))return rc;
    for(int i=0;i<attrCount;i++)if(attributes[i].indexNo!=-1){
        if(rc=ixm.CloseIndex(indexes[i]))return rc;
    }
    delete [] buf;
    delete [] attributes;
    delete [] indexes;
    ifs.close();
    return 0;
}


RC SM_Manager::Print(const char* relName) {
    RC invalid=IsValid();if(invalid)return invalid;
    DataAttrInfo* attributes;
    RM_FileHandle rfh;
    RM_FileHandle *prfh;
    int attrCount;
    RM_Record rec;
    char* data;
    RC rc;
    if(strcmp(relName,"relcat")==0)prfh=&relfh;
    else if(strcmp(relName,"attrcat")==0)prfh=&attrfh;
    else{
        if(rc=rmm.OpenFile(relName,rfh))return rc;
        prfh=&rfh;
    }
    if(rc=GetFromTable(relName,attrCount,attributes))return rc;

    Printer p(attributes,attrCount);
    p.PrintHeader(cout);

    RM_FileScan rfs;
    ///进行一个全表扫描 不用条件
    if(rc=rfs.OpenScan(*prfh,INT,sizeof(int),0,NO_OP,NULL))return rc;

    while(rc!=RM_EOF){
        rc=rfs.GetNextRec(rec);
        if(rc!=RM_EOF&&rc!=0)return rc;
        if(rc!=RM_EOF){
            rec.GetData(data);
            p.Print(cout,data);
        }
    }

    p.PrintFooter(cout);
    if(rc=rfs.CloseScan())return rc;
    if(!rfh.IsValid()){
        if(rc=rmm.CloseFile(rfh))return rc;
    }
    delete [] attributes;
    return 0;
}

RC SM_Manager::Set(const char* paramName, const char* value) {
    RC invalid=IsValid();if(invalid)return invalid;
    if(paramName==NULL||value==NULL)return SM_BADPARAM;
    params[paramName]=string(value);
    cout<<"Set\n"
        <<"   paramName=" << paramName << "\n"
        <<"   value    =" << value << "\n";
    return 0;
}


RC SM_Manager::Get(const string& paramName, string& value) const{
    RC invalid=IsValid();if(invalid)return invalid;
    auto it=params.find(paramName);
    if(it==params.end())return SM_BADPARAM;
    value=it->second;
    return 0;
}

///把数据库中已有的表信息(不是表的所有元组)打出来
RC SM_Manager::Help() {
    RC invalid=IsValid();if(invalid)return invalid;
    DataAttrInfo * attributes;
    DataAttrInfo nameattr[1];
    int attrCount;
    RM_Record rec;
    char* data;
    RC rc;

    if(rc=GetFromTable("relcat",attrCount,attributes))return rc;
    for(int i=0;i<attrCount;i++){
        if(strcmp(attributes[i].attrName,"relName")==0)nameattr[0]=attributes[i];
    }

    Printer p(nameattr,1);
    p.PrintHeader(cout);
    RM_FileScan rfs;
    if(rc=rfs.OpenScan(relfh,INT,sizeof(int),0,NO_OP,NULL))return rc;
    while(rc!=RM_EOF){
        rc=rfs.GetNextRec(rec);
        if(rc!=0&&rc!=RM_EOF)return rc;
        if(rc!=RM_EOF){
            rec.GetData(data);
            p.Print(cout,data);
        }
    }
    p.PrintFooter(cout);
    if(rc=rfs.CloseScan())return rc;
    delete [] attributes;

    return 0;
}

///暂时看不太明白这个有参数的help
RC SM_Manager::Help(const char* relName) {
    RC invalid=IsValid();if(invalid)return invalid;
    DataAttrInfo * attributes;
    DataAttrInfo nameattr[DataAttrInfo::members()-1];
    DataAttrInfo relinfo;
    int attrCount;
    RM_Record rec;
    char* data;
    RC rc;

    if(rc=GetFromTable("attrcat",attrCount,attributes))return rc;
    int j=0;
    for(int i=0;i<attrCount;i++){
        if(strcmp(attributes[i].attrName,"relName")!=0){
            nameattr[j]=attributes[i];
            j++;
        }else relinfo=attributes[i];
    }

    Printer p(nameattr,attrCount-1);
    p.PrintHeader(cout);
    RM_FileScan rfs;
    if(rc=rfs.OpenScan(attrfh,STRING,relinfo.attrLength,relinfo.offset,EQ_OP,(void*)relName))return rc;
    while(rc!=RM_EOF){
        rc=rfs.GetNextRec(rec);
        if(rc!=0&&rc!=RM_EOF)return rc;
        if(rc!=RM_EOF){
            rec.GetData(data);
            p.Print(cout,data);
        }
    }
    p.PrintFooter(cout);
    if(rc=rfs.CloseScan())return rc;
    delete [] attributes;

    return 0;
}

RC SM_Manager::IsValid() const{
    return bDBOpen?0:SM_BADOPEN;
}

RC SM_Manager::GetFromTable(const char* relName, int& attrCount, DataAttrInfo*& attributes) {
    RC invalid=IsValid();if(invalid)return invalid;
    if(relName==NULL)return SM_NOSUCHTABLE;
    void* value=const_cast<char*>(relName);
    RM_FileScan rfs;
    RC rc;
    ///先找一下这个关系是否存在
    if(rc=rfs.OpenScan(relfh,STRING,MAXNAME+1,offsetof(DataRelInfo,relName),EQ_OP,value,NO_HINT))
        return rc;
    RM_Record rec;
    rc=rfs.GetNextRec(rec);
    if(rc==RM_EOF)return SM_NOSUCHTABLE;
    DataRelInfo* prel;
    rec.GetData((char*&)prel);
    if(rc=rfs.CloseScan())return rc;
    attrCount=prel->attrCount;
    attributes=new DataAttrInfo[attrCount];
    RM_FileScan afs;
    if(rc=afs.OpenScan(attrfh,STRING,MAXNAME+1,offsetof(DataAttrInfo,relName),EQ_OP,value,NO_HINT))
        return rc;
    int numRecs=0;
    while(1){
        RM_Record rec;
        rc=afs.GetNextRec(rec);
        if(rc==RM_EOF||numRecs>attrCount)break;
        DataAttrInfo* pattr;
        rec.GetData((char*&)pattr);
        attributes[numRecs]=*pattr;
        numRecs++;
    }
    if(numRecs!=attrCount)return SM_BADTABLE;
    if(rc=afs.CloseScan())return rc;
    return 0;
}

bool SM_Manager::IsAttrIndexed(const char* relname, const char* attrName) const{

    DataAttrInfo a;
    RID rid;
    RC rc=GetAttrFromCat(relname,attrName,a,rid);
    return a.indexNo!=-1;
}

///这个只是测试能不能get到这个relName关系，判断存不存在?
RC SM_Manager::SemCheck(const char* relName) {
    RC invalid=IsValid();if(invalid)return invalid;
    DataRelInfo rel;
    RID rid;
    return GetRelFromCat(relName,rel,rid);
}

///这个关系有没有这个属性
RC SM_Manager::SemCheck(const RelAttr& ra) const{
    RC invalid=IsValid();if(invalid)return invalid;
    DataAttrInfo a;
    RID rid;
    return GetAttrFromCat(ra.relName,ra.attrName,a,rid);
}

///暂时看不懂
RC SM_Manager::SemCheck(const AggRelAttr& ra) const{
    RC invalid=IsValid();if(invalid)return invalid;
    DataAttrInfo a;
    RID rid;
    if(ra.func!=NO_F&&
       ra.func!=MIN_F&&
       ra.func!=MAX_F&&
       ra.func!=COUNT_F
       )
        return SM_BADAGGFUN;
    return GetAttrFromCat(ra.relName,ra.attrName,a,rid);
}

///这个函数是在一些关系中查找一个属性 如果这个属性只出现在一个关系 就是正常的
///如果哪个关系都找不到这个属性 或者 出现了 多次 就是不正常的
///并且这个函数如果找到了唯一的那个关系 还会给传入的ra的标上来
RC SM_Manager::FindRelForAttr(RelAttr& ra, int nRelations,
                              const char* const possibleRelations[])const {
    RC invalid=IsValid();if(invalid)return invalid;
    if(ra.relName!=NULL)return 0;
    DataAttrInfo a;
    RID rid;
    bool found=false;
    for(int i=0;i<nRelations;i++){
        RC rc=GetAttrFromCat(possibleRelations[i],ra.attrName,a,rid);
        if(rc==0){
            if(!found){
                found=true;
                ///这个dup会在里面分配一块内存 然后把传入的字符串拷贝过去
                ///一般跟free函数搭配使用
                ra.relName=strdup(possibleRelations[i]);
            }else{
                free(ra.relName);
                ra.relName=NULL;
                return SM_AMBGATTR;
            }
        }
    }
    if(!found) return SM_NOSUCHENTRY;
    else return 0;
}

///检查一下一个条件是否合法
///比如符号两边的值是否存在 是否是同类型的可比的值
RC SM_Manager::SemCheck(const Condition& cond) const{
    if(cond.op<NO_OP||cond.op>GE_OP) return SM_BADOP;
    if(cond.lhsAttr.relName==NULL||cond.lhsAttr.attrName==NULL)
        return SM_NOSUCHENTRY;
    if(cond.bRhsIsAttr){
        if(cond.rhsAttr.relName==NULL||cond.rhsAttr.attrName==NULL)
            return SM_NOSUCHENTRY;
        DataAttrInfo a,b;
        RID rid;
        RC rc=GetAttrFromCat(cond.lhsAttr.relName,cond.lhsAttr.attrName,a,rid);
        if(rc)return SM_NOSUCHENTRY;
        rc=GetAttrFromCat(cond.rhsAttr.relName,cond.rhsAttr.attrName,b,rid);
        if(rc)return SM_NOSUCHENTRY;

        if(b.attrType!=a.attrType)return SM_TYPEMISMATCH;
    }else{
        DataAttrInfo a;
        RID rid;
        RC rc=GetAttrFromCat(cond.lhsAttr.relName,cond.lhsAttr.attrName,a,rid);
        if(rc)return SM_NOSUCHENTRY;
        if(cond.rhsValue.type!=a.attrType)return SM_TYPEMISMATCH;
    }
    return 0;
}







RC SM_Manager::GetNumPages(const char* relName) const{
    DataRelInfo r;
    RID rid;
    RC rc=GetRelFromCat(relName,r,rid);
    if(rc)return rc;
    return r.numPages;
}

RC SM_Manager::GetNumRecords(const char* relName)const {
    DataRelInfo r;
    RID rid;
    RC rc=GetRelFromCat(relName,r,rid);
    if(rc)return rc;
    return r.numRecords;
}










































