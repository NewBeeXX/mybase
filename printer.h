#ifndef PRINTER_H
#define PRINTER_H

#include<iostream>
#include<cstring>
#include"mybase.h"
#include"catalog.h"
#include"data_attr_info.h"
#include"iterator.h"

#define MAXPRINTSTRING (2*MAXNAME+5)

#ifndef mmin
#define mmin(a,b) ((a)<(b)?(a):(b))
#endif // mmin

#ifndef mmax
#define mmax(a,b) ((a)>(b)?(a):(b))
#endif // mmin

void Spaces(int maxLength,int printedSoFar);
class DataAttrInfo;
class Tuple;
class Printer{
public:
    Printer(const DataAttrInfo* attributes,const int attrCount);
    Printer(const Tuple& t);
    ~Printer();
    void PrintHeader(std::ostream& c)const;

    ///一个不可改的指针指向不可改的内容
    void Print(std::ostream& c,const char* const data);
    void Print(std::ostream& c,const void* const data[]);
    void Print(std::ostream& c,const Tuple& t);

    void PrintFooter(std::ostream& c)const;

private:
    void Init(const DataAttrInfo *attributes_,const int attrCount_);
private:
    DataAttrInfo *attributes;
    int attrCount;
    char **psHeader;
    int *spaces;
    int iCount;
};













#endif // PRINTER_H
