//#include <cstdio>
//#include <iostream>
//#include <cstring>
//#include <unistd.h>
//#include "pf.h"
//#include "pf_internal.h"
//#include "pf_hashtable.h"
//
//using namespace std;
//
//
//
/////这个statistics先抄过来，不管什么意思了
//#ifdef PF_STATS
//#include "statistics.h"
/// This is defined within pf_buffermgr.cc
//extern StatisticsMgr *pStatisticsMgr;
//// This method is defined within pf_statistics.cc.  It is called at the end
//// to display the final statistics, or by the debugger to monitor progress.
//extern void PF_Statistics();
////
//// PF_ConfirmStatistics
////
//// This function will be run at the end of the program after all the tests
//// to confirm that the buffer manager operated correctly.
////
//// These numbers have been confirmed.  Note that if you change any of the
//// tests, you will also need to change these numbers as well.
////
//void PF_ConfirmStatistics()
//{
//   // Must remember to delete the memory returned from StatisticsMgr::Get
//   cout << "Verifying the statistics for buffer manager: ";
//   int *piGP = pStatisticsMgr->Get("GetPage");
//   int *piPF = pStatisticsMgr->Get("PageFound");
//   int *piPNF = pStatisticsMgr->Get("PageNotFound");
//   int *piWP = pStatisticsMgr->Get("WritePage");
//   int *piRP = pStatisticsMgr->Get("ReadPage");
//   int *piFP = pStatisticsMgr->Get("FlushPage");
//
//   if (piGP && (*piGP != 702)) {
//      cout << "Number of GetPages is incorrect! (" << *piGP << ")\n";
//      // No built in error code for this
//      exit(1);
//   }
//   if (piPF && (*piPF != 23)) {
//      cout << "Number of pages found in the buffer is incorrect! (" <<
//        *piPF << ")\n";
//      // No built in error code for this
//      exit(1);
//   }
//   if (piPNF && (*piPNF != 679)) {
//      cout << "Number of pages not found in the buffer is incorrect! (" <<
//        *piPNF << ")\n";
//      // No built in error code for this
//      exit(1);
//   }
//   if (piRP && (*piRP != 679)) {
//      cout << "Number of read requests to the Unix file system is " <<
//         "incorrect! (" << *piPNF << ")\n";
//      // No built in error code for this
//      exit(1);
//   }
//   if (piWP && (*piWP != 339)) {
//      cout << "Number of write requests to the Unix file system is "<<
//         "incorrect! (" << *piPNF << ")\n";
//      // No built in error code for this
//      exit(1);
//   }
//   if (piFP && (*piFP != 16)) {
//      cout << "Number of requests to flush the buffer is "<<
//         "incorrect! (" << *piPNF << ")\n";
//      // No built in error code for this
//      exit(1);
//   }
//   cout << " Correct!\n";
//
//   // Delete the memory returned from StatisticsMgr::Get
//   delete piGP;
//   delete piPF;
//   delete piPNF;
//   delete piWP;
//   delete piRP;
//   delete piFP;
//}
//#endif    // PF_STATS
//
//
//
//
/////以下是测试部分
//
//#define FILE1 "file1"
//#define FILE2 "file2"
//
//RC WriteFile(PF_Manager &pfm,char* fname);
//RC PrintFile(PF_FileHandle &fh);
//RC ReadFile(PF_Manager& pfm,char* fname);
//RC TestPF();
//RC TestHash();
//
/////这个函数虽然叫writefile，但是其实是打开文件后读到缓冲区
/////然后分配多个分页，然后把分页号写进分页的数据区
/////然后在把文件关了,这个关的操作应该就会写回disk
//
//RC WriteFile(PF_Manager &pfm,char* fname){
//    PF_FileHandle fh;
//    PF_PageHandle ph;
//
//    RC rc;
//    char* pData;
//    PageNum pageNum;
//
//    cout<<"打开文件:"<<fname<<endl;
//    if(rc=pfm.OpenFile(fname,fh))return rc;
//
//
//
//
//    for(int i=0;i<PF_BUFFER_SIZE;i++){
//
//        if(rc=fh.AllocatePage(ph))return rc;
////        puts("到这里");
//        if(rc=ph.GetData(pData))return rc;
//        if(rc=ph.GetPageNum(pageNum))return rc;
//
//
////        if((rc=fh.AllocatePage(ph))||
////           (rc=ph.GetData(pData))||
////           (rc=ph.GetPageNum(pageNum))
////            )
////            ///测试在缓冲区中分配该文件的一块内存，取得内存地址，分页编号
////            return rc;
//
//        if(i!=pageNum){
//            cout<<"pageNum 不正确"<<"pageNum:"<<(int)pageNum<<" i:"<<i<<endl;
//            exit(1);
//        }
//        memcpy(pData,(char*)&pageNum,sizeof(PageNum));
//        cout<<"Page allocated:"<<(int)pageNum<<endl;
//    }
//
//
//
//    if((rc=fh.AllocatePage(ph))!=PF_NOBUF){
//        cout<<"固定过多的页应该返回失败 但是没有返回失败"<<endl;
//        return rc;
//    }
//
//    cout<<"解固定页并关闭文件"<<endl;
//    for(int i=0;i<PF_BUFFER_SIZE;i++){
//        if(rc=fh.UnpinPage(i))
//            return rc;
//    }
//
//    if(rc=pfm.CloseFile(fh))return rc;
//
//    return 0;
//}
//
//RC PrintFile(PF_FileHandle &fh){
//    PF_PageHandle ph;
//    RC rc;
//    char* pData;
//    PageNum pageNum,temp;
//    cout<<"读取文件"<<endl;
//    if(rc=fh.GetFirstPage(ph))return rc;
//    ///这个do while 被我改了
//    while(1){
//        if((rc=ph.GetData(pData))||
//           (rc=ph.GetPageNum(pageNum))
//        )return rc;
//        memcpy((char*)&temp,pData,sizeof(PageNum));
//        cout<<"取得分页"<<(int)pageNum<<" "<<(int)temp<<endl;
//        if(rc=fh.UnpinPage(pageNum))return rc;
//        if((rc=fh.GetNextPage(pageNum,ph)))break;
//    }
//    if(rc!=PF_EOF)return rc;
//    cout<<"到达EOF"<<endl;
//    return 0;
//}
//RC ReadFile(PF_Manager &pfm,char* fname){
//    PF_FileHandle fh;
//    RC rc;
//    cout<<"Opening:"<<fname<<endl;
//    if((rc=pfm.OpenFile(fname,fh))||
//       (rc=PrintFile(fh))||
//       (rc=pfm.CloseFile(fh))
//       )
//        return rc;
//    else return 0;
//}
//
//
//
//RC TestPF(){
//    PF_Manager pfm;
//    PF_FileHandle fh1,fh2;
//    PF_PageHandle ph;
//    RC rc;
//    char *pData;
//    PageNum pageNum,temp;
//    int i;
//
//    int len;
//    ///get到的就是page的大小
//    pfm.GetBlockSize(len);
//    printf("分页大小:%d\n",len);
//    cout<<"创建并打开两个文件"<<endl;
//
//
//
//    if((rc=pfm.CreateFile(FILE1))||
//       (rc=pfm.CreateFile(FILE2))||
//       (rc=WriteFile(pfm,(char*)FILE1))||
//       (rc=ReadFile(pfm,(char*)FILE1))||
//       (rc=WriteFile(pfm,(char*)FILE2))||
//       (rc=ReadFile(pfm,(char*)FILE2))||
//       (rc=pfm.OpenFile(FILE1,fh1))||
//       (rc=pfm.OpenFile(FILE2,fh2))
//       )
//        return rc;
//
//
//    cout << "Disposing of alternate pages\n";
//
//    for(int i=0;i<PF_BUFFER_SIZE;i++){
//        if(i&1){
//            if(rc=fh1.DisposePage(i)){
//                puts("gg1");
//                return rc;
//            }
//        }else{
//            if(rc=fh2.DisposePage(i)){
//                puts("gg2");
//                return rc;
//            }
//        }
//    }
//
//
//
//    cout<<"关闭并删除两个文件"<<endl;
//
//    if((rc=fh1.FlushPages())||
//       (rc=fh2.FlushPages())||
//       (rc=pfm.CloseFile(fh1))||
//       (rc=pfm.CloseFile(fh2))||
//       (rc=ReadFile(pfm,(char*)FILE1))||
//       (rc=ReadFile(pfm,(char*)FILE2))||
//       (rc=pfm.DestroyFile(FILE1))||
//       (rc=pfm.DestroyFile(FILE2))
//       )return rc;
//
//
//    cout<<"再次创建、打开文件"<<endl;
//    if((rc=pfm.CreateFile(FILE1))||
//       (rc=pfm.CreateFile(FILE2))||
//       (rc=WriteFile(pfm,(char*)FILE1))||
//       (rc=WriteFile(pfm,(char*)FILE2))||
//       (rc=pfm.OpenFile(FILE1,fh1))||
//       (rc=pfm.OpenFile(FILE2,fh2))
//       )
//        return rc;
//
//    cout<<"给每个文件分配额外的分页"<<endl;
//    for(int i=PF_BUFFER_SIZE;i<PF_BUFFER_SIZE*2;i++){
//
//        if((rc=fh2.AllocatePage(ph))||
//           (rc=ph.GetData(pData))||
//           (rc=ph.GetPageNum(pageNum))
//           )return rc;
//
//        if(i!=pageNum){
//            cout<<"pageNum 不正确"<<"pageNum:"<<(int)pageNum<<" i:"<<i<<endl;
//            exit(1);
//        }
//
//        memcpy(pData,(char*)&pageNum,sizeof(PageNum));
//
//        if((rc=fh2.MarkDirty(pageNum))||
//           (rc=fh2.UnpinPage(pageNum))
//           )return rc;
//
//        if((rc=fh1.AllocatePage(ph))||
//           (rc=ph.GetData(pData))||
//           (rc=ph.GetPageNum(pageNum))
//           )return rc;
//
//        if(i!=pageNum){
//            cout<<"pageNum 不正确"<<"pageNum:"<<(int)pageNum<<" i:"<<i<<endl;
//            exit(1);
//        }
//
//        memcpy(pData,(char*)&pageNum,sizeof(PageNum));
//
//        if((rc=fh1.MarkDirty(pageNum))||
//           (rc=fh1.UnpinPage(pageNum))
//           )return rc;
//    }
//
//
//    ///丢弃额外的分页
//    cout<<"丢弃额外的分页"<<endl;
//
//    for(int i=PF_BUFFER_SIZE;i<PF_BUFFER_SIZE;i++){
//        if(i&1){
//            if(rc=fh1.DisposePage(i))return rc;
//        }else{
//            if(rc=fh2.DisposePage(i))return rc;
//        }
//    }
//
//    cout<<"取得file2的保留下来的额外分页"<<endl;
//
//    for(int i=PF_BUFFER_SIZE;i<PF_BUFFER_SIZE;i++){
//        if(i&1){
//            if((rc=fh2.GetThisPage(i,ph))||
//               (rc=ph.GetData(pData))||
//               (rc=ph.GetPageNum(pageNum))
//               )return rc;
//            memcpy((char*)&temp,pData,sizeof(PageNum));
//            cout<<"Page:"<<(int)pageNum<<" "<<(int)temp<<endl;
//            if(rc=fh2.UnpinPage(i))return rc;
//        }
//    }
//
//    cout<<"取得file1的保留下来的额外分页"<<endl;
//    for(int i=PF_BUFFER_SIZE;i<PF_BUFFER_SIZE;i++){
//        if(!(i&1)){
//            if((rc=fh1.GetThisPage(i,ph))||
//               (rc=ph.GetData(pData))||
//               (rc=ph.GetPageNum(pageNum))
//               )return rc;
//            memcpy((char*)&temp,pData,sizeof(PageNum));
//            cout<<"Page:"<<(int)pageNum<<" "<<(int)temp<<endl;
//            if(rc=fh1.UnpinPage(i))return rc;
//        }
//    }
//
//    cout<<"打印文件2 和 文件1"<<endl;
//    if((rc=PrintFile(fh2))||
//       (rc=PrintFile(fh1))
//       )return rc;
//
//
//    ///这里估计是用freelist的来重新分配
//    cout<<"Putting stuff into the holes of file 1"<<endl;
//    for(int i=0;i<PF_BUFFER_SIZE/2;i++){
//        if((rc=fh1.AllocatePage(ph))||
//           (rc=ph.GetData(pData))||
//           (rc=ph.GetPageNum(pageNum))
//           )return rc;
//        memcpy(pData,(char*)&pageNum,sizeof(PageNum));
//        if((rc=fh1.MarkDirty(pageNum))||
//           (rc=fh1.UnpinPage(pageNum))
//           )return rc;
//    }
//
//    cout<<"打印文件1然后关闭两个文件"<<endl;
//
//    if((rc=PrintFile(fh1))||
//       (rc=pfm.CloseFile(fh1))||
//       (rc=pfm.CloseFile(fh2))
//       )return rc;
//
//    cout<<"重新打开文件1并测试一些错误情况"<<endl;
//
//    if(rc=pfm.OpenFile(FILE1,fh1))return rc;
//
//    if((rc=fh1.DisposePage(100))!=PF_INVALIDPAGE){
//        cout<<"丢弃分页应该失败，但是没有"<<endl;
//        return rc;
//    }
//
//    if(rc=fh1.GetThisPage(1,ph))return rc;
//    if((rc=fh1.DisposePage(1))!=PF_PAGEPINNED){
//        cout<<"丢弃分页应该失败，但是没有"<<endl;
//        return rc;
//    }
//
//    if((rc=ph.GetData(pData))||
//       (rc=ph.GetPageNum(pageNum))
//       )return rc;
//
//    memcpy((char*)&temp,pData,sizeof(PageNum));
//
//    if(temp!=1||pageNum!=1){
//        cout<< "获取分页1的页码,得到的却是: " << (int)pageNum << " "<< (int)temp<<endl;
//        exit(1);
//    }
//
//    if(rc=fh1.UnpinPage(pageNum))return rc;
//
//    if(rc=fh1.UnpinPage(pageNum)!=PF_PAGEUNPINNED){
//        cout<<"解固定分页本应该fail，但是没有"<<endl;
//        return rc;
//    }
//
//    cout<<"打开文件1两次 打印出两份拷贝"<<endl;
//
//    if(rc=pfm.OpenFile(FILE1,fh2))return rc;
//
//    if((rc=PrintFile(fh1))||
//       (rc=PrintFile(fh2))
//       )return rc;
//
//    cout<<"关闭并删除两个文件"<<endl;
//    if((rc=pfm.CloseFile(fh1))||
//       (rc=pfm.CloseFile(fh2))||
//       (rc=pfm.DestroyFile(FILE1))||
//       (rc=pfm.DestroyFile(FILE2))
//       ) return rc;
//
//#ifdef PF_STATS
//   PF_Statistics();
//   PF_ConfirmStatistics();
//#endif
//
//    return 0;
//}
//
//
//RC TestHash(){
//    PF_HashTable hst(PF_HASH_TBL_SIZE);
//    RC rc;
//    int s;
//
//    cout<<"开始测试hashtable插入"<<endl;
//    ///hashtable映射到的值可以相同，但是不能相同
//    for(int i=1;i<=10;i++){
//        for(int p=1;p<=10;p++){
//            if(rc=hst.Insert(i,p,i+p))return rc;
//        }
//    }
//
//    cout<<"开始测试查找"<<endl;
//    for(int i=1;i<=10;i++){
//        for(int p=1;p<=10;p++)if(rc=hst.Find(i,p,s))return rc;
//    }
//
//    cout<<"测删除"<<endl;
//    for(int i=10;i;i--){
//        for(int p=10;p;p--)if(rc=hst.Delete(i,p))return rc;
//    }
//
//    cout<<"确保被删完"<<endl;
//    for(int i=1;i<=10;i++)for(int p=1;p<=10;p++){
//        if(rc=hst.Find(i,p,s)!=PF_HASHNOTFOUND){
//            cout<<"hashtable没删干净"<<endl;
//            return rc;
//        }
//    }
//    return 0;
//}
//
//
//int main(){
//    RC rc;
//
//    cerr.flush();
//    cout.flush();
//    cout<<"开始测试PF层"<<endl;
//    cout.flush();
//
//#ifdef PF_STATS
//    cout << "Note: Statistics are turned on.\n";
//#endif
//
//    ///先把文件删了
//    unlink(FILE1);
//    unlink(FILE2);
//
//
////    if(rc=TestPF()){
////        PF_PrintError(rc);
////        return 1;
////    }
////    if(rc=TestHash()){
////        PF_PrintError(rc);
////        return 1;
////    }
//
//    if((rc=TestPF())||
//       (rc=TestHash())
//    ){
//        PF_PrintError(rc);
//        return 1;
//    }
//    cout<<"完成PF层测试"<<endl;
//    return 0;
//}
//
//
//
//
//
//
//
