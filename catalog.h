#ifndef CATALOG_H
#define CATALOG_H


#include"printer.h"
#include"parser.h"

// in printer.h
// struct DataAttrInfo {
//     char     relName[MAXNAME+1];    // Relation name
//     char     attrName[MAXNAME+1];   // Attribute name
//     int      offset;                // Offset of attribute
//     AttrType attrType;              // Type of attribute
//     int      attrLength;            // Length of attribute
//     int      indexNo;               // Index number of attribute
// }


struct DataRelInfo{
    DataRelInfo(){
        memset(relName,0,MAXNAME+1);
    }
    DataRelInfo(char* buf){
        memcpy(this,buf,DataRelInfo::size());
        ///静态函数
    }
    DataRelInfo(const DataRelInfo &d){
        strcpy(relName,d.relName);
        recordSize=d.recordSize;
        attrCount=d.attrCount;
        numPages=d.numPages;
        numRecords=d.numRecords;
    }

    DataRelInfo& operator=(const DataRelInfo& d){
        if(this!=&d){
            strcpy(relName,d.relName);
            recordSize=d.recordSize;
            attrCount=d.attrCount;
            numPages=d.numPages;
            numRecords=d.numRecords;
        }
        return *this;
    }
    static unsigned int size(){
        return MAXNAME+1+4*sizeof(int);
    }
    static unsigned int members(){return 5;}

    int recordSize; ///每一个元组的size
    int attrCount; ///col数
    int numPages;///这个关系占用的分页数
    int numRecords;///元组数
    char relName[MAXNAME+1];
};










#endif // CATALOG_H
