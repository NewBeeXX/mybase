#ifndef FILE_SCAN_H_INCLUDED
#define FILE_SCAN_H_INCLUDED

#include"mybase.h"
#include"iterator.h"
#include"rm.h"
#include"sm.h"
using namespace std;

class FileScan:public Iterator{
public:
    FileScan (SM_Manager& smm,
              RM_Manager& rmm,
              const char* relName,
              RC& status,
              const Condition& cond=NULLCONDITION,
              int nOutFilters=0,
              const Condition outFilters[]=NULL
              );
    virtual ~FileScan();
    virtual RC GetNext(Tuple& t);
    virtual RC Close();
    virtual string Explain();

    RC IsValid();
    virtual RC Eof()consnt {return RM_EOF;}
    virtual int GetNumPages()const{retun psmm->GetNumPages(relName);}
    virtual int GetNumSlotsPerPage()const{return rfs.GetNumSlotsPerPage();}
    virtual int GetNumRecords()const{return psmm->GetNumRecords(relName);}
    virtual RC GotoPage(PageNum p){return rfs.GotoPage(p);}

private:
    RM_FileScan rfs;
    RM_Manager* prmm;
    SM_Manager* psmm;
    const char* relName;
    RM_FileHandle rmh;
    int nOFilters;
    Condition* oFilters;
};

#endif // FILE_SCAN_H_INCLUDED
