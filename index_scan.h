#ifndef INDEX_SCAN_H_INCLUDED
#define INDEX_SCAN_H_INCLUDED


#include"mybase.h"
#include"iterator.h"
#include"ix.h"
#include"sm.h"
#include"rm.h"

using namespace std;

///public继承 不改变基类的成员的访问属性 protected继承 就会把基类的public变成子类的public private就会把所有都变private
class IndexScan: public Iterator{
public:
    IndexScan(SM_Manager& smm,
              RM_Manager& rmm,
              IX_Manager& ixm,
              const char* relName,
              const char* indexAttrName,
              RC& status,
              const Condition& cond=NULLCONDITION,
              int nOutFilters=0,
              const Condition outFilters[]=NULL,
              bool desc=false
              );
    virtual ~IndexScan();
    virtual RC Open();
    virtual RC GetNext(Tuple& t);
    virtual RC Close();
    virtual string Explain();

    RC IsValid();
    virtual RC Eof()const{return IX_EOF;}
    virtual RC ReOpenScan(void* newData);
    virtual string GetIndexAttr()const{return attrName;}
    virtual string GetIndexRel()const{return relName;}
    virtual bool IsDesc()const{return ifs.IsDesc();}

private:
    IX_IndexScan ifs;
    IX_Manager* pixm;
    RM_Manager* prmm;
    SM_Manager* psmm;
    string relName;
    string attrName;
    RM_FileHandle rmh;
    IX_IndexHandle ixh;
    int nOFilters;
    Condition* oFilters;
    CompOp c;
};









#endif // INDEX_SCAN_H_INCLUDED
