#ifndef QL_H_INCLUDED
#define QL_H_INCLUDED

#include<cstdlib>
#include<cstring>
#include"mybase.h"
#include"parser.h"
#include"ql_error.h"
#include"rm.h"
#include"ix.h"
#include"sm.h"
#include"iterator.h"


class QL_Manager{
public:
    QL_Manager(SM_Manager &smm,IX_Manager& ixm,RM_Manager& rmm);
    ~QL_Manager();
    RC Select(int nSelAttrs, ///被选中的属性
              const AggRelAttr selAttrs[],
              int nRelations, ///from的关系
              const char* const relations[],
              int nConditions,
              const Condition conditions[],
              int order, ///顺序
              RelAttr orderAttr, /// 需要排序的属性 order by
              bool group,
              RelAttr groupAttr);

    RC Insert(const char* relName,
              int nValues,
              const Value values[]);

    RC Delete(const char* relName,
              int nConditions,
              const Condition conditions[]
              );

    RC Update(const char* relName,
              const RelAttr &updAttr,
              const int bIsValue,
              const RelAttr &rhsRelAttr,
              const Value &rhsValue,
              int nConditions,
              const Condition conditions[]
              );
public:
    RC IsValid()const;
    Iterator* GetLeafIterator(const char* relName,
                              int nConditions,
                              const Condition conditions[],
                              int nJoinConditions=0,
                              const Condition jconditions[]=NULL,
                              int order=0,
                              RelAttr* porderAttr=NULL
                              );

    RC MakeRootIterator(Iterator*& newit,
                        int nSelAttrs,
                        const AggRelAttr selAttrs[],
                        int nRelations,
                        const char* const relations[],
                        int order,
                        RelAttr orderAttr,
                        bool group,
                        RelAttr groupAttr
                        )const;

    RC PrintIterator(Iterator* it)const;
    void GetCondsForSingleRelation(int nConditions,
                                   Condition conditions[],
                                   char* relName,
                                   int& retCount,
                                   Condition*& retConds
                                   )const;
    void GetCondsForTwoRelations(int nConditions,
                                 Condition conditions[],
                                 int nRelsSoFar,
                                 char* relations[],
                                 char* relName2,
                                 int& retCount,
                                 Condition*& retConds
                                 )const;
private:
    RM_Manager &rmm;
    IX_Manager &ixm;
    SM_Manager &smm;
};


#endif // QL_H_INCLUDED
