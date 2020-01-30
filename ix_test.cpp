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
RC AddRecs(RM_FileHandle& fh,int nRecs);
RC DeleteIntEntries(IX_IndexHandle &ih,int nEntries);
RC DeleteFloatEntries(IX_IndexHandle &ih,int nEntries);
RC DeleteStringEntries(IX_IndexHandle &ih,int nEntries);
RC VerifyIntIndex(IX_IndexHandle &ih,int nStart,int nEntries,int bExists);
RC VerifyFloatIndex(IX_IndexHandle &ih,int nStart,int nEntries,int bExists);

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
        swap(values[r],values[i]);
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


RC AddRecs(RM_FileHandle& fh,int nRecs){
    RC rc;
    int i;
    int value;
    RID rid;
    PageNum pageNum;
    SlotNum slotNum;

    printf("插入%d条记录进入RM文件\n",nRecs);
    ran(nRecs);

    for(i=0;i<nRecs;i++){
            value=values[i]+1;
            if((rc=fh.InsertRec((char*)&value,rid)) ||
               (rc=rid.GetPageNum(pageNum))||
               (rc=rid.GetSlotNum(slotNum))
               )
                return rc;
            if((i+1)%PROG_UNIT==0){
                printf("\r\t%d%%      ", (int)((i+1)*100L/nRecs));
                fflush(stdout);
            }
    }
    printf("\r\t%d%%      \n", (int)(i*100L/nRecs));
    return 0;
}


RC DeleteIntEntries(IX_IndexHandle &ih,int nEntries){
    RC rc;
    int i;
    int value;
    printf("在索引中删除删除%d条int记录\n",nEntries);

    ran(nEntries); ///这里随机只是为了随机顺序 可以保证里面肯定有这条记录
    for(i=0;i<nEntries;i++){
        value=values[i]+1;
        RID rid(value,value*2);
        if(rc=ih.DeleteEntry((void*)&value,rid))return rc;
        if((i + 1) % PROG_UNIT == 0){
            printf("\r\t%d%%     ", (int)((i+1)*100L/nEntries));
            fflush(stdout);
        }
   }
   printf("\r\t%d%%      \n", (int)(i*100L/nEntries));
   return 0;
}


RC DeleteFloatEntries(IX_IndexHandle &ih,int nEntries){
    RC rc;
    int i;
    float value;
    printf("在索引中删除删除%d条float记录\n",nEntries);

    ran(nEntries); ///这里随机只是为了随机顺序 可以保证里面肯定有这条记录
    for(i=0;i<nEntries;i++){
        value=values[i]+1;
        RID rid(values[i]+1,(values[i]+1)*2);
        if(rc=ih.DeleteEntry((void*)&value,rid))return rc;
        if((i + 1) % PROG_UNIT == 0){
            printf("\r\t%d%%     ", (int)((i+1)*100L/nEntries));
            fflush(stdout);
        }
   }
   printf("\r\t%d%%      \n", (int)(i*100L/nEntries));
   return 0;

}


RC DeleteStringEntries(IX_IndexHandle &ih,int nEntries){
    RC rc;
    int i;
    char value[STRLEN];
    printf("在索引中删除删除%d条string记录\n",nEntries);

    ran(nEntries); ///这里随机只是为了随机顺序 可以保证里面肯定有这条记录
    for(i=0;i<nEntries;i++){
        sprintf(value,"number %d",values[i]+1);
        RID rid(values[i]+1,(values[i]+1)*2);
        if(rc=ih.DeleteEntry((void*)&value,rid))return rc;
        if((i + 1) % PROG_UNIT == 0){
            printf("\r\t%d%%     ", (int)((i+1)*100L/nEntries));
            fflush(stdout);
        }
   }
   printf("\r\t%d%%      \n", (int)(i*100L/nEntries));
   return 0;
}


RC VerifyIntIndex(IX_IndexHandle &ih,int nStart,int nEntries,int bExists){
    RC rc;
    int i;
    RID rid;
    IX_IndexScan scan;
    PageNum pageNum;
    SlotNum slotNum;


//    int flag=rand();
//    printf("确认索引内容 int nstart:%d 个数:%d 存在?:%d 标识符:%d\n",nStart,nEntries,bExists,flag);
    for(i=nStart;i<nStart+nEntries;i++){
//        printf("flag:%d i:%d\n",flag,i);

        int value=values[i]+1;
        if(rc=scan.OpenScan(ih,EQ_OP,&value)){
            printf("打开扫描失败\n");
            return rc;
        }
        rc=scan.GetNextEntry(rid);

        if(!bExists&&rc==0){
            printf("应该不存在的 结果找到了 value:%d\n",value);
            return IX_EOF;
        }else if(bExists&&rc==IX_EOF){
            printf("应该存在的 结果没找到 value:%d\n",value);
            return IX_EOF;
        }else if(rc!=0&&rc!=IX_EOF)return rc;

        if(bExists&&rc==0){
            if((rc=rid.GetPageNum(pageNum))||
               (rc=rid.GetSlotNum(slotNum))
               ) return rc;

            if(pageNum!=value||slotNum!=value*2){
                printf("出错 value为%d 的记录 rid却是(%d,%d)\n",value,pageNum,slotNum);
                return IX_EOF;
            }
            rc=scan.GetNextEntry(rid);
            if(rc==0){
                printf("一个value本应只有一个 却发现2个 value %d\n",value);
                return IX_EOF;
            }else if(rc!=IX_EOF)return rc;
        }
        if(rc=scan.CloseScan()){
            printf("关闭扫描出错\n");
            return rc;
        }
    }
    return 0;
}




RC VerifyFloatIndex(IX_IndexHandle &ih,int nStart,int nEntries,int bExists){
    RC rc;
    int i;
    RID rid;
    IX_IndexScan scan;
    PageNum pageNum;
    SlotNum slotNum;

    printf("确认索引内容\n");
    for(i=nStart;i<nStart+nEntries;i++){
        float value=values[i]+1;
        if(rc=scan.OpenScan(ih,EQ_OP,&value)){
            printf("打开扫描失败\n");
            return rc;
        }
        rc=scan.GetNextEntry(rid);

        if(!bExists&&rc==0){
            printf("应该不存在的 结果找到了 value:%f\n",value);
            return IX_EOF;
        }else if(bExists&&rc==IX_EOF){
            printf("应该存在的 结果没找到 value:%f\n",value);
            return IX_EOF;
        }else if(rc!=0&&rc!=IX_EOF)return rc;

        if(bExists&&rc==0){
            if((rc=rid.GetPageNum(pageNum))||
               (rc=rid.GetSlotNum(slotNum))
               ) return rc;

            if(pageNum!=value||slotNum!=value*2){
                printf("出错 value为%f 的记录 rid却是(%d,%d)\n",value,pageNum,slotNum);
                return IX_EOF;
            }
            rc=scan.GetNextEntry(rid);
            if(rc==0){
                printf("一个value本应只有一个 却发现2个 value %f\n",value);
                return IX_EOF;
            }else if(rc!=IX_EOF)return rc;
        }
        if(rc=scan.CloseScan()){
            printf("关闭扫描出错\n");
            return rc;
        }
    }
    return 0;
}


RC Test1(){
    RC rc;
    int index=0;
    IX_IndexHandle ih;

    printf("测试1:对索引的 创建 打开 关闭 删除\n");

    if((rc=ixm.CreateIndex(FILENAME,index,INT,sizeof(int)))||
       (rc=ixm.OpenIndex(FILENAME,index,ih))||
       (rc=ixm.CloseIndex(ih))
       ) return rc;
    LsFiles(FILENAME);
    if(rc=ixm.DestroyIndex(FILENAME,index))return rc;
    printf("通过测试1\n\n");
    return 0;
}


RC Test2(){
    RC rc;
    IX_IndexHandle ih;
    int index=0;

    printf("测试2: 插入整形节点进入索引\n");
    if((rc=ixm.CreateIndex(FILENAME,index,INT,sizeof(int)))||
       (rc=ixm.OpenIndex(FILENAME,index,ih))||
       (rc=InsertIntEntries(ih,FEW_ENTRIES))||
       (rc=ixm.CloseIndex(ih))||
       (rc=ixm.OpenIndex(FILENAME,index,ih))||
       (rc=VerifyIntIndex(ih,0,FEW_ENTRIES,TRUE))||
///!!!
       (rc=VerifyIntIndex(ih,FEW_ENTRIES,1,FALSE))||
       (rc=ixm.CloseIndex(ih))
       ) return rc;

    LsFiles(FILENAME);
    if(rc=ixm.DestroyIndex(FILENAME,index))return rc;
    printf("通过测试2\n\n");
    return 0;
}

RC Test3(){
    RC rc;
    int index=0;
    int nDelete=FEW_ENTRIES*8/10;
    IX_IndexHandle ih;
    printf("测试3: 删除一部分整形 \n");
    if((rc=ixm.CreateIndex(FILENAME,index,INT,sizeof(int)))||
       (rc=ixm.OpenIndex(FILENAME,index,ih))||
       (rc=InsertIntEntries(ih,FEW_ENTRIES))||
       (rc=DeleteIntEntries(ih,nDelete))||
       (rc=ixm.CloseIndex(ih))||
       (rc=ixm.OpenIndex(FILENAME,index,ih))||
       ///检验看看这些value是否已经被删
       (rc=VerifyIntIndex(ih,0,nDelete,FALSE))||
       (rc=VerifyIntIndex(ih,nDelete,FEW_ENTRIES-nDelete,TRUE))||
       (rc=ixm.CloseIndex(ih))
       )
       return rc;
    LsFiles(FILENAME);
    if(rc=ixm.DestroyIndex(FILENAME,index))return rc;
    printf("完成测试3\n\n");
    return 0;
}


RC Test4(){
    RC rc;
    IX_IndexHandle ih;
    int index=0;
    int i;
    int value=FEW_ENTRIES/2;
    RID rid;

    printf("测试4: 非等值扫描\n");

    if((rc=ixm.CreateIndex(FILENAME,index,INT,sizeof(int)))||
       (rc=ixm.OpenIndex(FILENAME,index,ih))||
       (rc=InsertIntEntries(ih,FEW_ENTRIES))
       ) return rc;

    /// < 关系 扫描
    IX_IndexScan scanlt;
    if(rc=scanlt.OpenScan(ih,LT_OP,&value)){
        printf("无法打开扫描\n");
        return rc;
    }

    i=0;
    while(!(rc=scanlt.GetNextEntry(rid)))
        i++;
    if(rc!=IX_EOF)return rc;

    printf("此次扫描 < 找到%d个记录 \n",i);

    IX_IndexScan scanle;
    if(rc=scanle.OpenScan(ih,LE_OP,&value)){
        printf("无法打开扫描\n");
        return rc;
    }
    i=0;
    while(!(rc=scanle.GetNextEntry(rid)))i++;
    if(rc!=IX_EOF)return rc;

    printf("此次扫描 <= 找到%d个记录 \n",i);


    IX_IndexScan scangt;
    if(rc=scangt.OpenScan(ih,GT_OP,&value)){
        printf("无法打开扫描\n");
        return rc;
    }
    i=0;
    while(!(rc=scangt.GetNextEntry(rid)))i++;
    if(rc!=IX_EOF)return rc;

    printf("此次扫描 > 找到%d个记录 \n",i);


    IX_IndexScan scange;
    if(rc=scange.OpenScan(ih,GE_OP,&value)){
        printf("无法打开扫描\n");
        return rc;
    }
    i=0;
    while(!(rc=scange.GetNextEntry(rid)))i++;
    if(rc!=IX_EOF)return rc;

    printf("此次扫描 >= 找到%d个记录 \n",i);

    if(rc=ixm.CloseIndex(ih))return rc;
    LsFiles(FILENAME);
    if(rc=ixm.DestroyIndex(FILENAME,index))return rc;

    printf("通过测试4\n\n");

    return 0;


}



RC Test5(){
    RC rc;
    int index=0;
    int nDelete=FEW_ENTRIES*8/10;
    IX_IndexHandle ih;
    printf("测试5: 删除一部分浮点 \n");
    if((rc=ixm.CreateIndex(FILENAME,index,FLOAT,sizeof(float)))||
       (rc=ixm.OpenIndex(FILENAME,index,ih))||
       (rc=InsertFloatEntries(ih,FEW_ENTRIES))||
       (rc=DeleteFloatEntries(ih,nDelete))||
       (rc=ixm.CloseIndex(ih))||
       (rc=ixm.OpenIndex(FILENAME,index,ih))||
       ///检验看看这些value是否已经被删
       (rc=VerifyFloatIndex(ih,0,nDelete,FALSE))||
       (rc=VerifyFloatIndex(ih,nDelete,FEW_ENTRIES-nDelete,TRUE))||
       (rc=ixm.CloseIndex(ih))
       )
       return rc;
    LsFiles(FILENAME);
    if(rc=ixm.DestroyIndex(FILENAME,index))return rc;
    printf("完成测试5\n\n");
    return 0;
}



RC Test6(){
    RC rc;
    IX_IndexHandle ih;
    int index=0;
    int i;
    char value[STRLEN];
    memset(value,' ',STRLEN);
    sprintf(value,"number %d",FEW_ENTRIES/2);

    RID rid;

    printf("测试6: 对字符串非等值扫描\n");

    if((rc=ixm.CreateIndex(FILENAME,index,STRING,STRLEN))||
       (rc=ixm.OpenIndex(FILENAME,index,ih))||
       (rc=InsertIntEntries(ih,FEW_ENTRIES))
       ) return rc;

    /// < 关系 扫描
    IX_IndexScan scanlt;
    if(rc=scanlt.OpenScan(ih,LT_OP,&value)){
        printf("无法打开扫描\n");
        return rc;
    }

    i=0;
    while(!(rc=scanlt.GetNextEntry(rid)))
        i++;
    if(rc!=IX_EOF)return rc;

    printf("此次扫描 < 找到%d个记录 \n",i);

    IX_IndexScan scanle;
    if(rc=scanle.OpenScan(ih,LE_OP,&value)){
        printf("无法打开扫描\n");
        return rc;
    }
    i=0;
    while(!(rc=scanle.GetNextEntry(rid)))i++;
    if(rc!=IX_EOF)return rc;

    printf("此次扫描 <= 找到%d个记录 \n",i);


    IX_IndexScan scangt;
    if(rc=scangt.OpenScan(ih,GT_OP,&value)){
        printf("无法打开扫描\n");
        return rc;
    }
    i=0;
    while(!(rc=scangt.GetNextEntry(rid)))i++;
    if(rc!=IX_EOF)return rc;

    printf("此次扫描 > 找到%d个记录 \n",i);


    IX_IndexScan scange;
    if(rc=scange.OpenScan(ih,GE_OP,&value)){
        printf("无法打开扫描\n");
        return rc;
    }
    i=0;
    while(!(rc=scange.GetNextEntry(rid)))i++;
    if(rc!=IX_EOF)return rc;

    printf("此次扫描 >= 找到%d个记录 \n",i);

    if(rc=ixm.CloseIndex(ih))return rc;
    LsFiles(FILENAME);
    if(rc=ixm.DestroyIndex(FILENAME,index))return rc;

    printf("通过测试6\n\n");

    return 0;
}








