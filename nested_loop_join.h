#ifndef NESTED_LOOP_JOIN_H_INCLUDED
#define NESTED_LOOP_JOIN_H_INCLUDED

#include"mybase.h"
#include"iterator.h"
#include"ix.h"
#include"sm.h"
#include"rm.h"
#include"ql_error.h"


using namespace std;

class NestedLoopJoin:public virtual Iterator{
public:
    NestedLoopJoin(Iterator* lhsIt,
                   Iterator* rhsIt,
                   RC& status,
                   int nOutFilters=0,
                   const Condition outFilters[]=NULL
                   );

    virtual ~NestedLoopJoin();
    virtual RC Open();
    virtual RC GetNext(Tuple& t);
    virtual RC Close();
    virtual string Explain();

    RC IsValid();
    virtual RC Eof()const{return QL_EOF;}
    RC virtual ReopenIfIndexJoin(const Tuple& t){return 0;}

protected:
    void virtual EvalJoin(Tuple& t,bool& joined,Tuple*l,Tuple* r);

protected:
    Iterator* lhsIt;
    Iterator* rhsIt;
    Tuple left;
    Tuple right;
    int nOFilters;
    Condition* oFilters;
    DataAttrInfo* lKeys;
    DataAttrInfo* rKeys;


};



















#endif // NESTED_LOOP_JOIN_H_INCLUDED
