#include<iostream>
#include<cstdio>
#include<string>
#include<sstream>
#include<unistd.h>
#include"rm.h"
#include"sm.h"
#include"mybase.h"
#include"catalog.h"


using namespace std;

///static 函数只能在本文件中调用
static void PrintErrorExit(RC rc){
    PrintErrorAll(rc);
    exit(rc);
}

int main(int argc,char* argv[]){
    RC rc;

    ///必须以 该程序名 <dbname> 的方式调用
    if(argc!=2){
        cerr<<"Usage: "<<argv[0]<<" <dbname> \n";
        exit(1);
    }

    string dbname(argv[1]);
    stringstream command;
    command<<"mkdir "<<dbname;
    ///构造 mkdir dbname 的命令
    rc=system(command.str().c_str());
    if(rc!=0){
        cerr<<argv[0]<<" mkdir error for"<<dbname<<"\n";
        exit(rc);
    }
    ///相当于 cd
    if(chdir(dbname.c_str())<0){
        cerr << argv[0] << " chdir error to " << dbname << "\n";
        exit(1);
    }

    ///创建系统目录
    PF_Manager pfm;
    RM_Manager rmm(pfm);
    RM_FileHandle relfh,attrfh;

    if((rc=rmm.CreateFile("relcat",))||

       )



}







