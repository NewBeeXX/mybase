#ifndef SM_H
#define SM_H


#include"sm_error.h"
#include<cstdlib>
#include<cstring>
#include"mybase.h"
#include"parser.h"
#include"rm.h"
#include"ix.h"
#include"catalog.h"
#include<string>
#include<map>


class SM_Manager{
    friend class QL_Manager;
public:
    SM_Manager(IX_Manager &ixm_,RM_Manager &rmm_);
    ~SM_Manager();

    RC OpenDb(const char* dbName);
    RC CloseDb();

    RC CreateTable(const char* relName,int attrCount,AttrInfo *attributes);

    RC CreateIndex(const char* relName,const char* attrName);

    RC DropTable(const char* relName);

    RC DropIndex(const char* relName,const char*attrName);

    RC Load(const char* relName,const char* fileName);

    RC Help();

    RC Help(const char* relName);

    RC Print(const char* relName);

    RC Set(const char* paramName,const char* value);

    RC Get(const string& paramName,string& value)const;

    RC IsValid()const;

    RC GetFromTable(const char* relName,int &attrCount,DataAttrInfo*& attributes);
    RC GetRelFromCat(const char* relName,DataRelInfo& rel,RID& rid);

    RC GetAttrFromCat(const char* relName,const char* attrName,
                      DataAttrInfo& attr,RID& rid
                      )const;

    RC GetNumPages(const char* relName)const;
    RC GetNumRecords(const char* relName)const;

    RC SemCheck(const char* relName)const;
    RC SemCheck(const RelAttr& ra)const;
    RC SemCheck(const AggRelAttr& ra)const;
    RC SemCheck(const Condition& cond)const;

    RC FindRelForAttr(RelAttr& ra,int nRelations,const char* const possibleRelations[])const;

    RC LoadRecord(const char* relName,int buflen,const char buf[]);

    bool IsAttrIndexed(const char* relname,const char* attrName)const;

    RC DropIndexFromAttrCatAlone(const char* relName,const char* attrName);
    RC ResetIndexFromAttrCatAlone(const char* relName,const char* attrName);

private:

    RM_Manager& rmm;
    IX_Manager& ixm;
    bool bDBOpen;
    RM_FileHandle relfh;
    RM_FileHandle attrfh;
    char cwd[1024];
    map<string,string> params;


};















#endif // SM_H
