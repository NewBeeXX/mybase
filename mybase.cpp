///dbms的主要驱动

#include<iostream>
#include<cstdio>
#include<cstring>
#include<unistd.h>
#include"mybase.h"
#include"rm.h"
#include"sm.h"
#include"ql.h"
#include"parser.h"

using namespace std;

static void PrintErrorExit(RC rc) {
  PrintErrorAll(rc);
  exit(rc);
}



int main(int argc,char* argv[]){
    RC rc;
    if(argc!=2){
        cerr<<"Usage: "<<argv[0]<<" dbname\n";
        exit(1);
    }
    char* dbname=argv[1];
    PF_Manager pfm;
    RM_Manager rmm(pfm);
    IX_Manager ixm(pfm);
    SM_Manager smm(ixm,rmm);
    QL_Manager qlm(smm,ixm,rmm);

    if(rc=smm.OpenDb(dbname))PrintErrorExit(rc);

    RC parseRC=RBparse(pfm,smm,qlm);

    if(rc=smm.CloseDb())PrintErrorExit(rc);

    if(parseRC!=0)PrintErrorExit(parseRC);

    cout<<"Bye.\n";

    return 0;
}







