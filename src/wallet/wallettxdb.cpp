// Copyright (c) 2014-2017 Coin Sciences Ltd
// FloatChain code distributed under the GPLv3 license, see COPYING file.

#include "floatchain/floatchain.h"
#include "wallet/wallettxdb.h"

#define FC_TDB_MAX_TXS_FILE_SIZE             0x8000000                          // Maximal data file size, 128MB

void sprintf_hex(char *hex,const unsigned char *bin,int size)
{
    int i;
    for(i=0;i<size;i++)
    {
        sprintf(hex+(i*2),"%02x",bin[size-1-i]);
    }
    
    hex[64]=0;      
}


void FC_TxEntity::Zero()
{
    memset(this,0,sizeof(FC_TxEntity));
}

void FC_TxEntity::Init(unsigned char *entity_id,uint32_t entity_type)
{
    meFCpy(m_EntityID,entity_id,FC_TDB_ENTITY_ID_SIZE);
    m_EntityType=entity_type;
}

void FC_TxEntityRow::Zero()
{
    memset(this,0,sizeof(FC_TxEntityRow));
}

/* Swaps bytes in the structure's m_Pos field. 
 * This is needed to make this field big-endian in the database.
 * And this is needed to make subsequent keys in LevelDB  */

void FC_TxEntityRow::SwapPosBytes()
{
    unsigned char *ptr=(unsigned char *)&m_Pos;
    unsigned char t;
    t=ptr[0];
    ptr[0]=ptr[3];
    ptr[3]=t;
    t=ptr[1];
    ptr[1]=ptr[2];
    ptr[2]=t;
}


void FC_TxEntityStat::Zero()
{
    memset(this,0,sizeof(FC_TxEntityStat));
}

void FC_TxImportRow::Zero()
{
    memset(this,0,sizeof(FC_TxImportRow));
    m_RowType=FC_TET_IMPORT;
}

void FC_TxImportRow::SwapPosBytes()
{
    unsigned char *ptr=(unsigned char *)&m_Pos;
    unsigned char t;
    t=ptr[0];
    ptr[0]=ptr[3];
    ptr[3]=t;
    t=ptr[1];
    ptr[1]=ptr[2];
    ptr[2]=t;
}
void FC_TxEntityDBStat::Zero()
{
    memset(this,0,sizeof(FC_TxEntityDBStat));
    m_Block=-1;
    m_RowType=FC_TET_DB_STAT;
    m_WalletVersion=FC_TDB_WALLET_VERSION;
}

void FC_TxImport::Zero()
{
    memset(this,0,sizeof(FC_TxImport));
}

int FC_TxImport::Init(int generation,int block)
{
    m_ImportID=generation;
    m_Block=block;
    m_Entities=new FC_Buffer;
    m_TmpEntities=new FC_Buffer;
    
    m_TmpEntities->Initialize(sizeof(FC_TxEntity),sizeof(FC_TxEntity),FC_BUF_MODE_MAP);    
    return m_Entities->Initialize(sizeof(FC_TxEntity),sizeof(FC_TxEntityStat),FC_BUF_MODE_MAP);    
}

int FC_TxImport::AddEntity(FC_TxEntity *entity)
{
    FC_TxEntityStat stat;
    if(m_Entities == NULL)
    {
        return FC_ERR_INTERNAL_ERROR;
    }
    if(m_Entities->Seek((unsigned char*)entity) >= 0)
    {
        return FC_ERR_FOUND;
    }
    
    stat.Zero();
    meFCpy(&stat.m_Entity,entity,sizeof(FC_TxEntity));
    stat.m_Generation=m_ImportID;
    stat.m_PosInImport=1;
    if(m_Entities->GetCount())
    {
        stat.m_PosInImport=((FC_TxEntityStat*)m_Entities->GetRow(m_Entities->GetCount()-1))->m_PosInImport+1;
    }
    return m_Entities->Add(&stat,(char*)&stat+sizeof(FC_TxEntity));
}

int FC_TxImport::AddEntity(FC_TxEntityStat *entity)
{
    if(m_Entities == NULL)
    {
        return FC_ERR_INTERNAL_ERROR;
    }
    if(m_Entities->Seek((unsigned char*)entity) >= 0)
    {
        return FC_ERR_FOUND;
    }
    return m_Entities->Add(entity,(char*)entity+sizeof(FC_TxEntity));
}

int FC_TxImport::FindEntity(FC_TxEntityStat *entity)
{
    return m_Entities->Seek((unsigned char*)entity);
}

int FC_TxImport::FindEntity(FC_TxEntity *entity)
{
    return m_Entities->Seek((unsigned char*)entity);
}

FC_TxEntityStat *FC_TxImport::GetEntity(int row)
{
    if((row < 0) || (row >= m_Entities->GetCount()))
    {
        return NULL;
    }
    return (FC_TxEntityStat *)m_Entities->GetRow(row);
}


void FC_TxImport::Destroy()
{
    if(m_Entities)
    {
        delete m_Entities;
    }
    if(m_TmpEntities)
    {
        delete m_TmpEntities;
    }
    
    Zero();
}


void FC_TxDefRow::Zero()
{
    memset(this,0,sizeof(FC_TxDefRow));
}

void FC_TxEntityDB::Zero()
{
    m_FileName[0]=0;
    m_DB=NULL;
    m_KeyOffset=0;
    m_KeySize=32;
    m_ValueOffset=32;
    m_ValueSize=48;    
    m_TotalSize=m_KeySize+m_ValueSize;
}

void FC_TxEntityDB::SetName(const char* name)
{
    FC_GetFullFileName(name,"wallet/txs",".db",FC_FOM_RELATIVE_TO_DATADIR | FC_FOM_CREATE_DIR,m_FileName);
}

int FC_TxEntityDB::Open() 
{
    
    m_DB=new FC_Database;
    
    m_DB->SetOption("KeySize",0,m_KeySize);
    m_DB->SetOption("ValueSize",0,m_ValueSize);
        
    return m_DB->Open(m_FileName,FC_OPT_DB_DATABASE_CREATE_IF_MISSING | FC_OPT_DB_DATABASE_TRANSACTIONAL | FC_OPT_DB_DATABASE_LEVELDB);
}

int FC_TxEntityDB::Close()
{
    if(m_DB)
    {
        m_DB->Close();
        delete m_DB;    
        m_DB=NULL;
    }
    return 0;
}

void FC_TxDB::Zero()
{
    int i;
    m_Database=NULL;
    m_DBStat.Zero();
    
    for(i=0;i<FC_TDB_MAX_IMPORTS;i++)
    {
        m_RawMemPools[i]=NULL;
        m_MemPools[i]=NULL;
        m_Imports[i].Zero();
    }
    m_RawUpdatePool=NULL;
    
    m_Name[0]=0x00; 
    m_LobFileNamePrefix[0]=0x00;
    m_LogFileName[0]=0x00;
    
    m_Mode=FC_WMD_NONE;
    m_Semaphore=NULL;
    m_LockedBy=0;    
}

void FC_TxDB::LogString(const char *message)
{
    FILE *fHan;
    
    fHan=fopen(m_LogFileName,"a");
    if(fHan == NULL)
    {
        return;
    }

    FC_LogString(fHan,message);  
    fclose(fHan);
}

int FC_TxDB::Lock(int write_mode,int allow_secondary)
{        
    uint64_t this_thread;
    this_thread=__US_ThreadID();
    
    if(this_thread == m_LockedBy)
    {
        if(allow_secondary == 0)
        {
            LogString("Secondary lock!!!");
        }
        return allow_secondary;
    }
    
    __US_SemWait(m_Semaphore); 
    m_LockedBy=this_thread;
    
    return 0;
}

void FC_TxDB::UnLock()
{    
    m_LockedBy=0;
    __US_SemPost(m_Semaphore);
}



int FC_TxDB::Initialize(const char *name,uint32_t mode)
{
    int err,value_len,i;   
    int import_count;
    int last_import;
    char msg[256];
    
    FC_TxEntityStat stat;
    FC_TxImportRow edbImport;
    FC_TxEntityRow edbRow;
    
    
    unsigned char *ptr;
    
    err=FC_ERR_NOERROR;
    
    strcpy(m_Name,name);
    
    m_Mode=mode;
    m_Database=new FC_TxEntityDB;
    
    FC_GetFullFileName(name,"wallet/txs","",FC_FOM_RELATIVE_TO_DATADIR | FC_FOM_CREATE_DIR,m_LobFileNamePrefix);
    FC_GetFullFileName(name,"wallet/txs",".log",FC_FOM_RELATIVE_TO_DATADIR,m_LogFileName);
    
    m_Database->SetName(name);
    
    err=m_Database->Open();
    
    if(err)
    {
        LogString("Initialize: Cannot open database");
        return err;
    }
    
    m_DBStat.Zero();
    
    ptr=(unsigned char*)m_Database->m_DB->Read((char*)&m_DBStat+m_Database->m_KeyOffset,m_Database->m_KeySize,&value_len,0,&err);
    if(err)
    {
        LogString("Initialize: Cannot read from database");
        return err;
    }

    
    
    
    if(ptr)                                                                     
    {        
        meFCpy((char*)&m_DBStat+m_Database->m_ValueOffset,ptr,m_Database->m_ValueSize);
        
        edbImport.Zero();
        edbImport.m_ImportID=0;
        edbImport.m_Pos=0;
        edbImport.SwapPosBytes();
        ptr=(unsigned char*)m_Database->m_DB->Read((char*)&edbImport+m_Database->m_KeyOffset,m_Database->m_KeySize,&value_len,FC_OPT_DB_DATABASE_SEEK_ON_READ,&err);
        edbImport.SwapPosBytes();
        if(err)
        {
            return err;
        }
        if(ptr == NULL)
        {
            return FC_ERR_NOERROR;
        }
    
        import_count=-1;
        last_import=-1;
        meFCpy((char*)&edbImport+m_Database->m_ValueOffset,ptr,m_Database->m_ValueSize);
    
        while(ptr)
        {
            if(edbImport.m_ImportID != last_import)
            {
                if(import_count+1<FC_TDB_MAX_IMPORTS)
                {
                    import_count++;
                    m_Imports[import_count].Init(edbImport.m_ImportID,edbImport.m_Block);
                    last_import=edbImport.m_ImportID;
                }
                else
                {
                    LogString("Initialize: too many open imports");
                    return FC_ERR_CORRUPTED;            
                }
            }
            
            if(edbImport.m_Pos)
            {
                stat.Zero();
                meFCpy(&(stat.m_Entity), &edbImport.m_Entity,sizeof(FC_TxEntity));
                stat.m_Generation=edbImport.m_Generation;
                stat.m_LastPos=edbImport.m_LastPos;
                stat.m_LastClearedPos=edbImport.m_LastPos;
                stat.m_PosInImport=edbImport.m_Pos;
                stat.m_LastImportedBlock=edbImport.m_LastImportedBlock;
                stat.m_TimeAdded=edbImport.m_TimeAdded;
                stat.m_Flags=edbImport.m_Flags;
                m_Imports[import_count].AddEntity(&stat);
            }
            
            ptr=(unsigned char*)m_Database->m_DB->MoveNext(&err);
            if(err)
            {
                LogString("Error on MoveNext");            
                return FC_ERR_CORRUPTED;            
            }
            if(ptr)
            {
                meFCpy((char*)&edbImport+m_Database->m_KeyOffset,ptr,m_Database->m_TotalSize);            
                edbImport.SwapPosBytes();
            }
            if(edbImport.m_RowType != FC_TET_IMPORT)
            {
                ptr=NULL;
            }
        }              
        if(last_import<0)
        {
            LogString("Initialize: Entity lists not found");
            return FC_ERR_CORRUPTED;            
        }
        if(m_Imports[0].m_Block != m_DBStat.m_Block)
        {
            LogString("Initialize: Block count mismatch");
            return FC_ERR_CORRUPTED;            
        }
    }
    else
    {
        edbRow.Zero();
        err=m_Database->m_DB->Write((char*)&edbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,(char*)&edbRow+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
        if(err)
        {
            return err;
        }        
        
        m_DBStat.Zero();
        if(m_Mode == FC_WMD_AUTO)
        {
            m_Mode=FC_WMD_TXS | FC_WMD_ADDRESS_TXS;
        }
        m_DBStat.m_InitMode=m_Mode & FC_WMD_MODE_MASK;
        err=m_Database->m_DB->Write((char*)&m_DBStat+m_Database->m_KeyOffset,m_Database->m_KeySize,(char*)&m_DBStat+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
        if(err)
        {
            return err;
        }        
        
        m_Imports[0].Init(0,-1);                                                // Chain import
    
        edbImport.Zero();
        edbImport.m_ImportID=0;
        edbImport.m_Block=m_Imports[0].m_Block;
        edbImport.SwapPosBytes();
        err=m_Database->m_DB->Write((char*)&edbImport+m_Database->m_KeyOffset,m_Database->m_KeySize,(char*)&edbImport+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
        edbImport.SwapPosBytes();
        if(err)
        {
            return err;
        }                    
        
        err=m_Database->m_DB->Commit(FC_OPT_DB_DATABASE_TRANSACTIONAL);
        if(err)
        {
            return err;
        }
    }
    
    m_MemPools[0]=new FC_Buffer;                                                // Key - entity with m_Pos set to 0 + txid
    err=m_MemPools[0]->Initialize(FC_TDB_ENTITY_KEY_SIZE+FC_TDB_TXID_SIZE,m_Database->m_TotalSize,FC_BUF_MODE_MAP);
    
    m_RawMemPools[0]=new FC_Buffer;    
    err=m_RawMemPools[0]->Initialize(FC_TDB_TXID_SIZE,m_Database->m_TotalSize,FC_BUF_MODE_MAP);

    m_RawUpdatePool=new FC_Buffer;    
    err=m_RawUpdatePool->Initialize(FC_TDB_TXID_SIZE,m_Database->m_TotalSize,FC_BUF_MODE_MAP);        
    
    Dump("Initialize");
    sprintf(msg, "Initialized. Chain height: %d, Txs: %d",m_DBStat.m_Block,m_DBStat.m_Count);
    LogString(msg);
    
    for(i=1;i<FC_TDB_MAX_IMPORTS;i++)                                           // All open imports are dropped in this version, with async imports, they should be continued
    {
        if((m_Imports+i)->m_Entities)
        {
            sprintf(msg, "Initialization, Dropping import %d",(m_Imports+i)->m_ImportID);
            LogString(msg);
            DropImport(m_Imports+i);
        }
    }
   
    
    return err;
}

int FC_TxDB::Destroy()
{
    int i;

    if(m_Database)
    {
        delete m_Database;
    }

    for(i=0;i<FC_TDB_MAX_IMPORTS;i++)
    {
        m_Imports[i].Destroy();
        if(m_MemPools[i])
        {
            delete m_MemPools[i];
        }
        if(m_RawMemPools[i])
        {
            delete m_RawMemPools[i];
        }
    }
    
    if(m_RawUpdatePool)
    {
        delete m_RawUpdatePool;
    }
    
    if(m_Semaphore)
    {
        __US_SemDestroy(m_Semaphore);
    }
     
    Zero();
    return FC_ERR_NOERROR;    
}

void FC_TxDB::Dump(const char *message)
{    
    if((m_Mode & FC_WMD_DEBUG) == 0)
    {
        return;
    }
    FC_TxEntityRow edbRow;
    
    unsigned char *ptr;
    int dbvalue_len,err,i;
    char ShortName[65];                                     
    char FileName[FC_DCT_DB_MAX_PATH];                      
    FILE *fHan;
    
    sprintf(ShortName,"wallet/txs");
    FC_GetFullFileName(m_Name,ShortName,".dmp",FC_FOM_RELATIVE_TO_DATADIR | FC_FOM_CREATE_DIR,FileName);
    
    fHan=fopen(FileName,"a");
    if(fHan == NULL)
    {
        return;
    }

    FC_LogString(fHan,message);  
    
    fprintf(fHan,"Entities\n");
    edbRow.Zero();    
    ptr=(unsigned char*)m_Database->m_DB->Read((char*)&edbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,&dbvalue_len,FC_OPT_DB_DATABASE_SEEK_ON_READ,&err);
    if(err)
    {
        return;
    }

    if(ptr)
    {
        meFCpy((char*)&edbRow+m_Database->m_ValueOffset,ptr,m_Database->m_ValueSize);
        while(ptr)
        {
            FC_MemoryDumpCharSizeToFile(fHan,(char*)&edbRow+m_Database->m_KeyOffset,0,m_Database->m_TotalSize,m_Database->m_TotalSize);        
            ptr=(unsigned char*)m_Database->m_DB->MoveNext(&err);
            if(ptr)
            {
                meFCpy((char*)&edbRow+m_Database->m_KeyOffset,ptr,m_Database->m_TotalSize);            
            }
        }
    }
    
    for(i=0;i<FC_TDB_MAX_IMPORTS;i++)
    {
        if(m_RawMemPools[i])
        {
            if(m_RawMemPools[i]->GetCount())
            {
                fprintf(fHan,"RawMemPool %d\n",m_Imports[i].m_ImportID);
                FC_MemoryDumpCharSizeToFile(fHan,m_RawMemPools[i]->GetRow(0),0,m_RawMemPools[i]->GetCount()*m_Database->m_TotalSize,m_Database->m_TotalSize);    
            }
        }
        if(m_MemPools[i])
        {
            if(m_MemPools[i]->GetCount())
            {
                fprintf(fHan,"MemPool %d\n",m_Imports[i].m_ImportID);
                FC_MemoryDumpCharSizeToFile(fHan,m_MemPools[i]->GetRow(0),0,m_MemPools[i]->GetCount()*m_Database->m_TotalSize,m_Database->m_TotalSize);    
            }
        }
    }
    if(m_RawUpdatePool)
    {
        if(m_RawUpdatePool->GetCount())
        {
            fprintf(fHan,"RawUpdatePool\n");
            FC_MemoryDumpCharSizeToFile(fHan,m_RawUpdatePool->GetRow(0),0,m_RawUpdatePool->GetCount()*m_Database->m_TotalSize,m_Database->m_TotalSize);    
        }
    }
    
    fprintf(fHan,"\n<<<<<< \tChain height: %6d\t%s\n\n",m_DBStat.m_Block,message);
    fclose(fHan);
}

int FC_TxDB::FlushDataFile(uint32_t fileid)
{
    char FileName[FC_DCT_DB_MAX_PATH];         
    int FileHan;
    sprintf(FileName,"%s%05u.dat",m_LobFileNamePrefix,fileid);
    
    FileHan=open(FileName,_O_BINARY | O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    
    if(FileHan<=0)
    {
        return FC_ERR_INTERNAL_ERROR;
    }
    __US_FlushFile(FileHan);
    close(FileHan);
    return FC_ERR_NOERROR;
}


int FC_TxDB::AddToFile(const unsigned char *tx,
                          uint32_t txsize,
                          uint32_t fileid,
                          uint32_t offset)
{
    char FileName[FC_DCT_DB_MAX_PATH];         
    int FileHan,err;
    sprintf(FileName,"%s%05u.dat",m_LobFileNamePrefix,fileid);
    
    err=FC_ERR_NOERROR;
    
    FileHan=open(FileName,_O_BINARY | O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    
    if(FileHan<=0)
    {
        return FC_ERR_INTERNAL_ERROR;
    }
    
    if(lseek64(FileHan,offset,SEEK_SET) != offset)
    {
        err=FC_ERR_INTERNAL_ERROR;
        goto exitlbl;
    }
    
    if(write(FileHan,tx,txsize) != txsize)
    {
        err=FC_ERR_INTERNAL_ERROR;
        return FC_ERR_INTERNAL_ERROR;
    }
    
exitlbl:

//    fsync(FileHan);
    close(FileHan);
    return err;
}

/*
 * Adds entity to chain import
 */

int FC_TxDB::AddEntity(FC_TxEntity *entity,uint32_t flags)
{
    return AddEntity(m_Imports,entity,flags);
}

int FC_TxDB::AddEntity(FC_TxImport *import,FC_TxEntity *entity,uint32_t flags)
{
    int err;
    char msg[256];
    char enthex[65];
    FC_TxEntityStat stat;
    
    
    FC_TxImportRow edbImport;
    
    err=FC_ERR_NOERROR;
    
    if(import->FindEntity(entity) < 0)
    {
        stat.Zero();
        meFCpy(&(stat.m_Entity), entity,sizeof(FC_TxEntity));
        stat.m_Generation=0;
        stat.m_LastPos=0;
        stat.m_LastClearedPos=0;
        stat.m_PosInImport=1;
        if(import->m_Entities->GetCount())
        {
            stat.m_PosInImport=((FC_TxEntityStat*)import->m_Entities->GetRow(import->m_Entities->GetCount()-1))->m_PosInImport+1;
        }
        stat.m_LastImportedBlock=m_DBStat.m_Block;                              // Block the entity was added on, relevant if out-of-sync
        stat.m_TimeAdded=FC_TimeNowAsUInt();
        stat.m_Flags=flags;                                                     // Mainly for out-of-sync flag
        
        edbImport.Zero();
        meFCpy(&(edbImport.m_Entity), entity,sizeof(FC_TxEntity));
        edbImport.m_Generation=0;
        edbImport.m_LastPos=0;
        edbImport.m_Pos=stat.m_PosInImport;
        edbImport.m_LastImportedBlock=stat.m_LastImportedBlock;
        edbImport.m_TimeAdded=stat.m_TimeAdded;
        edbImport.m_Flags=stat.m_Flags;
        
        edbImport.m_Block=m_DBStat.m_Block;
        edbImport.SwapPosBytes();
        err=m_Database->m_DB->Write((char*)&edbImport+m_Database->m_KeyOffset,m_Database->m_KeySize,(char*)&edbImport+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
        edbImport.SwapPosBytes();
        if(err)
        {
            goto exitlbl;
        }                    

        err=m_Database->m_DB->Commit(FC_OPT_DB_DATABASE_TRANSACTIONAL);
        if(err)
        {
            goto exitlbl;
        }
        
        import->AddEntity(&stat);
    }
    else
    {
        err=FC_ERR_FOUND;
    }
    

    
exitlbl:

    sprintf_hex(enthex,entity->m_EntityID,FC_TDB_ENTITY_ID_SIZE);

    if(err)
    {
        if(err == FC_ERR_FOUND)
        {
            sprintf(msg,"Entity (%08X, %s) already in the wallet",entity->m_EntityType,enthex);        
            LogString(msg);
        }
        else
        {
            sprintf(msg,"Could not add entity (%08X, %s), error: %d",entity->m_EntityType,enthex,err);        
            LogString(msg);            
        }
    }
    else
    {
        sprintf(msg,"Entity (%08X, %s) added successfully",entity->m_EntityType,enthex);
        LogString(msg);
    }

    return err;
}

int FC_TxDB::FindEntity(FC_TxImport *import,
                        FC_TxEntity *entity)
{
    FC_TxImport *imp;
    imp=m_Imports;
    if(import)                                                                  // Find import
    {
        imp=import;
    }
    
    return imp->FindEntity(entity);
}

int FC_TxDB::FindEntity(FC_TxImport *import,FC_TxEntityStat *entity)
{
    FC_TxImport *imp;
    int row;
    imp=m_Imports;
    if(import)                                                                  // Find import
    {
        imp=import;
    }
    
    row=imp->FindEntity(entity);
    if(row >= 0)
    {
        meFCpy(entity,imp->GetEntity(row),sizeof(FC_TxEntityStat));
    }
    return row;
}

int FC_TxDB::AddSubKeyDef(
                FC_TxImport *import,                                              
                const unsigned char *hash,                                       
                const unsigned char *subkey,                                     
                uint32_t subkeysize,
                uint32_t flags)                                             
{
    FC_TxDefRow txdef;
    FC_Buffer *rawmempool;
    int err,size;
    uint32_t LastFileID,LastFileSize; 
    char msg[256];
    
    rawmempool=m_RawMemPools[0];
    if(import)                                                                  // Find import
    {
        rawmempool=m_RawMemPools[import-m_Imports];
    }

    err=GetTx(&txdef,hash);
    
    if(err == FC_ERR_NOT_FOUND)                                                 // SubKey is not found, neither on disk, nor in the mempool    
    {
        err=FC_ERR_NOERROR;

        LastFileID=m_DBStat.m_LastFileID;
        LastFileSize=m_DBStat.m_LastFileSize;
        
        size=FC_AllocSize(subkeysize,m_Database->m_TotalSize,1);                // Size SubKey takes in the file (padded)
        if(subkeysize)
        {
            if(LastFileSize+size>FC_TDB_MAX_TXS_FILE_SIZE)                          // New file is needed
            {
                FlushDataFile(LastFileID);
                LastFileID+=1;
                LastFileSize=0;
            }

            err=AddToFile(subkey,subkeysize,LastFileID,LastFileSize);
            if(err)
            {
                sprintf(msg,"Couldn't store key in file, error:  %d",err);
                LogString(msg);
                return err;
            }
        }
        
        txdef.Zero();
        meFCpy(txdef.m_TxId,hash, FC_TDB_TXID_SIZE);
        txdef.m_Size=subkeysize;
        txdef.m_FullSize=subkeysize;
        txdef.m_InternalFileID=LastFileID;
        txdef.m_InternalFileOffset=LastFileSize;
        txdef.m_Block=-1;                                                        
        txdef.m_BlockFileID=-1;            
        txdef.m_TimeReceived=FC_TimeNowAsUInt();
        txdef.m_Flags=flags | ( (subkey != NULL) ? FC_SFL_NONE : FC_SFL_NODATA );
        LastFileSize+=size;
        m_DBStat.m_LastFileID=LastFileID;                                       // Even if commit is unsuccessful we'll lose some place. On restart we return to previous values
        m_DBStat.m_LastFileSize=LastFileSize;
        rawmempool->Add(&txdef,(unsigned char*)&txdef+FC_TDB_TXID_SIZE);                
    }
    
    return err;
}

int FC_TxDB::DecrementSubKey(
              FC_TxImport *import, 
              FC_TxEntity *parent_entity,
              FC_TxEntity *entity)
{
    FC_TxImport *imp;
    FC_Buffer *mempool;
    FC_TxEntityStat *stat;
    FC_TxEntityRow erow;
    
    int err;
    int mprow,last_pos;
    char msg[256];
    int value_len; 
    unsigned char *ptr;
    
    err=FC_ERR_NOERROR;
    
    imp=m_Imports;
    mempool=m_MemPools[0];
    if(import)                                                                  // Find import
    {
        imp=import;
        mempool=m_MemPools[import-m_Imports];
    }
    
    stat=imp->GetEntity(imp->FindEntity(parent_entity));
    if(stat == NULL)
    {
        LogString("Could not find parent entity");
        err=FC_ERR_INTERNAL_ERROR;
        goto exitlbl;
    }

    erow.Zero();                                                                // Decrement row in mempool
    meFCpy(&erow.m_Entity,entity,sizeof(FC_TxEntity));
    erow.m_Pos=1;
    erow.m_Generation=stat->m_Generation;


    last_pos=0;
    mprow=mempool->Seek(&erow);
    if(mprow >= 0)                                                     
    {
        last_pos=((FC_TxEntityRow*)(mempool->GetRow(mprow)))->m_LastSubKeyPos;
        ((FC_TxEntityRow*)(mempool->GetRow(mprow)))->m_LastSubKeyPos=last_pos-1;
    }
    else
    {
        erow.Zero();
        meFCpy(&erow.m_Entity,entity,sizeof(FC_TxEntity));
        erow.m_Generation=stat->m_Generation;
        erow.m_Pos=1;
        erow.SwapPosBytes();
        ptr=(unsigned char*)m_Database->m_DB->Read((char*)&erow+m_Database->m_KeyOffset,m_Database->m_KeySize,&value_len,0,&err);
        erow.SwapPosBytes();
        if(err)
        {
            sprintf(msg,"Error while reading subkey entry from database: %d",err);
            LogString(msg);
            err=FC_ERR_INTERNAL_ERROR;
            goto exitlbl;
        }
        if(ptr)
        {
            meFCpy((char*)&erow+m_Database->m_ValueOffset,ptr,m_Database->m_ValueSize);
            last_pos=erow.m_LastSubKeyPos;
        }
        erow.Zero();                                                            
        meFCpy(&erow.m_Entity,entity,sizeof(FC_TxEntity));
        erow.m_Generation=stat->m_Generation;
        erow.m_Pos=1;
        erow.m_Flags=last_pos;
        erow.m_LastSubKeyPos=last_pos-1;        
        mempool->Add(&erow,(unsigned char*)&erow+FC_TDB_ENTITY_KEY_SIZE+FC_TDB_TXID_SIZE);        
    }
exitlbl:
            
    return err;
}

int FC_TxDB::IncrementSubKey(
              FC_TxImport *import,                                              
              FC_TxEntity *parent_entity,                                       
              FC_TxEntity *entity,                                              
              const unsigned char *subkey_hash,                                 
              const unsigned char *tx_hash,                                     
              int block,                                                        
              uint32_t flags,                                                   
              int newtx)
{
    FC_TxImport *imp;
    FC_Buffer *mempool;
    FC_TxEntityStat *stat;
    FC_TxEntityRow erow;
    int err;
    int mprow,last_pos;
    char msg[256];
    int value_len; 
    unsigned char *ptr;
    
    err=FC_ERR_NOERROR;
    
    imp=m_Imports;
    mempool=m_MemPools[0];
    if(import)                                                                  // Find import
    {
        imp=import;
        mempool=m_MemPools[import-m_Imports];
    }
    
    stat=imp->GetEntity(imp->FindEntity(parent_entity));
    if(stat == NULL)
    {
        LogString("Could not find parent entity");
        err=FC_ERR_INTERNAL_ERROR;
        goto exitlbl;
    }

    if((newtx != 0) ||                                                          // Row is new - add always
       ( (imp->m_ImportID > 0) &&
         ( (stat->m_Flags & FC_EFL_NOT_IN_SYNC) != 0 ) ) ||                     // All tx entities rows are new, except those in sync - like old addresses/wallet by timereceived
       ((parent_entity->m_EntityType & FC_TET_ORDERMASK) != FC_TET_TIMERECEIVED))// Ordered by chain position - add always   
    {
        erow.Zero();                                                            // Anchor row in mempool, will not be stored in database
        meFCpy(&erow.m_Entity,entity,sizeof(FC_TxEntity));
        erow.m_Generation=stat->m_Generation;

        
        last_pos=0;
        mprow=mempool->Seek(&erow);
        if(mprow >= 0)                                                     
        {
            mprow+=1;                                                               // Anchor doesn't carry info except key, but the next row is the first for this entity
            last_pos=((FC_TxEntityRow*)(mempool->GetRow(mprow)))->m_LastSubKeyPos;
            ((FC_TxEntityRow*)(mempool->GetRow(mprow)))->m_LastSubKeyPos=last_pos+1;
        }
        else
        {
            erow.Zero();
            meFCpy(&erow.m_Entity,entity,sizeof(FC_TxEntity));
            erow.m_Generation=stat->m_Generation;
            erow.m_Pos=1;
            erow.SwapPosBytes();
            ptr=(unsigned char*)m_Database->m_DB->Read((char*)&erow+m_Database->m_KeyOffset,m_Database->m_KeySize,&value_len,0,&err);
            erow.SwapPosBytes();
            if(err)
            {
                sprintf(msg,"Error while reading subkey entry from database: %d",err);
                LogString(msg);
                err=FC_ERR_INTERNAL_ERROR;
                goto exitlbl;
            }
            if(ptr)
            {
                meFCpy((char*)&erow+m_Database->m_ValueOffset,ptr,m_Database->m_ValueSize);
                last_pos=erow.m_LastSubKeyPos;
            }
            erow.Zero();                                                            
            meFCpy(&erow.m_Entity,entity,sizeof(FC_TxEntity));
            erow.m_Generation=stat->m_Generation;
            erow.m_LastSubKeyPos=last_pos+1;        
            mempool->Add(&erow,(unsigned char*)&erow+FC_TDB_ENTITY_KEY_SIZE+FC_TDB_TXID_SIZE);        
        }

        erow.Zero();                                                            
        meFCpy(&erow.m_Entity,entity,sizeof(FC_TxEntity));
        erow.m_Generation=stat->m_Generation;
        meFCpy(erow.m_TxId,tx_hash,FC_TDB_TXID_SIZE);
        erow.m_LastSubKeyPos=last_pos+1;        
        erow.m_TempPos=last_pos+1;        
        erow.m_Block=block;
        erow.m_Flags=flags;
        mempool->Add(&erow,(unsigned char*)&erow+FC_TDB_ENTITY_KEY_SIZE+FC_TDB_TXID_SIZE);        
        
        if(last_pos == 0)
        {
            erow.Zero();
            meFCpy(&erow.m_Entity,&stat->m_Entity,sizeof(FC_TxEntity));
            erow.m_Generation=stat->m_Generation;
            meFCpy(erow.m_TxId,subkey_hash,FC_TDB_ENTITY_ID_SIZE);
            erow.m_Block=block;
            erow.m_Flags=flags;
            stat->m_LastPos+=1;
            erow.m_TempPos=stat->m_LastPos;                                     // Will be copied to m_LastPos on commit. m_LastPos=0 to allow Seek() above
            mempool->Add(&erow,(unsigned char*)&erow+FC_TDB_ENTITY_KEY_SIZE+FC_TDB_TXID_SIZE);            
        }
    }


exitlbl:

    return err;
}

/*
 * Adds tx to specific import
 */

int FC_TxDB::AddTx(FC_TxImport *import,
                   const unsigned char *hash,
                   const unsigned char *tx,
                   uint32_t txsize,
                   uint32_t txfullsize,
                   int block,
                   int block_file,
                   uint32_t block_offset,
                   uint32_t block_tx_offset,
                   uint32_t block_tx_index,
                   uint32_t flags,
                   uint32_t timestamp,
                   FC_Buffer *entities)
{
    int err;
    char txhex[65];
    char txtype[16];
    char msg[256];
    
    int newtx,duplicate,isrelevant,ondisk,i,mprow,size;
    uint32_t LastFileID,LastFileSize; 
    FC_TxImport *imp;
    FC_Buffer *mempool;
    FC_Buffer *rawmempool;
    FC_TxEntityStat *stat;
    FC_TxEntityRow erow;
    FC_TxDefRow txdef;
    FC_TxDefRow *lptxdef;
    
    err=FC_ERR_NOERROR;

    sprintf_hex(txhex,hash,FC_TDB_TXID_SIZE);    
    
    imp=m_Imports;
    mempool=m_MemPools[0];
    rawmempool=m_RawMemPools[0];
    if(import)                                                                  // Find import
    {
        imp=import;
        mempool=m_MemPools[import-m_Imports];
        rawmempool=m_RawMemPools[import-m_Imports];
    }
    
    isrelevant=0;
    if(imp->m_ImportID)                                                         // Takes tx only if it has relevant entity for this import
    {
        for(i=0;i<entities->GetCount();i++)
        {
            if(isrelevant == 0)
            {
                if(imp->FindEntity((FC_TxEntity*)entities->GetRow(i)) >= 0)
                {
                    isrelevant=1;
                }
            }
        }
    }
    else
    {
        isrelevant=1;                                                           // Chain import updates its entity list
    }
    
    if(isrelevant == 0)
    {
        sprintf(msg,"Tx %s ignored for import %d",txhex,imp->m_ImportID);
        LogString(msg);
        goto exitlbl;        
    }

    newtx=0;
    ondisk=0;
    err=GetTx(&txdef,hash);
    if(err == FC_ERR_NOT_FOUND)                                                 // Tx is not found, neither on disk, nor in the mempool    
    {
        err=FC_ERR_NOERROR;
        newtx=1;

        size=FC_AllocSize(txsize,m_Database->m_TotalSize,1);                    // Size tx takes in the file (padded)
        LastFileID=m_DBStat.m_LastFileID;
        LastFileSize=m_DBStat.m_LastFileSize;
        
        if(LastFileSize+size>FC_TDB_MAX_TXS_FILE_SIZE)                          // New file is needed
        {
            FlushDataFile(LastFileID);
            LastFileID+=1;
            LastFileSize=0;
        }
        
        err=AddToFile(tx,txsize,LastFileID,LastFileSize);
        if(err)
        {
            sprintf(msg,"Couldn't store tx %s in file, error:  %d",txhex,err);
            LogString(msg);
            goto exitlbl;        
        }
        
        txdef.Zero();
        meFCpy(txdef.m_TxId,hash, FC_TDB_TXID_SIZE);
        txdef.m_Size=txsize;
        txdef.m_FullSize=txfullsize;
        txdef.m_InternalFileID=LastFileID;
        txdef.m_InternalFileOffset=LastFileSize;
        txdef.m_Block=block;                                                        
        if(block >= 0)                                                          // Set block coordinates only if processed in the block
        {
            txdef.m_BlockFileID=block_file;
            txdef.m_BlockOffset=block_offset;
            txdef.m_BlockTxOffset=block_tx_offset; 
            txdef.m_BlockIndex=block_tx_index;
        }
        else
        {
            txdef.m_BlockFileID=-1;            
        }
        txdef.m_TimeReceived=timestamp;
        txdef.m_Flags=flags;
        LastFileSize+=size;
        m_DBStat.m_LastFileID=LastFileID;                                       // Even if commit is unsuccessful we'll lose some place. On restart we return to previous values
        m_DBStat.m_LastFileSize=LastFileSize;
        rawmempool->Add(&txdef,(unsigned char*)&txdef+FC_TDB_TXID_SIZE);        
    }
    else
    {
        if(err)
        {
            sprintf(msg,"Internal error while looking for tx %s in raw database, error: %d",txhex,err);
            LogString(msg);
            goto exitlbl;            
        }
        ondisk=1;
        mprow=rawmempool->Seek((unsigned char*)hash);
        if(mprow >= 0)                                                          // Found in mempool
        {
            ondisk=0;
            lptxdef=(FC_TxDefRow *)rawmempool->GetRow(mprow);
            lptxdef->m_Block=block;                                             // If on disk, block is not updated. In case in case of rollback, on-disk value will be higher 
                                                                                // than chain height -considered as mempool.
            lptxdef->m_Flags=flags;
            if(block >= 0)                                                      // Update block pos only if processed in the block
                                                                                // If on disk, no matter what is going on here, old reference is good
            {
                lptxdef->m_BlockFileID=block_file;
                lptxdef->m_BlockOffset=block_offset;
                lptxdef->m_BlockTxOffset=block_tx_offset;      
                lptxdef->m_BlockIndex=block_tx_index;
            }
            else
            {
                lptxdef->m_BlockFileID=-1;            
            }
        }
    }
    
    duplicate=1;
    for(i=0;i<entities->GetCount();i++)                                         // Processing entities
    {
        isrelevant=0;
        if(imp->FindEntity((FC_TxEntity*)entities->GetRow(i)) >= 0)
        {
            isrelevant=1;
        }
        else
        {
//            if(imp->m_ImportID == 0)
            if(false)                                                           // Disabled. Fully synced addresses should be added by higher level
                                                                                // Using AddEntity call 
            {
                imp->AddEntity((FC_TxEntity*)entities->GetRow(i));              // New entities added to chain import
                isrelevant=1;
            }
        }
        
        if(isrelevant)
        {
            stat=imp->GetEntity(imp->FindEntity((FC_TxEntity*)entities->GetRow(i)));
            if(stat == NULL)
            {
                sprintf(msg,"Could not add tx %s, entity not found",txhex);
                LogString(msg);
                err=FC_ERR_INTERNAL_ERROR;
                goto exitlbl;
            }
            
            erow.Zero();
            meFCpy(&erow.m_Entity,&stat->m_Entity,sizeof(FC_TxEntity));
            erow.m_Generation=stat->m_Generation;
            meFCpy(erow.m_TxId,hash,FC_TDB_TXID_SIZE);
            mprow=mempool->Seek(&erow);
            if(mprow >= 0)                                                      // Update block and flags if found in mempool
            {
                ((FC_TxEntityRow*)(mempool->GetRow(mprow)))->m_Block=block;
                ((FC_TxEntityRow*)(mempool->GetRow(mprow)))->m_Flags=flags;                
            }
            else
            {
                if((newtx != 0) ||                                              // Row is new - add always
                   ( (imp->m_ImportID > 0) &&
                     ( (stat->m_Flags & FC_EFL_NOT_IN_SYNC) != 0 ) ) ||         // All tx entities rows are new, except those in sync - like old addresses/wallet by timereceived
                   ((erow.m_Entity.m_EntityType & FC_TET_ORDERMASK) != FC_TET_TIMERECEIVED)) // Ordered by chain position - add always   
                {                        
                    erow.m_Block=block;
                    erow.m_Flags=flags;
                    stat->m_LastPos+=1;
                    erow.m_TempPos=stat->m_LastPos;                             // Will be copied to m_LastPos on commit. m_LastPos=0 to allow Seek() above
                    mempool->Add(&erow,(unsigned char*)&erow+FC_TDB_ENTITY_KEY_SIZE+FC_TDB_TXID_SIZE);
                    duplicate=0;
                }
            }            
        }                
    }

    if(imp->m_ImportID == 0)                                                    // Update block and flags for txs already on disk
    {    
        if(newtx == 0)
        {
            if(ondisk)
            {
                txdef.m_Block=block;                                                        
                if(block >= 0)                                                  // Don't erase old block coordinates
                {
                    txdef.m_BlockFileID=block_file;
                    txdef.m_BlockOffset=block_offset;
                    txdef.m_BlockTxOffset=block_tx_offset; 
                    txdef.m_BlockIndex=block_tx_index;
                }
                txdef.m_Flags=flags;
                mprow=m_RawUpdatePool->Seek((unsigned char*)hash);
                if(mprow >= 0)
                {
                    meFCpy((unsigned char*)m_RawUpdatePool->GetRow(mprow)+FC_TDB_TXID_SIZE,(unsigned char*)&txdef+FC_TDB_TXID_SIZE,m_Database->m_ValueSize);
                }
                else
                {
                    m_RawUpdatePool->Add((unsigned char *)&txdef,(unsigned char*)&txdef+FC_TDB_TXID_SIZE);
                }
            }
        }
    }
    
    if(newtx)
    {
        sprintf(txtype,"New");
        sprintf(msg,"NewTx %s, block %d, flags %08X, import %d",txhex,block,flags,imp->m_ImportID);
    }
    else
    {
        txhex[10]=0;
        if(duplicate)
        {
            sprintf(txtype,"Duplicate");            
        }
        else
        {
            sprintf(txtype,"Update");            
        }
        sprintf(msg,"%sTx %s, block %d, flags %08X, import %d",txtype,txhex,block,flags,imp->m_ImportID);
    }
    
    LogString(msg);
exitlbl:
    return err;
}

/*
 * Saves flag (normally "invalid") for specific transaction,
 * If invalid transaction becomes valid again this flag will be overwritten by AddTx
 */


int FC_TxDB::SaveTxFlag(const unsigned char *hash,uint32_t flag,int set_flag)
{
    int err,value_len,mprow; 
    unsigned char *ptr;
    char txhex[65];
    char msg[256];
    FC_TxDefRow *txdef;
    FC_TxDefRow StoredTxDef;
    err=FC_ERR_NOERROR;

    sprintf_hex(txhex,hash,FC_TDB_TXID_SIZE);    

    if(set_flag)
    {
        sprintf(msg,"Setting flag %08X to tx %s",flag,txhex);
    }
    else
    {
        sprintf(msg,"Unsetting flag %08X to tx %s",flag,txhex);        
    }    
    LogString(msg);

    mprow=m_RawMemPools[0]->Seek((unsigned char*)hash);
    if(mprow >= 0)
    {
        txdef=(FC_TxDefRow *)m_RawMemPools[0]->GetRow(mprow);
        if(set_flag)
        {
            txdef->m_Flags |= flag;
        }
        else
        {
            txdef->m_Flags &= ~flag;
        }
        return FC_ERR_NOERROR;
    }
    
    mprow=m_RawUpdatePool->Seek((unsigned char*)hash);
    if(mprow >= 0)
    {
        txdef=(FC_TxDefRow *)m_RawUpdatePool->GetRow(mprow);
        if(set_flag)
        {
            txdef->m_Flags |= flag;
        }
        else
        {
            txdef->m_Flags &= ~flag;
        }
    }
    
    StoredTxDef.Zero();
    
    meFCpy(StoredTxDef.m_TxId,hash, FC_TDB_TXID_SIZE);
    ptr=(unsigned char*)m_Database->m_DB->Read((char*)&StoredTxDef+m_Database->m_KeyOffset,m_Database->m_KeySize,&value_len,0,&err);
    if(err)
    {
        return err;
    }
    
    if(ptr)                                                                     
    {
        meFCpy((char*)&StoredTxDef+m_Database->m_ValueOffset,ptr,m_Database->m_ValueSize);        
        if(set_flag)
        {
            StoredTxDef.m_Flags |= flag;
        }
        else
        {
            StoredTxDef.m_Flags &= ~flag;
        }
    }
    else
    {
        return FC_ERR_NOT_FOUND;
    }
    
    err=m_Database->m_DB->Write((char*)&StoredTxDef+m_Database->m_KeyOffset,m_Database->m_KeySize,(char*)&StoredTxDef+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
    if(err)
    {
        return err;
    }                            

    err=m_Database->m_DB->Commit(FC_OPT_DB_DATABASE_TRANSACTIONAL);
    if(err)
    {
        return err;
    }
    
    return FC_ERR_NOERROR;
}


int FC_TxDB::GetTx(FC_TxDefRow *txdef,
              const unsigned char *hash)
{
    int err,value_len,mprow; 
    unsigned char *ptr;

    txdef->Zero();
    
    err=FC_ERR_NOERROR;

    mprow=m_RawMemPools[0]->Seek((unsigned char*)hash);
    if(mprow >= 0)
    {
        meFCpy(txdef,(FC_TxDefRow *)m_RawMemPools[0]->GetRow(mprow),sizeof(FC_TxDefRow));
        if(txdef->m_Block > m_DBStat.m_Block)                                   
        {
            txdef->m_Block=-1;
        }
        return FC_ERR_NOERROR;
    }

    mprow=m_RawUpdatePool->Seek((unsigned char*)hash);
    if(mprow >= 0)
    {
        meFCpy(txdef,(FC_TxDefRow *)m_RawUpdatePool->GetRow(mprow),sizeof(FC_TxDefRow));
        if(txdef->m_Block > m_DBStat.m_Block)                   
        {
            txdef->m_Block=-1;
        }
        return FC_ERR_NOERROR;
    }
    
    meFCpy(txdef->m_TxId,hash, FC_TDB_TXID_SIZE);
    ptr=(unsigned char*)m_Database->m_DB->Read((char*)txdef+m_Database->m_KeyOffset,m_Database->m_KeySize,&value_len,0,&err);
    if(err)
    {
        return err;
    }
    
    if(ptr)                                                                     
    {
        meFCpy((char*)txdef+m_Database->m_ValueOffset,ptr,m_Database->m_ValueSize);        
        if(txdef->m_Block > m_DBStat.m_Block)                                   // On-disk records are not updated on rollback
        {
            txdef->m_Block=-1;
        }
    }
    else
    {
        err=FC_ERR_NOT_FOUND;
    }
    
    
    return err;
    
}

int FC_TxDB::BeforeCommit(FC_TxImport *import)
{
    int err,i,j;
    FC_TxImport *imp;
    FC_Buffer *mempool;
    FC_TxEntityStat *stat;
    FC_TxEntityRow *lperow;
    
    err=FC_ERR_NOERROR;
    
    Dump("Before BeforeCommit");
    
    imp=m_Imports;
    mempool=m_MemPools[0];
    if(import)
    {
        imp=import;
        mempool=m_MemPools[import-m_Imports];
    }

    if(imp->m_ImportID)
    {
        return err;
    }
    
    for(i=0;i<imp->m_Entities->GetCount();i++)                                 // All not FC_TET_TIMERECEIVED are removed from mempool
    {
        stat=(FC_TxEntityStat*)imp->m_Entities->GetRow(i);                     
        if((stat->m_Entity.m_EntityType & FC_TET_ORDERMASK) != FC_TET_TIMERECEIVED)
        {
            stat->m_LastPos=stat->m_LastClearedPos;
        }
    }
    
    j=0;
    for(i=0;i<mempool->GetCount();i++)
    {
        lperow=(FC_TxEntityRow*)mempool->GetRow(i);
        if((lperow->m_Entity.m_EntityType & FC_TET_ORDERMASK) == FC_TET_TIMERECEIVED)
        {
            meFCpy(mempool->GetRow(j),mempool->GetRow(i),sizeof(FC_TxEntityRow));
            j++;
        }
    }
    mempool->SetCount(j);    
    Dump("After BeforeCommit");
    
    return err;
}

int FC_TxDB::Commit(FC_TxImport *import)
{
    int err;
    char msg[256];
    
    int i,block;
    FC_TxImport *imp;
    FC_Buffer *mempool;
    FC_Buffer *rawmempool;
    FC_TxEntityStat *stat;
    FC_TxEntityRow *lperow;
    FC_TxDefRow *lptxdef;
    FC_TxEntityRow erow;
    FC_TxImportRow edbImport;
    int anchor;
    int value_len; 
    unsigned char *ptr;
    
    
    Dump("Before Commit");
    err=FC_ERR_NOERROR;
    
    
    imp=m_Imports;
    mempool=m_MemPools[0];
    rawmempool=m_RawMemPools[0];
    if(import)
    {
        imp=import;
        mempool=m_MemPools[import-m_Imports];
        rawmempool=m_RawMemPools[import-m_Imports];
    }
    block=imp->m_Block+1;
    

    for(i=0;i<rawmempool->GetCount();i++)                                       // New raw transactions
    {
        lptxdef=(FC_TxDefRow *)rawmempool->GetRow(i);
        m_DBStat.m_Count+=1;
        m_DBStat.m_FullSize+=FC_AllocSize(lptxdef->m_Size,m_Database->m_TotalSize,1);
        err=m_Database->m_DB->Write((char*)lptxdef+m_Database->m_KeyOffset,m_Database->m_KeySize,(char*)lptxdef+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
        if(err)
        {
            goto exitlbl;
        }                            
    }
    
    if(imp->m_ImportID == 0)                                                    // Raw transaction updates
    {
        for(i=0;i<m_RawUpdatePool->GetCount();i++)                              
        {
            lptxdef=(FC_TxDefRow *)m_RawUpdatePool->GetRow(i);
            m_DBStat.m_FullSize+=FC_AllocSize(lptxdef->m_Size,m_Database->m_TotalSize,1);
            err=m_Database->m_DB->Write((char*)lptxdef+m_Database->m_KeyOffset,m_Database->m_KeySize,(char*)lptxdef+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
            if(err)
            {
                goto exitlbl;
            }                            
        }
    }
    
    anchor=0;
    for(i=0;i<mempool->GetCount();i++)                                          // mempool items
    {
        lperow=(FC_TxEntityRow *)mempool->GetRow(i);        
        if((lperow->m_Entity.m_EntityType & FC_TET_SUBKEY) && (lperow->m_TempPos == 0))
        {
            anchor=1;
        }
        else
        {
            if(anchor)
            {
                anchor=0;
                if(lperow->m_TempPos>1)
                {
                    erow.Zero();
                    meFCpy(&erow.m_Entity,&(lperow->m_Entity),sizeof(FC_TxEntity));
                    erow.m_Generation=lperow->m_Generation;
                    erow.m_Pos=1;
                    erow.SwapPosBytes();
                    ptr=(unsigned char*)m_Database->m_DB->Read((char*)&erow+m_Database->m_KeyOffset,m_Database->m_KeySize,&value_len,0,&err);
                    erow.SwapPosBytes();
                    if(err)
                    {
                        goto exitlbl;
                    }
                    if(ptr == NULL)
                    {
                        err=FC_ERR_CORRUPTED;
                        goto exitlbl;
                    }
                    meFCpy((char*)&erow+m_Database->m_ValueOffset,ptr,m_Database->m_ValueSize);
                    erow.m_LastSubKeyPos=lperow->m_LastSubKeyPos;
                    erow.SwapPosBytes();
                    err=m_Database->m_DB->Write((char*)&erow+m_Database->m_KeyOffset,m_Database->m_KeySize,(char*)&erow+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
                    erow.SwapPosBytes();
                }
            }
            lperow->m_Pos=lperow->m_TempPos;                                    // m_Pos in mempool was 0 - to support search by TxID  in mempool
            lperow->m_TempPos=0;
            lperow->SwapPosBytes();
            err=m_Database->m_DB->Write((char*)lperow+m_Database->m_KeyOffset,m_Database->m_KeySize,(char*)lperow+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
            lperow->SwapPosBytes();
            if(err)
            {
                goto exitlbl;
            }                            
        }
    }
    
    edbImport.Zero();                                                           // Import block update
    edbImport.m_Block=block;
    edbImport.m_ImportID=imp->m_ImportID;
    imp->m_Block=block;
    edbImport.SwapPosBytes();
    err=m_Database->m_DB->Write((char*)&edbImport+m_Database->m_KeyOffset,m_Database->m_KeySize,(char*)&edbImport+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
    edbImport.SwapPosBytes();
    if(err)
    {
        goto exitlbl;
    }                    
    
    for(i=0;i<imp->m_Entities->GetCount();i++)                                  // Import entity update
    {
        stat=(FC_TxEntityStat*)imp->m_Entities->GetRow(i);
        if(stat->m_LastPos != stat->m_LastClearedPos)                           // Entity has new items
        {
            edbImport.Zero();
            meFCpy(&(edbImport.m_Entity), &(stat->m_Entity),sizeof(FC_TxEntity));
            stat->m_LastClearedPos=stat->m_LastPos;
            edbImport.m_ImportID=imp->m_ImportID;
            edbImport.m_Generation=stat->m_Generation;
            edbImport.m_LastPos=stat->m_LastPos;
            edbImport.m_Pos=stat->m_PosInImport;
            edbImport.m_LastImportedBlock=stat->m_LastImportedBlock;
            edbImport.m_TimeAdded=stat->m_TimeAdded;
            edbImport.m_Flags=stat->m_Flags;
            edbImport.m_Block=block;
            edbImport.SwapPosBytes();
            err=m_Database->m_DB->Write((char*)&edbImport+m_Database->m_KeyOffset,m_Database->m_KeySize,(char*)&edbImport+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
            edbImport.SwapPosBytes();
            if(err)
            {
                goto exitlbl;
            }                    
        }
    }
    
    if(imp->m_ImportID == 0)
    {
        m_DBStat.m_Block=block;                                                 // DB stats
        err=m_Database->m_DB->Write((char*)&m_DBStat+m_Database->m_KeyOffset,m_Database->m_KeySize,(char*)&m_DBStat+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
        if(err)
        {
            goto exitlbl;
        }                            
    }
    
    FlushDataFile(m_DBStat.m_LastFileID);

    err=m_Database->m_DB->Commit(FC_OPT_DB_DATABASE_TRANSACTIONAL);
    if(err)
    {
        goto exitlbl;
    }

    
exitlbl:

    if(err)
    {
        sprintf(msg,"Could not commit new Block %d, Txs: %d, Rows: %d, Error: %d",block,rawmempool->GetCount(),mempool->GetCount(),err);
        LogString(msg);   
        RollBack(import,block-1);
    }
    else
    {
        sprintf(msg,"NewBlock %d, Txs: %d, Rows: %d",block,rawmempool->GetCount(),mempool->GetCount());
        LogString(msg);   
        mempool->Clear();
        rawmempool->Clear();
        m_RawUpdatePool->Clear();
    }
    Dump("After Commit");

    return err;
}

int FC_TxDB::RollBack(FC_TxImport *import,int block)
{
    int value_len; 
    unsigned char *ptr;
    int err;
    char msg[256];
    
    int i,j;
    FC_TxImport *imp;
    FC_Buffer *mempool;
    FC_TxEntityStat *stat;
    FC_TxEntityRow *lperow;
    FC_TxImportRow edbImport;
    FC_TxEntityRow erow;
    uint32_t pos;
    
    err=FC_ERR_NOERROR;
   
    Dump("Before RollBack");
    imp=m_Imports;
    mempool=m_MemPools[0];
    if(import)
    {
        imp=import;
        mempool=m_MemPools[import-m_Imports];
    }

    for(i=0;i<mempool->GetCount();i++)                                          // removing decremented subkey rows
    {
        lperow=(FC_TxEntityRow *)mempool->GetRow(i);        
        if((lperow->m_Entity.m_EntityType & FC_TET_SUBKEY) && (lperow->m_TempPos == 0) && (lperow->m_Pos == 1))
        {
                
            for(j=lperow->m_LastSubKeyPos;j<(int)lperow->m_Flags;j++)       // old count is carried by flags field of anchor row
            {
                erow.Zero();
                meFCpy(&erow.m_Entity,&(lperow->m_Entity),sizeof(FC_TxEntity));
                erow.m_Generation=lperow->m_Generation;
                erow.m_Pos=j+1;
                erow.SwapPosBytes();
                err=m_Database->m_DB->Delete((char*)&erow+m_Database->m_KeyOffset,m_Database->m_KeySize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
                erow.SwapPosBytes();
                if(err)
                {
                    goto exitlbl;
                }
            }
            if(lperow->m_LastSubKeyPos)                                     // Updating first subkey row
            {
                erow.Zero();
                meFCpy(&erow.m_Entity,&(lperow->m_Entity),sizeof(FC_TxEntity));
                erow.m_Generation=lperow->m_Generation;
                erow.m_Pos=1;
                erow.SwapPosBytes();
                ptr=(unsigned char*)m_Database->m_DB->Read((char*)&erow+m_Database->m_KeyOffset,m_Database->m_KeySize,&value_len,0,&err);
                erow.SwapPosBytes();
                if(err)
                {
                    goto exitlbl;
                }
                if(ptr == NULL)
                {
                    err=FC_ERR_CORRUPTED;
                    goto exitlbl;
                }
                meFCpy((char*)&erow+m_Database->m_ValueOffset,ptr,m_Database->m_ValueSize);                    
                erow.m_LastSubKeyPos=lperow->m_LastSubKeyPos;
                erow.SwapPosBytes();
                err=m_Database->m_DB->Write((char*)&erow+m_Database->m_KeyOffset,m_Database->m_KeySize,(char*)&erow+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
                erow.SwapPosBytes();
                if(err)
                {
                    goto exitlbl;
                }                    
            }
        }
    }


    j=0;                                                                        // Clearing mempool transactions except FC_TET_TIMERECEIVED
    for(i=0;i<mempool->GetCount();i++)
    {
        lperow=(FC_TxEntityRow*)mempool->GetRow(i);
        if((lperow->m_Entity.m_EntityType & FC_TET_ORDERMASK) == FC_TET_TIMERECEIVED)
        {
            meFCpy(mempool->GetRow(j),mempool->GetRow(i),sizeof(FC_TxEntityRow));
            j++;
        }
    }
    mempool->SetCount(j);    
    if(imp->m_ImportID == 0)
    {
        m_RawUpdatePool->Clear();
    }
    
    for(i=0;i<imp->m_Entities->GetCount();i++)                                  // Removing ordered-by-blockchain-position rows
    {
        stat=(FC_TxEntityStat*)imp->m_Entities->GetRow(i);
        if((stat->m_Entity.m_EntityType & FC_TET_ORDERMASK) != FC_TET_TIMERECEIVED)
        {        
            stat->m_LastPos=stat->m_LastClearedPos;
            pos=stat->m_LastPos;
            erow.Zero();
            meFCpy(&erow.m_Entity,&(stat->m_Entity),sizeof(FC_TxEntity));
            erow.m_Generation=stat->m_Generation;
            while(pos)
            {
                erow.m_Pos=pos;
                erow.SwapPosBytes();
                ptr=(unsigned char*)m_Database->m_DB->Read((char*)&erow+m_Database->m_KeyOffset,m_Database->m_KeySize,&value_len,0,&err);
                erow.SwapPosBytes();
                if(err)
                {
                    goto exitlbl;
                }

                if(ptr)                                                                     
                {
                    meFCpy((char*)&erow+m_Database->m_ValueOffset,ptr,m_Database->m_ValueSize);        
                    if(erow.m_Block > block)
                    {
                        erow.SwapPosBytes();
                        err=m_Database->m_DB->Delete((char*)&erow+m_Database->m_KeyOffset,m_Database->m_KeySize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
                        erow.SwapPosBytes();
                        pos--;
                        stat->m_LastPos=pos;
                    }
                    else
                    {
                        pos=0;
                    }
                }
                else
                {
                    err=FC_ERR_NOT_FOUND;
                    goto exitlbl;
                }
            }
            
            edbImport.Zero();                                                   // Updating import entity stats
            meFCpy(&(edbImport.m_Entity), &(stat->m_Entity),sizeof(FC_TxEntity));
            edbImport.m_ImportID=imp->m_ImportID;
            stat->m_LastClearedPos=stat->m_LastPos;
            edbImport.m_Generation=stat->m_Generation;
            edbImport.m_LastPos=stat->m_LastPos;
            edbImport.m_Pos=stat->m_PosInImport;
            edbImport.m_LastImportedBlock=stat->m_LastImportedBlock;
            edbImport.m_TimeAdded=stat->m_TimeAdded;
            edbImport.m_Flags=stat->m_Flags;
            edbImport.m_Block=block;
            edbImport.SwapPosBytes();
            err=m_Database->m_DB->Write((char*)&edbImport+m_Database->m_KeyOffset,m_Database->m_KeySize,(char*)&edbImport+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
            edbImport.SwapPosBytes();
            if(err)
            {
                goto exitlbl;
            }                                
        }        
    }    

    edbImport.Zero();                                                           // Import block update
    edbImport.m_Block=block;
    edbImport.m_ImportID=imp->m_ImportID;
    edbImport.SwapPosBytes();
    err=m_Database->m_DB->Write((char*)&edbImport+m_Database->m_KeyOffset,m_Database->m_KeySize,(char*)&edbImport+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
    edbImport.SwapPosBytes();
    if(err)
    {
        goto exitlbl;
    }                    
    
    
    if(imp->m_ImportID == 0)
    {
        m_DBStat.m_Block=block;
        err=m_Database->m_DB->Write((char*)&m_DBStat+m_Database->m_KeyOffset,m_Database->m_KeySize,(char*)&m_DBStat+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
        if(err)
        {
            goto exitlbl;
        }                            
    }
    
    err=m_Database->m_DB->Commit(FC_OPT_DB_DATABASE_TRANSACTIONAL);
    if(err)
    {
        goto exitlbl;
    }

    imp->m_Block=block;
    
exitlbl:

    if(err)
    {
        sprintf(msg,"Could not roll back to block %d, error: %d",block,err);        
        LogString(msg);
    }
    else
    {
        sprintf(msg,"Rolled back to block %d successfully",block);
        LogString(msg);
    }

    Dump("After RollBack");
    return err;
}

int FC_TxDB::GetRow(
               FC_TxEntityRow *erow)
{
    int err, value_len,i;
    unsigned char *ptr;
    FC_TxEntityRow *lpEnt;

    err=FC_ERR_NOERROR;

    erow->SwapPosBytes();
    ptr=(unsigned char*)m_Database->m_DB->Read((char*)erow+m_Database->m_KeyOffset,m_Database->m_KeySize,&value_len,0,&err);
    erow->SwapPosBytes();
    if(err)
    {
        return err;
    }
    if(ptr)
    {
        meFCpy((char*)erow+m_Database->m_ValueOffset,ptr,m_Database->m_ValueSize);
        return FC_ERR_NOERROR;
    }
    else
    {
        for(i=0;i<m_MemPools[0]->GetCount();i++)
        {
            lpEnt=(FC_TxEntityRow *)m_MemPools[0]->GetRow(i);
            if( (lpEnt->m_TempPos == erow->m_Pos) && 
                (meFCmp(&(lpEnt->m_Entity),&(erow->m_Entity),sizeof(FC_TxEntity)) == 0))
            {
                meFCpy(erow,lpEnt,FC_TDB_ROW_SIZE);
                erow->m_Pos=lpEnt->m_TempPos;
                return FC_ERR_NOERROR;
            }            
        }
    }
    
    return FC_ERR_NOT_FOUND;
    
}

int FC_TxDB::GetList(FC_TxEntity *entity,
                int from,
                int count,
                FC_Buffer *txs)
{
    return GetList(m_Imports,entity,from,count,txs);
}

int FC_TxDB::GetList(
                FC_TxImport *import,
                FC_TxEntity *entity,
                int from,
                int count,
                FC_Buffer *txs)
{
    int first,last,i;
    FC_TxEntityRow erow;
    FC_TxEntityRow *lpEnt;
    FC_Buffer *mempool;
    int value_len; 
    unsigned char *ptr;
    int err,mprow,found;
    char msg[256];
    
    txs->Clear();
    
    int row;
    FC_TxEntityStat *stat;
    
    row=import->FindEntity(entity);
    if(row < 0)
    {
        return FC_ERR_NOERROR;
    }
    
    mempool=m_MemPools[import-m_Imports];
    stat=(FC_TxEntityStat*)import->m_Entities->GetRow(row);
    
    first=from;
    last=GetListSize(entity,NULL);
    if(first <= 0)
    {
        first=last-count+1+first;
    }
    if(first+count-1 < 1)
    {
        return FC_ERR_NOERROR;        
    }
    if(first<=0)
    {
        count-=1-first;
        first=1;
    }
    if(first+count-1 <= last)
    {
        last=first+count-1;
    }
    
    
    erow.Zero();
    meFCpy(&erow.m_Entity,&(stat->m_Entity),sizeof(FC_TxEntity));
    erow.m_Generation=stat->m_Generation;
    mprow=-1;
    for(i=first;i<=last;i++)
    {
        erow.m_Pos=i;
        if(erow.m_Pos <= stat->m_LastClearedPos)                                // Database rows
        {
            erow.SwapPosBytes();
            ptr=(unsigned char*)m_Database->m_DB->Read((char*)&erow+m_Database->m_KeyOffset,m_Database->m_KeySize,&value_len,0,&err);
            erow.SwapPosBytes();
            if(err)
            {
                return err;
            }

            if(ptr)                                                                     
            {
                txs->Add((char*)&erow,ptr);
            }
            else
            {
                sprintf(msg,"GetList: couldn't find item %d in database, entity type %08X",i,erow.m_Entity.m_EntityType);
                LogString(msg);
                return FC_ERR_NOT_FOUND;
            }        
        }
        else                                                                    // mempool rows
        {
            mprow++;
            found=0;
            while((found == 0) && (mprow < mempool->GetCount()))
            {
                lpEnt=(FC_TxEntityRow *)mempool->GetRow(mprow);
                if( (lpEnt->m_TempPos == erow.m_Pos) && 
                    (meFCmp(&(lpEnt->m_Entity),entity,sizeof(FC_TxEntity)) == 0))
                {
                    meFCpy(&erow,lpEnt,FC_TDB_ROW_SIZE);
                    erow.m_Pos=i;                    
                    txs->Add((char*)&erow,(char*)&erow+FC_TDB_ENTITY_KEY_SIZE);                
                    found=1;
                }
                else
                {
                    mprow++;
                }
            }
            if(mprow >= mempool->GetCount())
            {
                sprintf(msg,"GetList: couldn't find item %d in mempool, entity type %08X",i,erow.m_Entity.m_EntityType);
                LogString(msg);
                return FC_ERR_NOT_FOUND;                
            }
        }
    }    
    
    return FC_ERR_NOERROR;
}

int FC_TxDB::GetList(FC_TxEntity *entity,
                int generation,
                int from,
                int count,
                FC_Buffer *txs)
{
    return GetList(m_Imports,entity,generation,from,count,txs);
}

int FC_TxDB::GetList(
                FC_TxImport *import,
                FC_TxEntity *entity,
                int generation,
                int from,
                int count,
                FC_Buffer *txs)
{
    int first,last,i,confirmed;
    FC_TxEntityRow erow;
    FC_TxEntityRow *lpEnt;
    FC_Buffer *mempool;
    int value_len; 
    unsigned char *ptr;
    int err,mprow,found;
    char msg[256];
    
    txs->Clear();
    
    mempool=m_MemPools[import-m_Imports];
    
    first=from;
    last=GetListSize(entity,generation,&confirmed);
    
    if(last <= 0)
    {
        return FC_ERR_NOERROR;
    }
    
    if(first <= 0)
    {
        first=last-count+1+first;
    }
    if(first+count-1 < 1)
    {
        return FC_ERR_NOERROR;        
    }
    if(first<=0)
    {
        first=1;
    }
    if(first+count-1 <= last)
    {
        last=first+count-1;
    }
    
    
    erow.Zero();
    meFCpy(&erow.m_Entity,entity,sizeof(FC_TxEntity));
    erow.m_Generation=generation;
    mprow=-1;
    for(i=first;i<=last;i++)
    {
        erow.m_Pos=i;
        if((int)erow.m_Pos <= confirmed)                                // Database rows
        {
            erow.SwapPosBytes();
            ptr=(unsigned char*)m_Database->m_DB->Read((char*)&erow+m_Database->m_KeyOffset,m_Database->m_KeySize,&value_len,0,&err);
            erow.SwapPosBytes();
            if(err)
            {
                return err;
            }

            if(ptr)                                                                     
            {
                txs->Add((char*)&erow,ptr);
            }
            else
            {
                sprintf(msg,"GetList: couldn't find item %d in database, entity type %08X",i,erow.m_Entity.m_EntityType);
                LogString(msg);
                return FC_ERR_NOT_FOUND;
            }        
        }
        else                                                                    // mempool rows
        {
            mprow++;
            found=0;
            while((found == 0) && (mprow < mempool->GetCount()))
            {
                lpEnt=(FC_TxEntityRow *)mempool->GetRow(mprow);
                if( (lpEnt->m_TempPos == erow.m_Pos) && 
                    (meFCmp(&(lpEnt->m_Entity),entity,sizeof(FC_TxEntity)) == 0))
                {
                    meFCpy(&erow,lpEnt,FC_TDB_ROW_SIZE);
                    erow.m_Pos=i;                    
                    txs->Add((char*)&erow,(char*)&erow+FC_TDB_ENTITY_KEY_SIZE);                
                    found=1;
                }
                else
                {
                    mprow++;
                }
            }
            if(mprow >= mempool->GetCount())
            {
                sprintf(msg,"GetList: couldn't find item %d in mempool, entity type %08X",i,erow.m_Entity.m_EntityType);
                LogString(msg);
                return FC_ERR_NOT_FOUND;                
            }
        }
    }    
    
    return FC_ERR_NOERROR;
}

int FC_TxDB::GetBlockItemIndex(FC_TxImport *import,FC_TxEntity *entity,int block)
{
    FC_TxImport *imp;
    int first,last,next;
   
    imp=m_Imports;
    if(import)
    {
        imp=import;
    }
    
    FC_TxEntityRow erow;
    int row;
    FC_TxEntityStat *stat;
    
    row=imp->FindEntity(entity);
    if(row < 0)
    {
        return 0;
    }
    
    stat=(FC_TxEntityStat*)imp->m_Entities->GetRow(row);
    
    first=1;
    GetListSize(entity,stat->m_Generation,&last);
        
    if(last <= 0)
    {
        return 0;
    }
    
    erow.Zero();
    meFCpy(&erow.m_Entity,&(stat->m_Entity),sizeof(FC_TxEntity));
    erow.m_Generation=stat->m_Generation;
    
    erow.m_Pos=first;
    GetRow(&erow);
    if(erow.m_Block > block)
    {
        return 0;
    }
    
    erow.m_Pos=last;
    GetRow(&erow);
    if(erow.m_Block <= block)
    {
        return last;        
    }
    
    while(last-first > 1)
    {
        next=(last+first)/2;
        erow.m_Pos=next;
        GetRow(&erow);
        if(erow.m_Block > block)
        {
            last=next;
        }
        else
        {
            first=next;
        }
    }
    
    return first;
}
    

int FC_TxDB::GetListSize(FC_TxEntity *entity,int generation,int *confirmed)
{
    int last_pos,mprow,err,value_len,last_conf;
    FC_TxEntityRow erow;
    unsigned char *ptr;

    last_conf=0;
    if(entity->m_EntityType & FC_TET_SUBKEY)
    {
        erow.Zero();
        meFCpy(&erow.m_Entity,entity,sizeof(FC_TxEntity));
        erow.m_Generation=generation;
        
        last_pos=0;
        mprow=m_MemPools[0]->Seek(&erow);
        if(mprow >= 0)                                                     
        {
            mprow+=1;                                                           // Anchor doesn't carry info except key, but the next row is the first for this entity
            last_pos=((FC_TxEntityRow*)(m_MemPools[0]->GetRow(mprow)))->m_LastSubKeyPos;            
            last_conf=((FC_TxEntityRow*)(m_MemPools[0]->GetRow(mprow)))->m_TempPos-1;
        }
        else
        {
            erow.Zero();
            meFCpy(&erow.m_Entity,entity,sizeof(FC_TxEntity));
            erow.m_Generation=generation;
            erow.m_Pos=1;
            erow.SwapPosBytes();
            ptr=(unsigned char*)m_Database->m_DB->Read((char*)&erow+m_Database->m_KeyOffset,m_Database->m_KeySize,&value_len,0,&err);
            erow.SwapPosBytes();
            if(err)
            {
                return 0;
            }
            if(ptr)
            {
                meFCpy((char*)&erow+m_Database->m_ValueOffset,ptr,m_Database->m_ValueSize);
                last_pos=erow.m_LastSubKeyPos;
                last_conf=last_pos;
            }
        }
        if(confirmed)
        {
            *confirmed=last_conf;
        }
    }
    else
    {
        return GetListSize(entity,confirmed);
    }
    return last_pos;    
}

int FC_TxDB::GetListSize(FC_TxEntity *entity,int *confirmed)
{
    int row;
    FC_TxEntityStat *stat;
    
    row=m_Imports[0].FindEntity(entity);
    if(row < 0)
    {
        return 0;
    }

    stat=(FC_TxEntityStat*)m_Imports[0].m_Entities->GetRow(row);        
    
    if(confirmed)
    {
        *confirmed=stat->m_LastClearedPos;
    }
    
    return (int)(stat->m_LastPos);    
}

FC_TxImport *FC_TxDB::StartImport(FC_Buffer *lpEntities,int block,int *err)
{
    char msg[256];
    char enthex[65];
    int i,j,slot,generation,row,count;
    uint32_t pos;
    int value_len; 
    FC_TxImportRow edbImport;
    FC_TxEntityStat *lpChainEntStat;
    FC_TxEntityStat *lpEntStat;
    FC_TxEntityRow erow;
    FC_TxEntityRow *lperow;
    unsigned char* ptr;
    
    
    Dump("Before StartImport");
    generation=m_DBStat.m_LastGeneration+1;
    slot=0;
    for(i=1;i<FC_TDB_MAX_IMPORTS;i++)
    {
        if(slot == 0)
        {
            if((m_Imports+i)->m_Entities == NULL)
            {
                slot=i;
            }
        }
    }
    
    if(slot == 0)
    {
        sprintf(msg,"Couldn't start new import - %d imports already in progress",FC_TDB_MAX_IMPORTS-1);
        LogString(msg);
        *err=FC_ERR_NOT_ALLOWED;
        goto exitlbl;
    }

    sprintf(msg,"Starting new import %d with %d entities from block %d",generation, lpEntities->GetCount(),block);
    LogString(msg);
    
    (m_Imports+slot)->Init(generation,block);
    edbImport.Zero();
    edbImport.m_Generation=(m_Imports+slot)->m_ImportID;
    edbImport.m_ImportID=edbImport.m_Generation;
    edbImport.SwapPosBytes();
    *err=m_Database->m_DB->Write((char*)&edbImport+m_Database->m_KeyOffset,m_Database->m_KeySize,(char*)&edbImport+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
    edbImport.SwapPosBytes();
    if(*err)
    {
        goto exitlbl;
    }                    
    
    for(j=0;j<lpEntities->GetCount();j++)
    {
        (m_Imports+slot)->AddEntity((FC_TxEntity*)lpEntities->GetRow(j));
        count=0;
        lpEntStat=(m_Imports+slot)->GetEntity(j);
        lpEntStat->m_Flags |= FC_EFL_NOT_IN_SYNC;
        row=m_Imports->FindEntity(lpEntStat);
        if(row >= 0)                                                            // Old entity
        {
            lpChainEntStat=m_Imports->GetEntity(row);
            if( (lpChainEntStat->m_Flags & FC_EFL_NOT_IN_SYNC) == 0 )           // in-sync rows ordered by timereceived should be copied
            {
                if( (lpChainEntStat->m_Entity.m_EntityType & FC_TET_ORDERMASK) == FC_TET_TIMERECEIVED)
                {
                    lpEntStat->m_Flags -= FC_EFL_NOT_IN_SYNC;
                    erow.Zero();
                    meFCpy(&erow.m_Entity,&(lpEntStat->m_Entity),sizeof(FC_TxEntity));
                    for(pos=1;pos<=lpChainEntStat->m_LastClearedPos;pos++)      // Copy old chain wallet list ordered by timereceived 
                    {
                        erow.m_Pos=pos;
                        erow.SwapPosBytes();
                        erow.m_Generation=lpChainEntStat->m_Generation;
                        ptr=(unsigned char*)m_Database->m_DB->Read((char*)&erow+m_Database->m_KeyOffset,m_Database->m_KeySize,&value_len,0,err);
                        erow.m_Generation=(m_Imports+slot)->m_ImportID;         // Change generation when writing to DB
                        *err=m_Database->m_DB->Write((char*)&erow+m_Database->m_KeyOffset,m_Database->m_KeySize,(char*)ptr,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);                        
                        erow.SwapPosBytes();
                        count++;
                        if(*err)
                        {
                            goto exitlbl;
                        }                            
                        if(((pos+1) % 1000) == 0)
                        {
                            m_Database->m_DB->Commit(FC_OPT_DB_DATABASE_TRANSACTIONAL);                    
                        }
                    }
                    if(lpChainEntStat->m_LastClearedPos < lpChainEntStat->m_LastPos)
                    {
                        for(i=0;i<m_MemPools[0]->GetCount();i++)                // mempool transactions
                        {
                            lperow=(FC_TxEntityRow *)m_MemPools[0]->GetRow(i);       
                            if(meFCmp(&(lperow->m_Entity),&(lpEntStat->m_Entity),sizeof(FC_TxEntity)) == 0)
                            {
                                meFCpy(&erow,lperow,sizeof(FC_TxEntityRow));
                                erow.m_TempPos=0;
                                erow.m_Pos=lperow->m_TempPos;
                                erow.m_Generation=(m_Imports+slot)->m_ImportID;
                                erow.SwapPosBytes();
                                *err=m_Database->m_DB->Write((char*)&erow+m_Database->m_KeyOffset,m_Database->m_KeySize,(char*)&erow+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
                                erow.SwapPosBytes();
                                count++;
                            }
                        }
                    }      
                    lpEntStat->m_LastPos=lpChainEntStat->m_LastPos;
                    lpEntStat->m_LastClearedPos=lpChainEntStat->m_LastPos;            
                }
            }            
        }
        
        meFCpy(&edbImport.m_Entity,lpEntities->GetRow(j),sizeof(FC_TxEntity));
        sprintf_hex(enthex,edbImport.m_Entity.m_EntityID,FC_TDB_ENTITY_ID_SIZE);
        sprintf(msg,"Import: %d, Entity: (%08X, %s), transferred txs: %d",generation,edbImport.m_Entity.m_EntityType,enthex,count);
        LogString(msg);
        edbImport.m_Pos+=1;
        edbImport.m_LastPos=lpEntStat->m_LastPos;
        edbImport.m_Flags=lpEntStat->m_Flags;
        edbImport.SwapPosBytes();
        *err=m_Database->m_DB->Write((char*)&edbImport+m_Database->m_KeyOffset,m_Database->m_KeySize,(char*)&edbImport+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
        edbImport.SwapPosBytes();
        if(*err)
        {
            goto exitlbl;
        }                    
    }
    
    m_DBStat.m_LastGeneration=generation;
    
    *err=m_Database->m_DB->Write((char*)&m_DBStat+m_Database->m_KeyOffset,m_Database->m_KeySize,(char*)&m_DBStat+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
    if(*err)
    {
        goto exitlbl;
    }                            
        
    *err=m_Database->m_DB->Commit(FC_OPT_DB_DATABASE_TRANSACTIONAL);
    if(*err)
    {
        goto exitlbl;
    }

exitlbl:

    if(*err)
    {
        sprintf(msg,"Couldn't start new import, error: %d",*err);
        LogString(msg);
        m_DBStat.m_LastGeneration=generation-1; 
        if(slot)
        {
            (m_Imports+slot)->m_Entities->Clear();
            delete (m_Imports+slot)->m_Entities;
            delete (m_Imports+slot)->m_TmpEntities;
            (m_Imports+slot)->m_Block=-1;
            (m_Imports+slot)->m_ImportID=0;
        }
        return NULL;
    }

    if(m_MemPools[slot])
    {
        m_MemPools[slot]->Clear();
    }
    else
    {
        m_MemPools[slot]=new FC_Buffer;    
        m_MemPools[slot]->Initialize(FC_TDB_ENTITY_KEY_SIZE+FC_TDB_TXID_SIZE,m_Database->m_TotalSize,FC_BUF_MODE_MAP);
    }

    if(m_RawMemPools[slot])
    {
        m_RawMemPools[slot]->Clear();
    }
    else
    {
        m_RawMemPools[slot]=new FC_Buffer;    
        m_RawMemPools[slot]->Initialize(FC_TDB_TXID_SIZE,m_Database->m_TotalSize,FC_BUF_MODE_MAP);
    }
    
    
    sprintf(msg,"Import %d successfully started",generation);
    LogString(msg);        
    Dump("After StartImport");

    return m_Imports+slot;
}


FC_TxImport *FC_TxDB::FindImport(int import_id)
{
    int i;
    if(import_id == 0)
    {
        return m_Imports;
    }
    for(i=0;i<FC_TDB_MAX_IMPORTS;i++)
    {
        if((m_Imports+i)->m_Entities)
        {
            if((m_Imports+i)->m_ImportID == import_id)
            {
                return (m_Imports+i);
            }
        }
    }
    return NULL;
}

int FC_TxDB::ImportGetBlock(FC_TxImport *import)
{
    return import->m_Block;
}

int FC_TxDB::Unsubscribe(FC_Buffer* lpEntities)
{
    char msg[256];
    char enthex[65];
    int err;
    int deleted_items,row,unsubscribed_entities,subkey_list_size,value_len;
    int i,j;
    uint32_t pos;
    FC_TxImportRow edbImport;
    FC_TxEntityStat *lpent;
    FC_TxEntityRow erow;
    FC_TxEntityRow subkey_erow;
    unsigned char stream_subkey_hash160[20];
    unsigned char *ptr;
    
    err=FC_ERR_NOERROR;
    
    if(lpEntities->GetCount() == 0)
    {
        return FC_ERR_NOERROR;
    }
    
    deleted_items=0;
    unsubscribed_entities=0;
    for(j=0;j<lpEntities->GetCount();j++)                                       // Removing previous generation rows
    {
        row=m_Imports->FindEntity((FC_TxEntity*)lpEntities->GetRow(j));
        if(row >= 0)
        {
            unsubscribed_entities+=1;
            lpent=(FC_TxEntityStat*)m_Imports->m_Entities->GetRow(row);
            lpent->m_Flags |= FC_EFL_UNSUBSCRIBED;
            for(pos=1;pos<=lpent->m_LastClearedPos;pos++)
            {
                erow.Zero();
                meFCpy(&erow.m_Entity,&(lpent->m_Entity),sizeof(FC_TxEntity));
                erow.m_Generation=lpent->m_Generation;
                erow.m_Pos=pos;
                erow.SwapPosBytes();
                
                switch(lpent->m_Entity.m_EntityType & FC_TET_TYPE_MASK)
                {
                    case FC_TET_STREAM_KEY:
                    case FC_TET_STREAM_PUBLISHER:
                        ptr=(unsigned char*)m_Database->m_DB->Read((char*)&erow+m_Database->m_KeyOffset,m_Database->m_KeySize,&value_len,0,&err);
                        if(err == FC_ERR_NOERROR)
                        {
                            if(ptr)
                            {
                                meFCpy((char*)&erow+m_Database->m_ValueOffset,ptr,m_Database->m_ValueSize);
                            }
                            FC_GetCompoundHash160(&stream_subkey_hash160,&(erow.m_Entity.m_EntityID),erow.m_TxId);
                            subkey_erow.Zero();
                            meFCpy(subkey_erow.m_Entity.m_EntityID,&stream_subkey_hash160,FC_TDB_ENTITY_ID_SIZE);
                            subkey_erow.m_Entity.m_EntityType=lpent->m_Entity.m_EntityType | FC_TET_SUBKEY;
                            subkey_erow.m_Generation=erow.m_Generation;
                            GetListSize(&(subkey_erow.m_Entity),subkey_erow.m_Generation,&subkey_list_size);
                            for(i=0;i<subkey_list_size;i++)                     // If there are subkey rows in mempool, they will be stored on commit,
                                                                                // but there should be only few irrelevant rows from  old generation
                            {
                                subkey_erow.m_Pos=i+1;
                                subkey_erow.SwapPosBytes();
                                m_Database->m_DB->Delete((char*)&subkey_erow+m_Database->m_KeyOffset,m_Database->m_KeySize,FC_OPT_DB_DATABASE_TRANSACTIONAL);        
                                subkey_erow.SwapPosBytes();
                                deleted_items++;
                                if(deleted_items >= 1000)
                                {
                                    m_Database->m_DB->Commit(FC_OPT_DB_DATABASE_TRANSACTIONAL);                    
                                    deleted_items=0;
                                }
                            }
                        }
                        break;
                }
                
                m_Database->m_DB->Delete((char*)&erow+m_Database->m_KeyOffset,m_Database->m_KeySize,FC_OPT_DB_DATABASE_TRANSACTIONAL);        
                erow.SwapPosBytes();
                deleted_items++;
                
                if(deleted_items >= 1000)
                {
                    m_Database->m_DB->Commit(FC_OPT_DB_DATABASE_TRANSACTIONAL);                    
                    deleted_items=0;
                }
            }
            
            sprintf_hex(enthex,lpent->m_Entity.m_EntityID,FC_TDB_ENTITY_ID_SIZE);
            sprintf(msg,"Entity (%08X, %s) unsubscribed",lpent->m_Entity.m_EntityType,enthex);            
            LogString(msg);
        }
    }    

    i=0;
    for(j=0;j<m_Imports->m_Entities->GetCount();j++)
    {
        lpent=(FC_TxEntityStat*)m_Imports->m_Entities->GetRow(j);
        if((lpent->m_Flags & FC_EFL_UNSUBSCRIBED) == 0)
        {
            if(i != j)
            {
                lpent->m_PosInImport=i+1;
                
                meFCpy(m_Imports->m_Entities->GetRow(i),lpent,m_Imports->m_Entities->m_RowSize);
                edbImport.Zero();
                meFCpy(&(edbImport.m_Entity), &(lpent->m_Entity),sizeof(FC_TxEntity));
                edbImport.m_ImportID=0;
                edbImport.m_Generation=lpent->m_Generation;
                edbImport.m_LastPos=lpent->m_LastPos;
                edbImport.m_Pos=lpent->m_PosInImport;
                edbImport.m_LastImportedBlock=lpent->m_LastImportedBlock;
                edbImport.m_TimeAdded=lpent->m_TimeAdded;
                edbImport.m_Flags=lpent->m_Flags;
                edbImport.m_Block=m_Imports->m_Block;
                
                edbImport.SwapPosBytes();
                err=m_Database->m_DB->Write((char*)&edbImport+m_Database->m_KeyOffset,m_Database->m_KeySize,(char*)&edbImport+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
                edbImport.SwapPosBytes();
                deleted_items++;
                if(err)
                {
                    goto exitlbl;
                }                    
            }
            i++;
        }
    }    
    
    for(j=i;j<m_Imports->m_Entities->GetCount();j++)
    {
        edbImport.Zero();
        edbImport.m_ImportID=0;
        edbImport.m_Pos=j+1;

        edbImport.SwapPosBytes();
        err=m_Database->m_DB->Delete((char*)&edbImport+m_Database->m_KeyOffset,m_Database->m_KeySize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
        edbImport.SwapPosBytes();
        deleted_items++;
        if(err)
        {
            goto exitlbl;
        }                            
    }
    
    m_Imports->m_Entities->SetCount(i);
    
    if(deleted_items)
    {
        err=m_Database->m_DB->Commit(FC_OPT_DB_DATABASE_TRANSACTIONAL);                    
        deleted_items=0;                
    }    

exitlbl:
    
    return err;
}

int FC_TxDB::CompleteImport(FC_TxImport *import)
{
    char msg[256];
    char enthex[65];
    char txhex[65];
    int err;
    int i,j,row,gen,chain_entities,take_it,value_len;
    int subkey_list_size,deleted_items,mprow;
    uint32_t pos,clearedpos;
    FC_Buffer *mempool;
    FC_Buffer *rawmempool;
    
    FC_TxImportRow edbImport;
    FC_TxEntityStat *lpent;
    FC_TxEntityStat *lpdel;
    FC_TxEntityRow erow;
    FC_TxEntityRow subkey_erow;
    FC_TxEntityRow *lperow;
    FC_TxDefRow *lptxdef;
    unsigned char stream_subkey_hash160[20];
    unsigned char *ptr;
    
    
    Dump("Before CompleteImport");
    
    err=FC_ERR_NOERROR;
   
    chain_entities=m_Imports->m_Entities->GetCount();
    
/*    
    if(m_MemPools[0]->GetCount())
    {
        sprintf(msg,"CompleteImport: Import %d cannot be completed because chain mempool is not empty",import->m_ImportID);
        LogString(msg);
        err=FC_ERR_NOT_ALLOWED;
        goto exitlbl;        
    }
 */
    mempool=m_MemPools[import-m_Imports];
    rawmempool=m_RawMemPools[import-m_Imports];
    if((import->m_Block != m_Imports->m_Block) && (m_Imports->m_Block != -1))
    {
        sprintf(msg,"CompleteImport: Import %d is out-of-sync with chain, import block: %d, chain block: %d",import->m_ImportID,import->m_Block,m_Imports->m_Block);
        LogString(msg);
        err=FC_ERR_NOT_ALLOWED;
        goto exitlbl;
    }
    
    
    for(j=0;j<import->m_Entities->GetCount();j++)
    {
        lpent=(FC_TxEntityStat*)import->m_Entities->GetRow(j);
        lpdel=NULL;
        row=m_Imports->FindEntity(lpent);
        if(row >= 0)                                                            // Storing old generation info in import entity list
        {                                                                       
            lpdel=m_Imports->GetEntity(row);
            pos=lpdel->m_LastPos;
            clearedpos=lpdel->m_LastClearedPos;
            lpdel->m_LastPos=lpent->m_LastPos; 
            if(lpent->m_Flags & FC_EFL_NOT_IN_SYNC)
            {
                lpdel->m_LastPos+=pos-clearedpos;                               // New position including mempool
            }
            lpdel->m_LastClearedPos=lpent->m_LastClearedPos;                    // New position excluding mempool
            lpent->m_LastClearedPos=clearedpos;                                 // Old position, excluding mempool
//            lpent->m_LastPos=lpent->m_LastPos;                                // New position excluding mempool
            gen=lpdel->m_Generation;
            lpdel->m_Generation=lpent->m_Generation;                            // Swapping generation
            lpent->m_Generation=gen;
        }
        else
        {
            row=m_Imports->m_Entities->GetCount();
            m_Imports->AddEntity(lpent);
            lpdel=m_Imports->GetEntity(row);
        }
        
        lpdel->m_LastImportedBlock=import->m_Block;
        edbImport.m_Flags=lpdel->m_Flags;
        if(lpdel->m_Flags & FC_EFL_NOT_IN_SYNC)
        {
            lpdel->m_Flags-=FC_EFL_NOT_IN_SYNC;
        }
        
        
        edbImport.Zero();                                                       // Storing new entity    
        edbImport.m_ImportID=0;
        meFCpy(&edbImport.m_Entity,&(lpent->m_Entity),sizeof(FC_TxEntity));
        edbImport.m_Generation=lpdel->m_Generation;
        edbImport.m_Block=import->m_Block;
        edbImport.m_Pos=lpdel->m_PosInImport;
        edbImport.m_LastPos=lpdel->m_LastPos;
        edbImport.m_LastImportedBlock=import->m_Block;
        edbImport.m_TimeAdded=lpdel->m_TimeAdded;
        edbImport.m_Flags=lpdel->m_Flags;
        sprintf_hex(enthex,edbImport.m_Entity.m_EntityID,FC_TDB_ENTITY_ID_SIZE);
        sprintf(msg,"Import: %d, Transferring entity (%08X, %s) to the chain, txs: %d",import->m_ImportID,edbImport.m_Entity.m_EntityType,enthex,lpdel->m_LastPos);
        LogString(msg);
        edbImport.SwapPosBytes();
        err=m_Database->m_DB->Write((char*)&edbImport+m_Database->m_KeyOffset,m_Database->m_KeySize,(char*)&edbImport+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
        edbImport.SwapPosBytes();
        if(err)
        {
            goto exitlbl;
        }                    
        
        edbImport.Zero();                                                       // Deleting old entity
        edbImport.m_ImportID=import->m_ImportID;
        edbImport.m_Pos=lpent->m_PosInImport;
        edbImport.SwapPosBytes();
        err=m_Database->m_DB->Delete((char*)&edbImport+m_Database->m_KeyOffset,m_Database->m_KeySize,FC_OPT_DB_DATABASE_TRANSACTIONAL);        
        edbImport.SwapPosBytes();
        if(err)
        {
            goto exitlbl;
        }                    
    }

    edbImport.Zero();                                                           // Import block update
    edbImport.m_Block=import->m_Block;
    edbImport.m_ImportID=0;
    edbImport.SwapPosBytes();
    err=m_Database->m_DB->Write((char*)&edbImport+m_Database->m_KeyOffset,m_Database->m_KeySize,(char*)&edbImport+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
    edbImport.SwapPosBytes();
    if(err)
    {
        goto exitlbl;
    }                    
    
    m_DBStat.m_Block=import->m_Block;                                                 // DB stats
    err=m_Database->m_DB->Write((char*)&m_DBStat+m_Database->m_KeyOffset,m_Database->m_KeySize,(char*)&m_DBStat+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
    if(err)
    {
        goto exitlbl;
    }                            

    
    edbImport.Zero();                                                           // Deleting old import
    edbImport.m_ImportID=import->m_ImportID;
    edbImport.m_Pos=0;
    edbImport.SwapPosBytes();
    err=m_Database->m_DB->Delete((char*)&edbImport+m_Database->m_KeyOffset,m_Database->m_KeySize,FC_OPT_DB_DATABASE_TRANSACTIONAL);        
    edbImport.SwapPosBytes();
    if(err)
    {
        goto exitlbl;
    }                

    
    err=m_Database->m_DB->Commit(FC_OPT_DB_DATABASE_TRANSACTIONAL);
    if(err)
    {
        goto exitlbl;
    }

    m_Imports->m_Block=import->m_Block;
    
                                                                                // Cleanup below this point, cannot fail
    j=0;
    for(i=0;i<m_MemPools[0]->GetCount();i++)
    {
        take_it=1;
        lperow=(FC_TxEntityRow*)m_MemPools[0]->GetRow(i);        
        row=import->FindEntity(&(lperow->m_Entity));
        if(row >= 0)                                                            // Shifting mempool transactions, in-sync transactions are removed
        {
            lpent=import->GetEntity(row);
            lperow->m_TempPos+=lpent->m_LastPos-lpent->m_LastClearedPos;
            lperow->m_Generation=import->m_ImportID;
            if( (lpent->m_Flags & FC_EFL_NOT_IN_SYNC) == 0 )
            {
                mprow=rawmempool->Seek(lperow->m_TxId);
                if(mprow >= 0)
                {
                    lptxdef=(FC_TxDefRow*)rawmempool->GetRow(mprow);
                    lptxdef->m_Flags |= 0x80000000;                             // FC_TFL_IMPOSSIBLE
                }
                take_it=0;
            }
        }
        if(take_it)
        {
            meFCpy(m_MemPools[0]->GetRow(j),m_MemPools[0]->GetRow(i),sizeof(FC_TxEntityRow));
            j++;            
        }
    }
    m_MemPools[0]->SetCount(j);    
    
    for(i=0;i<mempool->GetCount();i++)                                          // Copying mempool transactions
    {
        lperow=(FC_TxEntityRow *)mempool->GetRow(i);               
        if( ((lperow->m_Entity.m_EntityType & FC_TET_TYPE_MASK) == FC_TET_PUBKEY_ADDRESS) || 
            ((lperow->m_Entity.m_EntityType & FC_TET_TYPE_MASK) == FC_TET_SCRIPT_ADDRESS))           
        {
            mprow=rawmempool->Seek(lperow->m_TxId);                             // Will be added later when block is processed
            if(mprow >= 0)
            {
                lptxdef=(FC_TxDefRow*)rawmempool->GetRow(mprow);
                lptxdef->m_Flags |= 0x80000000;                                 // FC_TFL_IMPOSSIBLE
            }
        }
        sprintf_hex(txhex,lperow->m_TxId,FC_TDB_TXID_SIZE);    
        sprintf_hex(enthex,lperow->m_Entity.m_EntityID,FC_TDB_ENTITY_ID_SIZE);
        sprintf(msg,"Import: %d, Transferring mempool: tx: %s, entity: (%08X, %s)",import->m_ImportID,txhex,lperow->m_Entity.m_EntityType,enthex);
        LogString(msg);
        m_MemPools[0]->Add(lperow,(unsigned char*)lperow+FC_TDB_ENTITY_KEY_SIZE+FC_TDB_TXID_SIZE);        
    }
    
    for(i=0;i<rawmempool->GetCount();i++)                                       // Copying rawmempool transactions
    {
        lptxdef=(FC_TxDefRow*)rawmempool->GetRow(i);        
        if((lptxdef->m_Flags & 0x80000000) == 0)
        {
            mprow=m_RawMemPools[0]->Seek(lptxdef->m_TxId);
            if(mprow < 0)
            {
                sprintf_hex(txhex,lptxdef->m_TxId,FC_TDB_TXID_SIZE);    
                sprintf(msg,"Import: %d, Transferring rawmempool: tx: %s",import->m_ImportID,txhex);
                LogString(msg);
                m_RawMemPools[0]->Add(lptxdef,(unsigned char*)lptxdef+FC_TDB_TXID_SIZE);                    
            }            
        }
    }
       
    sprintf(msg,"CompleteImport: Successfully updated chain after import %d, removing old records",import->m_ImportID);
    LogString(msg);
    
    deleted_items=0;
    for(j=0;j<import->m_Entities->GetCount();j++)                               // Removing previous generation rows
    {
        lpent=(FC_TxEntityStat*)import->m_Entities->GetRow(j);
        if(lpent->m_Generation != import->m_ImportID)                           // generation was changed when storing old chain entity
        {
            for(pos=1;pos<=lpent->m_LastClearedPos;pos++)
            {
                erow.Zero();
                meFCpy(&erow.m_Entity,&(lpent->m_Entity),sizeof(FC_TxEntity));
                erow.m_Generation=lpent->m_Generation;
                erow.m_Pos=pos;
                erow.SwapPosBytes();
                
                switch(lpent->m_Entity.m_EntityType & FC_TET_TYPE_MASK)
                {
                    case FC_TET_STREAM_KEY:
                    case FC_TET_STREAM_PUBLISHER:
                        ptr=(unsigned char*)m_Database->m_DB->Read((char*)&erow+m_Database->m_KeyOffset,m_Database->m_KeySize,&value_len,0,&err);
                        if(err == FC_ERR_NOERROR)
                        {
                            if(ptr)
                            {
                                meFCpy((char*)&erow+m_Database->m_ValueOffset,ptr,m_Database->m_ValueSize);
                            }
                            FC_GetCompoundHash160(&stream_subkey_hash160,&(erow.m_Entity.m_EntityID),erow.m_TxId);
                            subkey_erow.Zero();
                            meFCpy(subkey_erow.m_Entity.m_EntityID,&stream_subkey_hash160,FC_TDB_ENTITY_ID_SIZE);
                            subkey_erow.m_Entity.m_EntityType=lpent->m_Entity.m_EntityType | FC_TET_SUBKEY;
                            subkey_erow.m_Generation=erow.m_Generation;
                            GetListSize(&(subkey_erow.m_Entity),subkey_erow.m_Generation,&subkey_list_size);
                            for(i=0;i<subkey_list_size;i++)                     // If there are subkey rows in mempool, they will be stored on commit,
                                                                                // but there should be only few irrelevant rows from  old generation
                            {
                                subkey_erow.m_Pos=i+1;
                                subkey_erow.SwapPosBytes();
                                m_Database->m_DB->Delete((char*)&subkey_erow+m_Database->m_KeyOffset,m_Database->m_KeySize,FC_OPT_DB_DATABASE_TRANSACTIONAL);        
                                subkey_erow.SwapPosBytes();
                                deleted_items++;
                                if(deleted_items >= 1000)
                                {
                                    m_Database->m_DB->Commit(FC_OPT_DB_DATABASE_TRANSACTIONAL);                    
                                    deleted_items=0;
                                }
                            }
                        }
                        break;
                }
                
                m_Database->m_DB->Delete((char*)&erow+m_Database->m_KeyOffset,m_Database->m_KeySize,FC_OPT_DB_DATABASE_TRANSACTIONAL);        
                erow.SwapPosBytes();
                deleted_items++;
                
                if(deleted_items >= 1000)
                {
                    m_Database->m_DB->Commit(FC_OPT_DB_DATABASE_TRANSACTIONAL);                    
                    deleted_items=0;
                }
            }
            if(deleted_items)
            {
                m_Database->m_DB->Commit(FC_OPT_DB_DATABASE_TRANSACTIONAL);                    
                deleted_items=0;                
            }
        }
    }    
    
    
exitlbl:

    if(err)
    {
        for(j=0;j<chain_entities;j++)
        {
            lpdel=(FC_TxEntityStat*)m_Imports->m_Entities->GetRow(j);           // Restoring chain entities in case of failure (swapping back)
            if(lpdel->m_Generation == import->m_ImportID)
            {
                row=import->FindEntity(lpdel);
                if(row >= 0)
                {
                    lpent=import->GetEntity(row);
                    pos=lpdel->m_LastPos;
                    lpdel->m_LastPos=lpent->m_LastPos;
                    lpdel->m_LastClearedPos=lpent->m_LastClearedPos;
                    lpent->m_LastPos=pos;
                    lpent->m_LastClearedPos=pos;
                    gen=lpdel->m_Generation;
                    lpdel->m_Generation=lpent->m_Generation;
                    lpent->m_Generation=gen;
                    lpent->m_Flags |= FC_EFL_NOT_IN_SYNC;
                }
            }
        }
        m_Imports->m_Entities->SetCount(chain_entities);        
        sprintf(msg,"Could not complete import %d, error: %d",import->m_ImportID,err);        
        LogString(msg);
        
    }
    else
    {
        sprintf(msg,"CompleteImport: Import %d completed",import->m_ImportID);
        LogString(msg);
        m_MemPools[import-m_Imports]->Clear();
        m_RawMemPools[import-m_Imports]->Clear();
        delete import->m_Entities;
        delete import->m_TmpEntities;
        import->m_Block=-1;
        import->m_ImportID=0;
        import->m_Entities=NULL;
        import->m_TmpEntities=NULL;
    }

    Dump("After CompleteImport");

    return err;
}

int FC_TxDB::DropImport(FC_TxImport *import)
{
    char msg[256];
    int err;
    int j,commit_required;
    uint32_t pos;
    FC_TxImportRow edbImport;
    FC_TxEntityStat *lpent;
    FC_TxEntityRow erow;
    
    Dump("Before DropImport");
    
    err=FC_ERR_NOERROR;

    for(j=0;j<import->m_Entities->GetCount();j++)
    {
        lpent=(FC_TxEntityStat*)import->m_Entities->GetRow(j);
        edbImport.Zero();
        edbImport.m_ImportID=import->m_ImportID;
        edbImport.m_Pos=lpent->m_PosInImport;
        edbImport.SwapPosBytes();
        err=m_Database->m_DB->Delete((char*)&edbImport+m_Database->m_KeyOffset,m_Database->m_KeySize,FC_OPT_DB_DATABASE_TRANSACTIONAL);        
        edbImport.SwapPosBytes();
        if(err)
        {
            goto exitlbl;
        }                    
    }
    
    edbImport.Zero();
    edbImport.m_ImportID=import->m_ImportID;
    edbImport.m_Pos=0;
    edbImport.SwapPosBytes();
    err=m_Database->m_DB->Delete((char*)&edbImport+m_Database->m_KeyOffset,m_Database->m_KeySize,FC_OPT_DB_DATABASE_TRANSACTIONAL);        
    edbImport.SwapPosBytes();
    if(err)
    {
        goto exitlbl;
    }                
    
    err=m_Database->m_DB->Commit(FC_OPT_DB_DATABASE_TRANSACTIONAL);
    if(err)
    {
        goto exitlbl;
    }
    
    sprintf(msg,"DropImport: Successfully dropped Import %d, removing old records",import->m_ImportID);
    LogString(msg);
    
    commit_required=0;
    for(j=0;j<import->m_Entities->GetCount();j++)
    {
        lpent=(FC_TxEntityStat*)import->m_Entities->GetRow(j);
        for(pos=1;pos<=lpent->m_LastPos;pos++)
        {
            erow.Zero();
            meFCpy(&erow.m_Entity,&(lpent->m_Entity),sizeof(FC_TxEntity));
            erow.m_Generation=lpent->m_Generation;
            erow.m_Pos=pos;
            erow.SwapPosBytes();
            m_Database->m_DB->Delete((char*)&erow+m_Database->m_KeyOffset,m_Database->m_KeySize,FC_OPT_DB_DATABASE_TRANSACTIONAL);        
            erow.SwapPosBytes();
            commit_required=1;
            if(((pos+1) % 1000) == 0)
            {
                m_Database->m_DB->Commit(FC_OPT_DB_DATABASE_TRANSACTIONAL);                    
                commit_required=0;
            }
        }
        if(commit_required)
        {
            m_Database->m_DB->Commit(FC_OPT_DB_DATABASE_TRANSACTIONAL);                    
            commit_required=0;                
        }
    }    
    
    
exitlbl:

    if(err)
    {
        sprintf(msg,"Could not drop import %d, error: %d",import->m_ImportID,err);        
        LogString(msg);        
    }
    else
    {
        sprintf(msg,"DropImport: Import %d dropped",import->m_ImportID);
        LogString(msg);
        delete import->m_Entities;
        delete import->m_TmpEntities;
        import->m_Block=-1;
        import->m_ImportID=0;
        import->m_Entities=NULL;
        import->m_TmpEntities=NULL;
    }

    Dump("After DropImport");
    return err;    
}
