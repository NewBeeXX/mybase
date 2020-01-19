#include<cerrno>
#include<cstdio>
#include<cstring>
#include<iostream>
#include "predicate.h"


using namespace std;

///这里写一个内部自己用的没有声明的函数
///比较浮点数相等 只需要delta<eps
///当然下面也可以不用
bool AlmostEqualRelative(float A,float B,float maxRelativeError=1e-6) {
    return fabs(A-B)<=maxRelativeError;
}

bool Predicate::eval(const char* buf, CompOp c)const {
    return this->eval(buf,NULL,c);
}

bool Predicate::eval(const char* buf, const char* rhs, CompOp c) const{
    const void* value_=rhs;
    ///如果不传rhs的话，直接拿初始传入的value作为_
    if(rhs==NULL)value_=value;

    ///不用比较
    if(c==NO_OP||value_==NULL)return true;

    ///原来控制一个属性的位置也是在上层，这里只是加一个偏移量
    const char* attr=buf+attrOffset;

    if(c==LT_OP){//<
        if(attrType==INT)return *((int*)attr)<*((int*)value_);
        if(attrType==FLOAT)return *((float*)attr)<*((float*)value_);
        if(attrType==STRING)
            return strncmp(attr,(char*)value_,attrLength)<0;
    }else if(c==GT_OP){
        if(attrType==INT)return *((int*)attr)>*((int*)value_);
        if(attrType==FLOAT)return *((float*)attr)>*((float*)value_);
        if(attrType==STRING)
            return strncmp(attr,(char*)value_,attrLength)>0;
    }else if(c==EQ_OP){
        if(attrType==INT)return *((int*)attr)==*((int*)value_);
        if(attrType==FLOAT)return *((float*)attr)==*((float*)value_);
        if(attrType==STRING)
            return strncmp(attr,(char*)value_,attrLength)==0;
    }else if(c==LE_OP){
        ///下面这几段写的挺好
        return this->eval(buf,rhs,LT_OP)||this->eval(buf,rhs,EQ_OP);
    }else if(c==GE_OP){
        return this->eval(buf,rhs,GT_OP)||this->eval(buf,rhs,EQ_OP);
    }else if(c==NE_OP){
        return !this->eval(buf,rhs,EQ_OP);
    }

    ///到这里就是c类型不对
    return 1;

}












