#ifndef NESTED_LOOP_INDEX_JOIN_H_INCLUDED
#define NESTED_LOOP_INDEX_JOIN_H_INCLUDED

#include"nested_loop_join.h"
#include"index_scan.h"

using namespace std;
class NestedLoopIndexJoin: public NestedLoopJoin{
public:
    NestedLoopIndexJoin(Iterator* lhsIt,
                        IndexScan* rhsIt,
                        RC& status,
                        int nOutFilters,
                        const Condition outFilters[]
                        ):NestedLoopJoin(lhsIt, rhsIt, status, nOutFilters, outFilters){
        if(nOutFilters<1||outFilters==NULL){
            status=QL_BADJOINKEY;
            return ;
        }
        explain.str("");
        explain << "NestedLoopIndexJoin\n";
        if(nOFilters>0){
            explain << "   nJoinConds = " << nOutFilters << "\n";
            for(int i=0;i<nOutFilters;i++){
                explain << "   joinConds[" << i << "]:" << outFilters[i] << "\n";
            }
        }
    }
    RC virtual ReopenIfIndexJoin(const Tuple& t){
        IndexScan* rhsIxIt=dynamic_cast<IndexScan*>(rhsIt);
        if(rhsIxIt==NULL)return 0;
        void* newValue=NULL;
        t.Get(rhsIxIt->GetIndexAttr().c_str(),newValue);
        return rhsIxIt->ReOpenScan(newValue);

    }


};


#endif // NESTED_LOOP_INDEX_JOIN_H_INCLUDED
