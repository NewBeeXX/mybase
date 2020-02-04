#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED

#include<iostream>
#include"mybase.h"
#include"pf.h"



struct AttrInfo{
    char *attrName;
    AttrType attrType;
    int attrLength;
};
struct RelAttr{
    char* relName;
    char* attrName;
    friend std::ostream& operator<<(std::ostream& s,const RelAttr& ra);
};
struct AggRelAttr{
    AggFun func;
    char* relName;
    char* attrName;
    friend std::ostream& operator<<(std::ostream& s,const AggRelAttr &ra);

};

struct Value{
    AttrType type;
    void* data;
    friend std::ostream& operator<<(std::ostream& s,const Value& v);
};

///等式或不等式
struct Condition{
    RelAttr lhsAttr;
    CompOp op;
    int bRhsIsAttr;
    RelAttr rhsAttr;
    Value rhsValue;
    friend std::ostream& operator<<(std::ostream& s,const Condition& c);
};

static Condition NULLCONDITION;

std::ostream& operator<<(std::ostream& s,const CompOp& op);
std::ostream& operator<<(std::ostream& s,const AggFun& func);
std::ostream& operator<<(std::ostream& s,const AttrType& at);

class QL_Manager;
class SM_Manager;

RC RBparse(PF_Manager& pfm,SM_Manager& smm,QL_Manager& qlm);

void PrintError(RC rc);

extern int bQueryPlans;




#endif // PARSER_H_INCLUDED
