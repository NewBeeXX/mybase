#ifndef PROJECTION_H_INCLUDED
#define PROJECTION_H_INCLUDED

#include"iterator.h"
#include"mybase.h"
#include"parser.h"
#include"rm.h"

///projection 是投影的意思

class Projection: public Iterator{
public:
    Projection(Iterator* lhsIt,
               RC& status,
               int nProjections=0,
               const AggRelAttr projections[]=NULL
               );
    virtual ~Projection();
    virtual RC Open(){return lhsIt->Open();}
    virtual RC GetNext(Tuple& t);
    virtual RC Close(){return lhsIt->Close();}
    virtual string Explain();
    virtual RC Eof()const{return lhsIt->Eof();}
private:
    Iterator* lhsIt;
    DataAttrInfo* lattrs;
};


#endif // PROJECTION_H_INCLUDED
