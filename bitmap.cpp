#include "rm.h"
#include<cmath>
#include<cstring>

///bitmap的size是多少bit的意思

bitmap::bitmap(int numBits):size(numBits) {
    buffer=new char[this->numChars()];
    memset((void*)buffer,0,this->numChars());
    this->reset();
}

bitmap::bitmap(char* buf, int numBits):size(numBits) {
    buffer=new char[this->numChars()];
    memcpy(buffer,buf,this->numChars());
}

int bitmap::to_char_buf(char* b, int len) const{
    memcpy((void*)b,buffer,len);
    return 0;
}

bitmap::~bitmap() {
    delete [] buffer;
}

int bitmap::numChars() const{
    return (size-1)/8+1;
}

void bitmap::reset() {
    for(int i=0;i<size;i++)this->reset(i);
}

void bitmap::reset(unsigned int bitNumber) {
    int byte=bitNumber/8;
    int offset=bitNumber%8;
    buffer[byte]&=~(1<<offset);
}


void bitmap::set() {
    for(int i=0;i<size;i++)this->set(i);
}

///下标0开始
void bitmap::set(unsigned int bitNumber) {
    int byte=bitNumber/8;
    int offset=bitNumber%8;
    buffer[byte]|=(1<<offset);
}

bool bitmap::test(unsigned int bitNumber)const {
    int byte=bitNumber/8;
    int offset=bitNumber%8;
    return buffer[byte]&(1<<offset);
}

ostream& operator<<(ostream& os,const bitmap& b){
    os<<"[";
    for(int i=0;i<b.getSize();i++){
        if(i!=0&&i%8==0){
            os<<".";
        }
        os<<(b.test(i)?1:0);
    }
    os<<"]";
    return os;
}














