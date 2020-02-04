#ifndef MERGE_JOIN_H_INCLUDED
#define MERGE_JOIN_H_INCLUDED

#include"nested_loop_join.h"
#include"mybase.h"
#include"iterator.h"
#include"rm.h"
#include<vector>

using namespace std;

class MergeJoin: public NestedLoopJoin{
public:
    MergeJoin(Iterator* lhsIt,
              Iterator* rhsIt,
              RC& status,
              int nJoinConds,
              int equiCond,
              const Condition joinConds_[]
              );

    virtual ~MergeJoin(){
        delete pcurrTuple;
        delete potherTuple;
    }
    virtual RC Open(){
        firstOpen=true;
        ///调用父类方法
        return this->NestedLoopJoin::Open();
    }
    virtual RC IsValid(){
        if((curr==lhsIt&&other==rhsIt)||(curr==rhsIt&&other==lhsIt))
            return this->NestedLoopJoin::IsValid();
        return SM_BADTABLE;
    }
    virtual RC GetNext(Tuple& t);

private:
    bool firstOpen;
    Iterator* curr;
    Iterator* other;
    Condition equi;
    int equiPos;
    bool sameValueCross;
    vector<Tuple>cpvec;
    vector<Tuple>::iterator cpit;
    bool lastCpit;
    Tuple* pcurrTuple;
    Tuple* potherTuple;

};



#endif // MERGE_JOIN_H_INCLUDED
