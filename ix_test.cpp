#include<cstdio>
#include<iostream>
#include<cstring>
#include<cstdlib>
#include<ctime>

#include"mybase.h"
#include"pf.h"
#include"rm.h"
#include"ix.h"

using namespace std;

const char* FILENAME="testrel";
#define BADFILE "/abc/def/xyz"
#define STRLEN 39
#define FEW_ENTRIES 20
#define MANY_ENTRIES 1000
#define NENTRIES 5000
#define PROG_UNIT 200

int values[NENTRIES];

PF_Manager pfm;
RM_Manager rmm(pfm);
IX_Manager ixm(pfm);

RC Test1();
RC Test2();
RC Test3();
RC Test4();
RC Test5();
RC Test6();

void PrintErrorAll(RC rc);
void LsFiles(const char* fileName);
void ran(int n);

RC InsertIntEntries(IX_IndexHandle &ih,int nEntries);
RC InsertFloatEntries(IX_IndexHandle &ih,int nEntries);
RC InsertStringEntries(IX_IndexHandle &ih,int nEntries);
Add Recs(RM_FileHandle& fh,int nRecs);
RC DeleteIntEntries(IX_IndexHandle &ih,int nEntries);
RC DeleteFloatEntries(IX_IndexHandle &ih,int nEntries);
RC DeleteStringEntries(IX_IndexHandle &ih,int nEntries);
RC VerifyIntIndex(IX_IndexHandle &ih,int nStart,iint nEntries);
RC VerifyFloatIndex(IX_IndexHandle &ih,int nStart,iint nEntries);

RC PrintIndex(IX_IndexHandle &ih);

#define NUM_TESTS 6

int (*tests[])() = {
    Test1,
    Test2,
    Test3,
    Test4,
    Test5,
    Test6,
};



int main(){
    RC rc;

    int testNum;
    cout<<"开始ix层测试"<<endl;

    srand(time(0));

    rmm.DestroyFile(FILENAME);
    ixm.DestroyIndex(FILENAME,0);
    ixm.DestroyIndex(FILENAME,1);
    ixm.DestroyIndex(FILENAME,2);
    ixm.DestroyIndex(FILENAME,3);

    for(int i=0;i<NUM_TESTS;i++){
        if(rc=tests[i]()){
            PrintErrorAll(rc);
            return 1;
        }
    }

    cout<<"完成ix层测试"<<endl;
    return 0;

}


void LsFiles(const char *fileName){
   char command[80];
   sprintf(command, "ls -l *%s*", fileName);
   printf("Doing \"%s\"\n", command);
   system(command);
}

///随机生成n个不重复的数 0～n-1 使用了传说中的洗牌算法
void ran(int n){
    for(int i=0;i<NENTRIES;i++)values[i]=i;
    for(int i=n-1;i>=0;i--){
        int r=rand()%(i+1);
        swap(value[r],value[i]);
    }
}



RC InsertIntEntries(IX_IndexHandle &ih,int nEntries){
    RC rc;
    int i;
    int value;
    cout<<"插入"<<nEntries<<"个int类型进索引"<<endl;

    ran(nEntries);
    for(i=0;i<nEntries;i++){
        value=values[i]+1;
        RID rid(value,value*2);///虚构一个 位置指针而已 不会真正去取
        if(rc=ih.InsertEntry((void*)&value,rid))return rc;
        if((i+1)%PROG_UNIT==0){
            printf("\r\t%d%%    ",(int)((i+1)*100)/nEntries);
            fflush(stdout);
        }
    }

    printf("\r\t%d%%    \n",(int)((i)*100)/nEntries);

    return 0;
}



RC InsertFloatEntries(IX_IndexHandle &ih,int nEntries){
    RC rc;
    int i;
    float value;
    cout<<"插入"<<nEntries<<"个float类型进索引"<<endl;

    ran(nEntries);
    for(i=0;i<nEntries;i++){
        value=values[i]+1;
        RID rid(values[i]+1,(values[i]+1)*2);///虚构一个 位置指针而已 不会真正去取
        if(rc=ih.InsertEntry((void*)&value,rid))return rc;
        if((i+1)%PROG_UNIT==0){
            printf("\r\t%d%%    ",(int)((i+1)*100)/nEntries);
            fflush(stdout);
        }
    }

    printf("\r\t%d%%    \n",(int)((i)*100)/nEntries);

    return 0;

}


RC InsertStringEntries(IX_IndexHandle &ih,int nEntries){
    RC rc;
    int i;
    char value[STRLEN];
    cout<<"插入"<<nEntries<<"个string类型进索引"<<endl;
    ran(nEntries);
    for(i=0;i<nEntries;i++){
        memset(value,' ',STRLEN);
        sprintf(value,"number %d",values[i]+1);
        RID rid(values[i]+1,(values[i]+1)*2);
        if(rc=ih.InsertEntry((void*)&value,rid))return rc;
        if((i+1)%PROG_UNIT==0){
            printf("\r\t%d%%    ",(int)((i+1)*100)/nEntries);
            fflush(stdout);
        }
    }

    printf("\r\t%d%%    \n",(int)((i)*100)/nEntries);

    return 0;
    }

}


Add Recs(RM_FileHandle& fh,int nRecs);
RC DeleteIntEntries(IX_IndexHandle &ih,int nEntries);
RC DeleteFloatEntries(IX_IndexHandle &ih,int nEntries);
RC DeleteStringEntries(IX_IndexHandle &ih,int nEntries);
RC VerifyIntIndex(IX_IndexHandle &ih,int nStart,iint nEntries);
RC VerifyFloatIndex(IX_IndexHandle &ih,int nStart,iint nEntries);





