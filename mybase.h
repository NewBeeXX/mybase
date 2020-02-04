#ifndef MYBASE_H
#define MYBASE_H

#include<iostream>
#include<cmath>
#include<cassert>


#define MAXNAME 24 ///col名的最大长度?
#define MAXSTRINGLEN 255 ///最大字符串长
#define MAXATTRS ///最大列数

#define YY_SKIP_YYWRAP 1
#define yywrap() 1

#ifndef offsetof
#define offsetof(type,field) ((size_t) &( ((type*)0)->field )  )
#endif // offsetof

void yyerror(const char *);


typedef int RC;

#define OK_RC         0    // OK_RC return code is guaranteed to always be 0

#define START_PF_ERR  (-1)
#define END_PF_ERR    (-100)
#define START_RM_ERR  (-101)
#define END_RM_ERR    (-200)
#define START_IX_ERR  (-201)
#define END_IX_ERR    (-300)
#define START_SM_ERR  (-301)
#define END_SM_ERR    (-400)
#define START_QL_ERR  (-401)
#define END_QL_ERR    (-500)

#define START_PF_WARN  1
#define END_PF_WARN    100
#define START_RM_WARN  101
#define END_RM_WARN    200
#define START_IX_WARN  201
#define END_IX_WARN    300
#define START_SM_WARN  301
#define END_SM_WARN    400
#define START_QL_WARN  401
#define END_QL_WARN    500


const int ALL_PAGES = -1;


//
// Attribute types
//
enum AttrType {
    INT,
    FLOAT,
    STRING
};

//
// Comparison operators
//
enum CompOp {
    NO_OP,                                      // no comparison
    EQ_OP, NE_OP, LT_OP, GT_OP, LE_OP, GE_OP    // binary atomic operators
};

//
// Aggregation functions for group by
//
enum AggFun {
  NO_F,
  MIN_F, MAX_F, COUNT_F,
  SUM_F, AVG_F           // numeric args only
};

//
// Pin Strategy Hint
//
enum ClientHint {
    NO_HINT                                     // default value
};

//
// TRUE, FALSE and BOOLEAN
//
#ifndef BOOLEAN
typedef char Boolean;
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef NULL
#define NULL 0
#endif


#endif // MYBASE_H
