#include"ix_manager.h"
#include"ix_indexhandle.h"
#include<sstream>

IX_Manager::IX_Manager(PF_Manager &pfm):pfm(pfm) {}


IX_Manager::~IX_Manager() {

}

RC IX_Manager::CreateIndex(const char* fileName, int indexNo, AttrType attrType,
                           int attrLength, int pageSize) {
    if( indexNo<0||
        attrType<INT||
        attrType>STRING||
        fileName==NULL
      ) return IX_FCREATEFAIL;
    if(attrLength>=pageSize-(int)sizeof(RID)||attrLength<=0)return IX_INVALIDSIZE;
    if((attrType == INT && (unsigned int)attrLength != sizeof(int)) ||
            (attrType == FLOAT && (unsigned int)attrLength != sizeof(float))||
            (attrType == STRING &&((unsigned int)attrLength <= 0 ||(unsigned int)attrLength > MAXSTRINGLEN)))
        return IX_FCREATEFAIL;
    stringstream newname;
    newname<<fileName<<"."<<indexNo;
    RC rc;
    if((rc=pfm.CreateFile(newname.str().c_str()))<0 ){
        PF_PrintError(rc);
        return IX_PF;
    }
    PF_FileHandle pfh;
    if((rc=pfm.OpenFile(newname.str().c_str(),pfh))<0 ){
        PF_PrintError(rc);
        return IX_PF;
    }
    PF_PageHandle headerPage;
    char* pData;
    if((rc=pfh.AllocatePage(headerPage))<0 ){
        PF_PrintError(rc);
        return IX_PF;
    }

    if((rc=headerPage.GetData(pData))<0 ){
        PF_PrintError(rc);
        return IX_PF;
    }

    IX_FileHdr hdr;
    hdr.numPages=1;
    hdr.pageSize=pageSize;
    hdr.pairSize=attrLength+sizeof(RID);
    hdr.order=-1;
    hdr.height=0;
    hdr.rootPage=-1;
    hdr.attrType=attrType;
    hdr.attrLength=attrLength;

    memcpy(pData,&hdr,sizeof(hdr));
    PageNum headerPageNum;
    headerPage.GetPageNum(headerPageNum);
    ///必然是0
    if((rc=pfh.MarkDirty(headerPageNum))<0){
        PF_PrintError(rc);
        return IX_PF;
    }

    if((rc=pfh.UnpinPage(headerPageNum))<0){
        PF_PrintError(rc);
        return IX_PF;
    }
    if((rc=pfm.CloseFile(pfh))<0){
        PF_PrintError(rc);
        return IX_PF;
    }
    return 0;
}


RC IX_Manager::DestroyIndex(const char* fileName, int indexNo) {
    if(indexNo<0||fileName==NULL)return IX_FCREATEFAIL;
    stringstream newname;
    newname<<fileName<<"."<<indexNo;
    RC rc;
    if((rc=pfm.DestroyFile(newname.str().c_str()))<0){
        PF_PrintError(rc);
        return IX_PF;
    }
    return 0;

}

RC IX_Manager::OpenIndex(const char* fileName, int indexNo, IX_IndexHandle& ixh) {
    if(indexNo<0||
       fileName==NULL
       )
        return IX_FCREATEFAIL;
    PF_FileHandle pfh;
    stringstream newname;
    newname<<fileName<<"."<<indexNo;

    RC rc;
    if((rc=pfm.OpenFile(newname.str().c_str(),pfh))<0){
        PF_PrintError(rc);
        return IX_PF;
    }
    PF_PageHandle ph;
    char* pData;
    if((rc=pfh.GetThisPage(0,ph))||
       (rc=ph.GetData(pData))
       )return rc;

    IX_FileHdr hdr;
    memcpy(&hdr,pData,sizeof(hdr));
    if((rc=ixh.Open(&pfh,hdr.pairSize,hdr.rootPage,hdr.pageSize))<0){
        IX_PrintError(rc);
        return rc;
    }
    if((rc=pfh.UnpinPage(0))<0){
        PF_PrintError(rc);
        return rc;
    }

    return 0;
}

RC IX_Manager::CloseIndex(IX_IndexHandle& ixh) {
    if(!ixh.bFileOpen||ixh.pfHandle==NULL)return IX_FNOTOPEN;
    ///把头给写了
    if(ixh.HdrChanged()){
        PF_PageHandle ph;
        RC rc;
        if((rc=ixh.pfHandle->GetThisPage(0,ph))<0){
            PF_PrintError(rc);return rc;
        }
        if((rc=ixh.SetFileHeader(ph))<0){
            PF_PrintError(rc);
            return rc;
        }
        if((rc=ixh.pfHandle->MarkDirty(0))<0){
            PF_PrintError(rc);
            return rc;
        }
        if((ixh.pfHandle->UnpinPage(0))<0){
            PF_PrintError(rc);
            return rc;
        }
        if((ixh.ForcePages())<0){
            IX_PrintError(rc);
            return rc;
        }
    }

    RC rc2=pfm.CloseFile(*ixh.pfHandle);
    if(rc2<0){
        PF_PrintError(rc2);
        return rc2;
    }
    ixh.~IX_IndexHandle();
    ixh.pfHandle=NULL;
    ixh.bFileOpen=false;

    return 0;
}



