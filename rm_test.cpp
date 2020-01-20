//#include<cstdio>
//#include<iostream>
//#include<cstring>
//#include<unistd.h>
//#include<cstdlib>
//#include"mybase.h"
//#include"pf.h"
//#include"rm.h"
//
//using namespace std;
//
//const char* FILENAME = "testrel";
//#define STRLEN 29
//#define PROG_UNIT 50
//
//#define FEW_RECS 20 ///要加入的记录数量
//
//
/////这里用来计算一个结构体的成员变量在其中的偏移量
//#ifndef offsetof
//
//#define offsetof(type,field) ((size_t) &( ((type*)0)->field )  )
//
//#endif // offsetof
//
/////测试用的记录类型
//struct TestRec {
//    char str[STRLEN];
//    int num;
//    float r;
//};
//
/////两个manager
//PF_Manager pfm;
/////rm manager要抓一个pfm
/////但是rm抓的这一个pfm并不需要是已经打开一个文件的,他只是需要随时抓着
/////他可以打开任意一个文件
//RM_Manager rmm(pfm);
//
//RC Test1(void);
//RC Test2(void);
//void PrintErrorAll(RC rc);
//void LsFile(char *fileName);
//void PrintRecord(TestRec &recBuf);
//
//RC AddRecs(RM_FileHandle &fh,int numRecs);
//RC VerifyFile(RM_FileHandle &fh,int numRecs);
//RC PrintFile(RM_FileScan &fs);
//
//RC CreateFile(const char* filename,int recordSize);
//RC DestroyFile(const char* filename);
//RC OpenFile(const char* filename,RM_FileHandle &fh);
//RC CloseFile(const char* filename,RM_FileHandle &fh);
//RC InsertRec(RM_FileHandle& fh,char* record,RID &rid);
//RC UpdateRec(RM_FileHandle& fh,RM_Record &rec);
//RC DeleteRec(RM_FileHandle& fh,RID& rid);
//
//RC GetNextRecScan(RM_FileScan &fs,RM_Record &rec);
//
//
/////函数指针的数组
//#define NUM_TESTS 2
//int (*tests[]) () = {Test1,Test2};
//
//
//int main() {
//    RC rc;
//
//    cerr.flush();
//    cout.flush();
//    cout<<"开始RM层测试"<<endl;
//    cout.flush();
//    unlink(FILENAME);
//    for(int i=0; i<NUM_TESTS; i++) {
//        if(rc=tests[i]()) {
//            printf("这里调用PrintErrorAll rc=%d\n",rc);
////            PrintErrorAll(rc);
//            return 1;
//        }
//    }
//    cout<<"完成RM层测试"<<endl;
//    return 0;
//}
//
//
//
//
//
/////打印文件在系统中的属性
//void LsFile(char *fileName) {
//    char command[80];
//    sprintf(command,"ls -l %s",fileName);
//    printf("正在执行命令: \"%s\" \n",command);
//    system(command);
//}
//
/////打印一条记录
//void PrintRecord(TestRec &recBuf) {
//    printf("[%s,%d,%f]\n",recBuf.str,recBuf.num,recBuf.r);
//}
//
/////增加记录
//RC AddRecs(RM_FileHandle &fh,int numRecs) {
//    RC rc;
//    TestRec recBuf;
//    RID rid;
//    PageNum pageNum;
//    SlotNum slotNum;
//
//    memset(&recBuf,0,sizeof(recBuf));
//
//    printf("\n加入 %d 条记录\n",numRecs);
//    int i;
//    for(i=0; i<numRecs; i++) {
//        memset(recBuf.str,' ',STRLEN);
//        sprintf(recBuf.str,"a%d",i);
//        recBuf.num=i;
//        recBuf.r=(float)i;
//        ///rid由这个insert函数自行安排
//        if((rc=InsertRec(fh,(char*)&recBuf,rid))||
//                (rc=rid.GetPageNum(pageNum))||
//                (rc=rid.GetSlotNum(slotNum))
//          ) return rc;
//        if((i+1)%PROG_UNIT==0) {
//            printf("%d ",i+1);
//            fflush(stdout);
//        }
//    }
//    if(i%PROG_UNIT!=0)printf("%d\n",i);
//    else putchar('\n');
//
//    return 0;
//}
//
/////确认增加了记录
//RC VerifyFile(RM_FileHandle &fh,int numRecs) {
//    RC rc;
//    int n;
//    TestRec *pRecBuf;
//    RID rid;
//    char stringBuf[STRLEN];
//    char *found;
//    RM_Record rec;
//
//    ///found就是个bool数组
//    found=new char[numRecs];
//    memset(found,0,numRecs);
//
//    printf("\n正在确认文件的记录\n");
//
//    RM_FileScan fs;
//    if(rc=fs.OpenScan(fh,INT,sizeof(int),offsetof(TestRec,num),NO_OP,NULL,NO_HINT))return rc;
//
//    for(rc=GetNextRecScan(fs,rec),n=0; rc==0; rc=GetNextRecScan(fs,rec),n++) {
//
//
/////涨姿势了，这里必须要(char*&)才行
//        if((rc=rec.GetData((char*&)pRecBuf))||
//                (rc=rec.GetRid(rid))
//          ) {
//            fs.CloseScan();
//            delete[] found;
//            return (rc);
//        }
//
//        memset(stringBuf,' ',STRLEN);
//        sprintf(stringBuf,"a%d",pRecBuf->num);
//
//        if(pRecBuf->num<0||
//                pRecBuf->num>=numRecs||
//                strcmp(pRecBuf->str,stringBuf)||
//                pRecBuf->r!=(float)(pRecBuf->num)
//          ) {
//            printf("确认文件出错： 有一条无效记录=[%s,%d,%f]\n",
//                   pRecBuf->str,
//                   pRecBuf->num,
//                   pRecBuf->r
//                  );
//            printf("这条记录的预期值应是:RID[%d,%d]=[%s,%d,%f]\n",
//                   rid.Page(),rid.Slot(),stringBuf,
//                   pRecBuf->num,
//                   pRecBuf->r
//                  );
//            exit(1);
//        }
//
//        if(found[pRecBuf->num]) {
//            printf("同一条记录找到了2次??? record=[%s,%d,%f]\n",
//                   pRecBuf->str, pRecBuf->num, pRecBuf->r
//                  );
//            exit(1);
//        }
//        printf("正常地找到记录RID[%d,%d]=[%s,%d,%f]\n",
//               rid.Page(), rid.Slot(), stringBuf, pRecBuf->num, pRecBuf->r
//              );
//        found[pRecBuf->num]=1;
//    }
//    if(rc!=RM_EOF) {
//        fs.CloseScan();
//        delete[] found;
//        return (rc);
//    }
//    if(rc=fs.CloseScan()) {
//        fs.CloseScan();
//        delete[] found;
//        return (rc);
//    }
//    if(n!=numRecs){
//        delete [] found;
//        printf("插入的记录没找全 应有%d条 实际找到%d条\n",numRecs,n);
//        exit(1);
//    }
//
//    rc=0;
//
//    fs.CloseScan();
//    delete[] found;
//    return (rc);
//
//}
//
//RC PrintFile(RM_FileScan &fs){
//    RC rc;
//    int n;
//    TestRec *pRecBuf;
//    RID rid;
//    RM_Record rec;
//
//    printf("\n打印文件内容\n");
//
//    for(rc=GetNextRecScan(fs,rec),n=0;rc==0;rc=GetNextRecScan(fs,rec),n++){
//        if((rc=rec.GetData((char*&)pRecBuf))||
//           (rc=rec.GetRid(rid))
//           ) return rc;
//        PrintRecord(*pRecBuf);
//    }
//    if(rc!=RM_EOF)return rc;
//    printf("找到%d条记录\n",n);
//    return 0;
//}
//
//RC CreateFile(const char* fileName,int recordSize){
//    printf("创建文件:%s\n",fileName);
//    return rmm.CreateFile(fileName,recordSize);
//}
//
//RC DestroyFile(const char* filename){
//    printf("删除文件:%s\n",filename);
//    return rmm.DestroyFile(filename);
//}
//
//RC OpenFile(const char* filename,RM_FileHandle &fh){
//    printf("打开文件:%s\n",filename);
//    return rmm.OpenFile(filename,fh);
//}
//
//RC CloseFile(const char* filename,RM_FileHandle &fh){
//    if(filename)printf("关闭文件:%s\n",filename);
//    return rmm.CloseFile(fh);
//}
//
//RC InsertRec(RM_FileHandle& fh,char* record,RID &rid){
//    return fh.InsertRec(record,rid);
//}
//
//RC DeleteRec(RM_FileHandle& fh,RID& rid){
//    return fh.DeleteRec(rid);
//}
//
//RC UpdateRec(RM_FileHandle& fh,RM_Record &rec){
//    return fh.UpdataRec(rec);
//}
//
//RC GetNextRecScan(RM_FileScan &fs,RM_Record &rec){
//    return fs.GetNextRec(rec);
//}
//
/////创建打开关闭删除 文件
//RC Test1(void){
//    RC rc;
//    RM_FileHandle fh;
//    printf("第一个测试开始 *************\n");
//
//    if((rc=CreateFile(FILENAME,sizeof(TestRec)))||
//       (rc=OpenFile(FILENAME,fh))||
//       (rc=CloseFile(FILENAME,fh))
//       )return rc;
//    LsFile((char*)FILENAME);
//    if(rc=DestroyFile(FILENAME))return rc;
//    printf("完成测试1 *****************\n");
//    return 0;
//}
//
/////测试给一个文件加入记录
//RC Test2(void){
//    RC rc;
//    RM_FileHandle fh;
//    printf("测试2开始了 ************\n");
//    if((rc=CreateFile(FILENAME,sizeof(TestRec)))||
//       (rc=OpenFile(FILENAME,fh))||
//       (rc=AddRecs(fh,FEW_RECS))||
//       (rc=VerifyFile(fh,FEW_RECS))||
//       (rc=CloseFile(FILENAME,fh))
//       ) return rc;
//    LsFile((char*)FILENAME);
//    if(rc=DestroyFile(FILENAME))return rc;
//    printf("完成测试2 ***************\n");
//    return 0;
//}
//
//
//
//
//
