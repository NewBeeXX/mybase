#include<iostream>
#include<sstream>
#include<cstdio>
#include<cstring>
#include<unistd.h>
#include"rm.h"
#include"sm.h"
#include"mybase.h"

using namespace std;

int main(int argc,char* argv[]){
    RC rc;

    if(argc!=2){
        cerr << "Usage: " << argv[0] << " <dbname> \n";
        exit(1);
    }

    ///删数据库的话 直接把整个文件夹删掉即可
    string dbname(argv[1]);
    stringstream command;
    command<<"rm -r "<<dbname;
    rc=system(command.str().c_str());
    if(rc!=0){
        cerr << argv[0] << " rmdir error for " << dbname << "\n";
        exit(rc);
    }

    return 0;
}
