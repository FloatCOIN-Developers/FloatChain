// Copyright (c) 2014-2017 Coin Sciences Ltd
// FloatChain code distributed under the GPLv3 license, see COPYING file.

#include "floatchain/floatchain.h"

unsigned char null_entity[FC_PLS_SIZE_ENTITY];
unsigned char upgrade_entity[FC_PLS_SIZE_ENTITY];
unsigned char adminminerlist_entity[FC_PLS_SIZE_ENTITY];

int FC_IsNullEntity(const void* lpEntity)
{
    if(lpEntity == NULL)
    {
        return 1;
    }
    if(meFCmp(lpEntity,null_entity,FC_PLS_SIZE_ENTITY) == 0)
    {
        return 1;        
    }
    return 0;
}

int FC_IsUpgradeEntity(const void* lpEntity)
{
    if(meFCmp(lpEntity,upgrade_entity,FC_PLS_SIZE_ENTITY) == 0)
    {
        return 1;        
    }
    return 0;
}


void FC_PermissionDBRow::Zero()
{
    memset(this,0,sizeof(FC_PermissionDBRow));
}

void FC_BlockMinerDBRow::Zero()
{
    memset(this,0,sizeof(FC_BlockMinerDBRow));    
}

void FC_AdminMinerGrantDBRow::Zero()
{
    memset(this,0,sizeof(FC_AdminMinerGrantDBRow));        
}

int FC_PermissionDBRow::InBlockRange(uint32_t block)
{
    if((block+1) >= m_BlockFrom)
    {
        if((block+1) < m_BlockTo)
        {
            return 1;
        }        
    }
    return 0;
}


void FC_PermissionLedgerRow::Zero()
{
    memset(this,0,sizeof(FC_PermissionLedgerRow));
}

void FC_BlockLedgerRow::Zero()
{
    memset(this,0,sizeof(FC_BlockLedgerRow));    
}

void FC_PermissionDetails::Zero()
{
    memset(this,0,sizeof(FC_PermissionDetails));    
}


/** Set initial database object values */

void FC_PermissionDB::Zero()
{
    m_FileName[0]=0;
    m_DB=0;
    m_KeyOffset=0;
    m_KeySize=FC_PLS_SIZE_ENTITY+24;                                            // Entity,address,type
    m_ValueOffset=56;
    m_ValueSize=24;    
    m_TotalSize=m_KeySize+m_ValueSize;
}

/** Set database file name */

void FC_PermissionDB::SetName(const char* name)
{
    FC_GetFullFileName(name,"permissions",".db",FC_FOM_RELATIVE_TO_DATADIR | FC_FOM_CREATE_DIR,m_FileName);
}

/** Open database */

int FC_PermissionDB::Open() 
{
    
    m_DB=new FC_Database;
    
    m_DB->SetOption("KeySize",0,m_KeySize);
    m_DB->SetOption("ValueSize",0,m_ValueSize);
        
    return m_DB->Open(m_FileName,FC_OPT_DB_DATABASE_CREATE_IF_MISSING | FC_OPT_DB_DATABASE_TRANSACTIONAL | FC_OPT_DB_DATABASE_LEVELDB);
}

/** Close database */

int FC_PermissionDB::Close()
{
    if(m_DB)
    {
        m_DB->Close();
        delete m_DB;    
        m_DB=NULL;
    }
    return 0;
}
    
/** Set initial ledger values */

void FC_PermissionLedger::Zero()
{
    m_FileName[0]=0;
    m_FileHan=0;
    m_KeyOffset=0;
    m_KeySize=FC_PLS_SIZE_ENTITY+32;                                            // Entity,address,type,prevrow
    m_ValueOffset=FC_PLS_SIZE_ENTITY+32;
    m_ValueSize=64;    
    m_TotalSize=m_KeySize+m_ValueSize;
}

/** Set ledger file name */

void FC_PermissionLedger::SetName(const char* name)
{
    FC_GetFullFileName(name,"permissions",".dat",FC_FOM_RELATIVE_TO_DATADIR,m_FileName);
}

/** Open ledger file */

int FC_PermissionLedger::Open()
{
    if(m_FileHan>0)
    {
        return m_FileHan;
    }
    
    m_FileHan=open(m_FileName,_O_BINARY | O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    return m_FileHan;            
}

void FC_PermissionLedger::Flush()
{
    if(m_FileHan>0)
    {
        __US_FlushFile(m_FileHan);
    }    
}



/** Close ledger file */

int FC_PermissionLedger::Close()
{
    if(m_FileHan>0)
    {
        close(m_FileHan);
    }    
    m_FileHan=0;
    return 0;    
}

/** Returns ledger row */

int FC_PermissionLedger::GetRow(uint64_t RowID, FC_PermissionLedgerRow* row)
{
    int64_t off;
    off=RowID*m_TotalSize;
    
    if(m_FileHan<=0)
    {
        return FC_ERR_INTERNAL_ERROR;
    }
    if(lseek64(m_FileHan,off,SEEK_SET) != off)
    {
        return FC_ERR_NOT_FOUND;
    }
    
    row->Zero();
    
    if(read(m_FileHan,(unsigned char*)row+m_KeyOffset,m_TotalSize) != m_TotalSize)
    {
        return FC_ERR_FILE_READ_ERROR;
    }
    
    return FC_ERR_NOERROR;
    
}

/** Returns ledger size*/

uint64_t FC_PermissionLedger::GetSize()
{
    if(m_FileHan<=0)
    {
        return 0;
    }
    
    return lseek64(m_FileHan,0,SEEK_END);    
}

/** Writes row into ledger without specifying position */

int FC_PermissionLedger::WriteRow(FC_PermissionLedgerRow* row)
{
    if(m_FileHan<=0)
    {
        return FC_ERR_INTERNAL_ERROR;
    }
    
    if(write(m_FileHan,(unsigned char*)row+m_KeyOffset,m_TotalSize) != m_TotalSize)
    {
        return FC_ERR_INTERNAL_ERROR;
    }
    
    return FC_ERR_NOERROR;
}

/** Writes row into ledger to specified position */

int FC_PermissionLedger::SetRow(uint64_t RowID, FC_PermissionLedgerRow* row) 
{
    if(m_FileHan<=0)
    {
        return FC_ERR_INTERNAL_ERROR;
    }
    
    int64_t off;
    off=RowID*m_TotalSize;
    
    if(lseek64(m_FileHan,off,SEEK_SET) != off)
    {
        return FC_ERR_INTERNAL_ERROR;
    }
   
    return WriteRow(row);
}

/** Setting initial values */

int FC_Permissions::Zero()
{
    m_Database=NULL;
    m_Ledger=NULL;
    m_MemPool = NULL;
    m_TmpPool = NULL;
    m_CopiedMemPool=NULL;
    m_Name[0]=0x00; 
    m_LogFileName[0]=0x00;
    m_Block=-1;    
    m_Row=0;
    m_AdminCount=0;
    m_MinerCount=0;
//    m_DBRowCount=0;            
    m_CheckPointRow=0;
    m_CheckPointAdminCount=0;
    m_CheckPointMinerCount=0;
    m_CheckPointMemPoolSize=0;
    m_CopiedBlock=0;
    m_CopiedRow=0;
    m_ForkBlock=0;
    m_CopiedAdminCount=0;
    m_CopiedMinerCount=0;
    m_ClearedAdminCount=0;
    m_ClearedMinerCount=0;
    m_ClearedMinerCountForMinerVerification=0;
    m_TmpSavedAdminCount=0;
    m_TmpSavedMinerCount=0;

    m_Semaphore=NULL;
    m_LockedBy=0;
    
    m_MempoolPermissions=NULL;
    m_MempoolPermissionsToReplay=NULL;
    m_CheckForMempoolFlag=0;
    
    return FC_ERR_NOERROR;
}

/** Initialization */

int FC_Permissions::Initialize(const char *name,int mode)
{
    int err,value_len,take_it;    
    int32_t pdbBlock,pldBlock;
    uint64_t pdbLastRow,pldLastRow,this_row;
    uint64_t ledger_size;
    FC_BlockLedgerRow pldBlockRow;
    char block_row_addr[32];
    char msg[256];
    
    unsigned char *ptr;

    FC_PermissionDBRow pdbRow;
    FC_PermissionLedgerRow pldRow;
        
    strcpy(m_Name,name);
    memset(null_entity,0,FC_PLS_SIZE_ENTITY);
    memset(upgrade_entity,0,FC_PLS_SIZE_ENTITY);
    upgrade_entity[0]=FC_PSE_UPGRADE;
    memset(adminminerlist_entity,0,FC_PLS_SIZE_ENTITY);
    adminminerlist_entity[0]=FC_PSE_ADMINMINERLIST;
    
    err=FC_ERR_NOERROR;
    
    m_Ledger=new FC_PermissionLedger;
    m_Database=new FC_PermissionDB;
     
    m_Ledger->SetName(name);
    m_Database->SetName(name);
    FC_GetFullFileName(name,"permissions",".log",FC_FOM_RELATIVE_TO_DATADIR,m_LogFileName);
    
    err=m_Database->Open();
    
    if(err)
    {
        LogString("Initialize: Cannot open database");
        return err;
    }
    
    pdbBlock=-1;    
    pdbLastRow=1;
 
    pdbRow.Zero();
    
    ptr=(unsigned char*)m_Database->m_DB->Read((char*)&pdbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,&value_len,0,&err);
    if(err)
    {
        LogString("Initialize: Cannot read from database");
        return err;
    }

    if(ptr)                                                                     
    {
        meFCpy((char*)&pdbRow+m_Database->m_ValueOffset,ptr,m_Database->m_ValueSize);
        pdbBlock=pdbRow.m_BlockTo;
        pdbLastRow=pdbRow.m_LedgerRow;
    }
    else
    {
        pdbRow.Zero();
        pdbRow.m_BlockTo=(uint32_t)pdbBlock;
        pdbRow.m_LedgerRow=pdbLastRow;
        
        err=m_Database->m_DB->Write((char*)&pdbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,(char*)&pdbRow+m_Database->m_ValueOffset,m_Database->m_ValueSize,0);
        if(err)
        {
            return err;
        }
        
        err=m_Database->m_DB->Commit(0);//FC_OPT_DB_DATABASE_TRANSACTIONAL
        if(err)
        {
            return err;
        }
    }
    
    m_TmpPool=new FC_Buffer;
    m_TmpPool->Initialize(m_Database->m_ValueOffset,sizeof(FC_PermissionLedgerRow),FC_BUF_MODE_MAP);
        
    m_MemPool=new FC_Buffer;
    
    err=m_MemPool->Initialize(m_Ledger->m_KeySize,m_Ledger->m_TotalSize,FC_BUF_MODE_MAP);
    
    m_CopiedMemPool=new FC_Buffer;
    
    err=m_CopiedMemPool->Initialize(m_Ledger->m_KeySize,m_Ledger->m_TotalSize,0);
    
    m_MempoolPermissions=new FC_Buffer;
    
    err=m_MempoolPermissions->Initialize(sizeof(FC_MempoolPermissionRow),sizeof(FC_MempoolPermissionRow),0);

    m_MempoolPermissionsToReplay=new FC_Buffer;
    
    err=m_MempoolPermissionsToReplay->Initialize(sizeof(FC_MempoolPermissionRow),sizeof(FC_MempoolPermissionRow),0);
    
    pldBlock=-1;
    pldLastRow=1;
    
    if(m_Ledger->Open() <= 0)
    {
        return FC_ERR_DBOPEN_ERROR;
    }
    
    if(m_Ledger->GetRow(0,&pldRow) == 0)
    {
        pldBlock=pldRow.m_BlockTo;
        pldLastRow=pldRow.m_PrevRow;
    }
    else
    {
        pldRow.Zero();
        pldRow.m_BlockTo=(uint32_t)pldBlock;
        pldRow.m_PrevRow=pldLastRow;
        m_Ledger->SetRow(0,&pldRow);
    }        
    
    ledger_size=m_Ledger->GetSize()/m_Ledger->m_TotalSize;
    m_Row=ledger_size;

    m_Ledger->Close();
    if(pdbBlock < pldBlock)
    {
        sprintf(msg,"Initialize: Database corrupted, blocks, Ledger: %d, DB: %d, trying to repair.",pldBlock,pdbBlock);
        LogString(msg);
        if(m_Ledger->Open() <= 0)
        {
            LogString("Error: Repair: couldn't open ledger");
            return FC_ERR_DBOPEN_ERROR;
        }
    
        this_row=m_Row-1;
        take_it=1;
        if(this_row == 0)
        {
            take_it=0;
        }
    
        while(take_it && (this_row>0))
        {
            m_Ledger->GetRow(this_row,&pldRow);
        
            if((int32_t)pldRow.m_BlockReceived <= pdbBlock)
            {
                take_it=0;
            }
            if(take_it)
            {
                this_row--;
            }            
        }
        
        this_row++;

        m_Ledger->GetRow(0,&pldRow);

        pldRow.m_BlockTo=pdbBlock;
        pldRow.m_PrevRow=this_row;
        m_Ledger->SetRow(0,&pldRow);        

        m_Ledger->Close();  

        pldBlock=pdbBlock;
        pldLastRow=this_row;        
    }
    
    if(pdbBlock != pldBlock)
    {        
        sprintf(msg,"Initialize: Database corrupted, blocks, Ledger: %d, DB: %d",pldBlock,pdbBlock);
        LogString(msg);
        return FC_ERR_CORRUPTED;
    }

    if(pdbLastRow != pldLastRow)
    {
        sprintf(msg,"Initialize: Database corrupted, rows, Ledger: %ld, DB: %ld",pldLastRow,pdbLastRow);
        LogString(msg);
        return FC_ERR_CORRUPTED;
    }

    if(pldLastRow > ledger_size)
    {
        sprintf(msg,"Initialize: Database corrupted, size, last row: %ld, file size: %ld",pldLastRow,ledger_size);
        LogString(msg);
        return FC_ERR_CORRUPTED;        
    }
    
    m_Block=pdbBlock;
    m_Row=pdbLastRow;            

    err=UpdateCounts();
    if(err)
    {
        LogString("Error: Cannot initialize AdminMiner list");            
        return FC_ERR_DBOPEN_ERROR;            
    }
    
    m_ClearedAdminCount=m_AdminCount;
    m_ClearedMinerCount=m_MinerCount;
    m_ClearedMinerCountForMinerVerification=m_MinerCount;

    if(m_Ledger->Open() <= 0)
    {
        LogString("Error: Couldn't open ledger");
        return FC_ERR_DBOPEN_ERROR;
    }
    
    if(m_Row-1 > 0)                                                             // Last row can contain wrong admin/mine count in case of hard crash
    {
        sprintf(block_row_addr,"Block %08X row",m_Block);
        m_Ledger->GetRow(m_Row-1,(FC_PermissionLedgerRow*)&pldBlockRow);        
        if(meFCmp((char*)pldBlockRow.m_Address,block_row_addr,strlen(block_row_addr)))
        {
            m_Ledger->Close();  
            LogString("Error: Last ledger row doesn't contain block information");
            return FC_ERR_DBOPEN_ERROR;            
        }
        pldBlockRow.m_AdminCount=m_AdminCount;
        pldBlockRow.m_MinerCount=m_MinerCount;
        m_Ledger->SetRow(m_Row-1,(FC_PermissionLedgerRow*)&pldBlockRow);
        m_Ledger->GetRow(m_Row-2,(FC_PermissionLedgerRow*)&pldBlockRow);        
        pldBlockRow.m_AdminCount=m_AdminCount;
        pldBlockRow.m_MinerCount=m_MinerCount;
        m_Ledger->SetRow(m_Row-2,(FC_PermissionLedgerRow*)&pldBlockRow);
        m_Ledger->Close();  
    }
    
    
    m_Semaphore=__US_SeFCreate();
    if(m_Semaphore == NULL)
    {
        LogString("Initialize: Cannot initialize semaphore");
        return FC_ERR_INTERNAL_ERROR;
    }

    sprintf(msg,"Initialized: Admin count: %d, Miner count: %d, ledger rows: %ld",m_AdminCount,m_MinerCount,m_Row);
    LogString(msg);
    return FC_ERR_NOERROR;
}

void FC_Permissions::MempoolPermissionsCopy()
{
    m_MempoolPermissionsToReplay->Clear();
    if(m_MempoolPermissions->GetCount())
    {
        m_MempoolPermissionsToReplay->SetCount(m_MempoolPermissions->GetCount());
        meFCpy(m_MempoolPermissionsToReplay->GetRow(0),m_MempoolPermissions->GetRow(0),m_MempoolPermissions->m_Size);
        m_MempoolPermissions->Clear();
    }
}

int FC_Permissions::MempoolPermissionsCheck(int from, int to)
{
    int i;
    FC_MempoolPermissionRow *row;
    for(i=from;i<to;i++)
    {
        row=(FC_MempoolPermissionRow*)m_MempoolPermissionsToReplay->GetRow(i);
        switch(row->m_Type)
        {
            case FC_PTP_SEND:
                if(CanSend(row->m_Entity,row->m_Address) == 0)
                {
                    return 0;
                }
                break;
            case FC_PTP_RECEIVE:
                if(CanReceive(row->m_Entity,row->m_Address) == 0)
                {
                    return 0;
                }
                break;
            case FC_PTP_WRITE:
                if(CanWrite(row->m_Entity,row->m_Address) == 0)
                {
                    return 0;
                }
                break;
        }
    }
    
    for(i=from;i<to;i++)
    {
        m_MempoolPermissions->Add(m_MempoolPermissionsToReplay->GetRow(i));
    }
    
    return 1;
}


/** Logging message */

void FC_Permissions::LogString(const char *message)
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

/** Freeing objects */

int FC_Permissions::Destroy()
{
    int removefiles;
    FC_PermissionDBRow pdbRow;
    
    removefiles=0;
    if(m_Ledger)
    {
        m_Ledger->Open();
        if(m_Ledger->GetSize() == m_Ledger->m_TotalSize)
        {
            removefiles=1;
        }
        m_Ledger->Close();
    }
    
    if(m_Semaphore)
    {
        __US_SemDestroy(m_Semaphore);
    }
    
    if(m_Database)
    {
        if(removefiles)
        {
            pdbRow.Zero();
            m_Database->m_DB->Delete((char*)&pdbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,0);
            m_Database->m_DB->Commit(0);
        }
        m_Database->Close();        
        delete m_Database;
    }
    
    if(m_Ledger)
    {
        if(removefiles)
        {
            m_Ledger->Close();
            remove(m_Ledger->m_FileName);
        }
        delete m_Ledger;
    }
    
    if(m_MemPool)
    {
        delete m_MemPool;
    }  
    if(m_TmpPool)
    {
        delete m_TmpPool;
    }  
    if(m_CopiedMemPool)
    {
        delete m_CopiedMemPool;        
    }

    if(m_MempoolPermissions)
    {
        delete m_MempoolPermissions;
    }
    
    if(m_MempoolPermissionsToReplay)
    {
        delete m_MempoolPermissionsToReplay;
    }
    
    Zero();
    
    return FC_ERR_NOERROR;
}
  
/** Locking permissions object */

void FC_Permissions::Lock(int write_mode)
{        
    uint64_t this_thread;
    this_thread=__US_ThreadID();
    
    if(this_thread == m_LockedBy)
    {
        LogString("Secondary lock!!!");
        return;
    }
    
    __US_SemWait(m_Semaphore); 
    m_LockedBy=this_thread;
}

/** Unlocking permissions object */

void FC_Permissions::UnLock()
{    
    m_LockedBy=0;
    __US_SemPost(m_Semaphore);
}

int FC_MeFCmpCheckSize(const void *s1,const char *s2,size_t s1_size)
{
    if(strlen(s2) != s1_size)
    {
        return 1;
    }
    return meFCmp(s1,s2,s1_size);
}

uint32_t FC_Permissions::GetPossiblePermissionTypes(uint32_t entity_type)
{
    uint32_t full_type;
    
    full_type=0;
    switch(entity_type)
    {
        case FC_ENT_TYPE_ASSET:
            full_type = FC_PTP_ISSUE | FC_PTP_ADMIN;
            break;
        case FC_ENT_TYPE_STREAM:
            full_type = FC_PTP_WRITE | FC_PTP_ACTIVATE | FC_PTP_ADMIN;
            break;
        case FC_ENT_TYPE_NONE:
            full_type = FC_PTP_GLOBAL_ALL;
            if(FC_gState->m_Features->Streams() == 0)
            {
                full_type-=FC_PTP_CREATE;
            }        
            break;
        default:
            if(FC_gState->m_Features->FixedIn10007())
            {
                if(entity_type <= FC_ENT_TYPE_STREAM_MAX)
                {
                    full_type = FC_PTP_WRITE | FC_PTP_ACTIVATE | FC_PTP_ADMIN;
                }
            }
            break;
    }
    return full_type;
}


/** Return ORed FC_PTP_ constants by textual value */

uint32_t FC_Permissions::GetPermissionType(const char *str,int entity_type)
{
    uint32_t result,perm_type,full_type;
    char* ptr;
    char* start;
    char* ptrEnd;
    char c;
    
    full_type=GetPossiblePermissionTypes(entity_type);
    
    ptr=(char*)str;
    ptrEnd=ptr+strlen(ptr);
    start=ptr;
    
    result=0;
    while(ptr<=ptrEnd)
    {
        c=*ptr;
        if( (c == ',') || (c ==0x00))
        {
            if(ptr > start)
            {
                perm_type=0;
                if((FC_MeFCmpCheckSize(start,"all",      ptr-start) == 0) || 
                   (FC_MeFCmpCheckSize(start,"*",      ptr-start) == 0))
                {
                    perm_type=full_type;
                }
                if(FC_MeFCmpCheckSize(start,"connect",  ptr-start) == 0)perm_type = FC_PTP_CONNECT;
                if(FC_MeFCmpCheckSize(start,"send",     ptr-start) == 0)perm_type = FC_PTP_SEND;
                if(FC_MeFCmpCheckSize(start,"receive",  ptr-start) == 0)perm_type = FC_PTP_RECEIVE;
                if(FC_MeFCmpCheckSize(start,"issue",    ptr-start) == 0)perm_type = FC_PTP_ISSUE;
                if(FC_MeFCmpCheckSize(start,"mine",     ptr-start) == 0)perm_type = FC_PTP_MINE;
                if(FC_MeFCmpCheckSize(start,"admin",    ptr-start) == 0)perm_type = FC_PTP_ADMIN;
                if(FC_MeFCmpCheckSize(start,"activate", ptr-start) == 0)perm_type = FC_PTP_ACTIVATE;
                if(FC_gState->m_Features->Streams())
                {
                    if(FC_MeFCmpCheckSize(start,"create", ptr-start) == 0)perm_type = FC_PTP_CREATE;
                    if(FC_MeFCmpCheckSize(start,"write", ptr-start) == 0)perm_type = FC_PTP_WRITE;
                }
                
                if(perm_type == 0)
                {
                    return 0;
                }
                result |= perm_type;
                start=ptr+1;
            }
        }
        ptr++;
    }
    
    if(result & ~full_type)
    {
        result=0;
    }
    
    return  result;
}

/** Returns permission value and details for key (entity,address,type) */

uint32_t FC_Permissions::GetPermission(const void* lpEntity,const void* lpAddress,uint32_t type,FC_PermissionLedgerRow *row,int checkmempool)
{
    if(lpEntity == NULL)
    {
        return GetPermission(null_entity,lpAddress,type,row,checkmempool);
    }
    
    int err,value_len,mprow;
    uint32_t result;
    FC_PermissionLedgerRow pldRow;
    FC_PermissionDBRow pdbRow;
    unsigned char *ptr;

    pdbRow.Zero();
    meFCpy(&pdbRow.m_Entity,lpEntity,FC_PLS_SIZE_ENTITY);
    meFCpy(&pdbRow.m_Address,lpAddress,FC_PLS_SIZE_ADDRESS);
    pdbRow.m_Type=type;
    
    pldRow.Zero();
    meFCpy(&pldRow.m_Entity,lpEntity,FC_PLS_SIZE_ENTITY);
    meFCpy(&pldRow.m_Address,lpAddress,FC_PLS_SIZE_ADDRESS);
    pldRow.m_Type=type;

    meFCpy(row,&pldRow,sizeof(FC_PermissionLedgerRow));
    
    if(m_Database->m_DB == NULL)
    {
        LogString("GetPermission: Database not opened");
        return 0;
    }
                                                                                
    ptr=(unsigned char*)m_Database->m_DB->Read((char*)&pdbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,&value_len,0,&err);
    if(err)
    {
        LogString("GetPermission: Cannot read from database");
        return 0;
    }
    
    result=0;
    if(ptr)
    {
        meFCpy((char*)&pdbRow+m_Database->m_ValueOffset,ptr,m_Database->m_ValueSize);
        
//        pldRow.m_PrevRow=pdbRow.m_LedgerRow;
        pldRow.m_BlockFrom=pdbRow.m_BlockFrom;
        pldRow.m_BlockTo=pdbRow.m_BlockTo;
        pldRow.m_Flags=pdbRow.m_Flags;
        pldRow.m_ThisRow=pdbRow.m_LedgerRow;
        
        if( (m_CopiedRow > 0) && ( (type == FC_PTP_ADMIN) || (type == FC_PTP_MINE) || (type == FC_PTP_BLOCK_MINER) ) )
        {
            if(m_Ledger->Open() <= 0)
            {
                LogString("GetPermission: couldn't open ledger");
                return 0;
            }
            m_Ledger->GetRow(pdbRow.m_LedgerRow,&pldRow);
            while( (pldRow.m_PrevRow > 0 ) && (pldRow.m_BlockReceived > (uint32_t)m_ForkBlock) )
            {
                m_Ledger->GetRow(pldRow.m_PrevRow,&pldRow);
            }
                        
            if(pldRow.m_BlockReceived > (uint32_t)m_ForkBlock)
            {
                ptr=NULL;
            }
            
            m_Ledger->Close();
        }        
        
        if(ptr)
        {
            row->m_BlockFrom=pldRow.m_BlockFrom;
            row->m_BlockTo=pldRow.m_BlockTo;
            row->m_ThisRow=pldRow.m_ThisRow;
            row->m_Flags=pldRow.m_Flags;        
            pldRow.m_PrevRow=pldRow.m_ThisRow;        
        }
/*        
        row->m_BlockFrom=pdbRow.m_BlockFrom;
        row->m_BlockTo=pdbRow.m_BlockTo;
        row->m_ThisRow=pdbRow.m_LedgerRow;
        row->m_Flags=pdbRow.m_Flags;
 */ 
/*        
        found_in_db=1;
        row->m_FoundInDB=found_in_db;        
 */ 
    }
    
    if(checkmempool)
    { 
        mprow=0;
        while(mprow>=0)
        {
            mprow=m_MemPool->Seek((unsigned char*)&pldRow+m_Ledger->m_KeyOffset);
            if(mprow>=0)
            {
                meFCpy((unsigned char*)row+m_Ledger->m_KeyOffset,m_MemPool->GetRow(mprow),m_Ledger->m_TotalSize);
//                row->m_FoundInDB=found_in_db;
                pldRow.m_PrevRow=row->m_ThisRow;
            }
        }
    }
    
    result=0;
    if((uint32_t)(m_Block+1) >= row->m_BlockFrom)
    {
        if((uint32_t)(m_Block+1) < row->m_BlockTo)
        {
            result=type;
        }                                
    }

    return result;    
}

/** Returns permission value and details for NULL entity */

uint32_t FC_Permissions::GetPermission(const void* lpAddress,uint32_t type,FC_PermissionLedgerRow *row)
{
    return GetPermission(NULL,lpAddress,type,row,1);
}

/** Returns permission value for key (entity,address,type) */

uint32_t FC_Permissions::GetPermission(const void* lpEntity,const void* lpAddress,uint32_t type)
{
    FC_PermissionLedgerRow row;
    return  GetPermission(lpEntity,lpAddress,type,&row,1);
}

/** Returns permission value for NULL entity */

uint32_t FC_Permissions::GetPermission(const void* lpAddress,uint32_t type)
{
    FC_PermissionLedgerRow row;
    return  GetPermission(lpAddress,type,&row);
}

/** Returns all permissions (entity,address) ANDed with type */

uint32_t FC_Permissions::GetAllPermissions(const void* lpEntity,const void* lpAddress,uint32_t type)
{
    uint32_t result;
    
    result=0;
    
    if(type & FC_PTP_CONNECT)    result |= CanConnect(lpEntity,lpAddress);
    if(type & FC_PTP_SEND)       result |= CanSend(lpEntity,lpAddress);
    if(type & FC_PTP_RECEIVE)    result |= CanReceive(lpEntity,lpAddress);
    if(type & FC_PTP_WRITE)      result |= CanWrite(lpEntity,lpAddress);
    if(type & FC_PTP_CREATE)       result |= CanCreate(lpEntity,lpAddress);
    if(type & FC_PTP_ISSUE)      result |= CanIssue(lpEntity,lpAddress);
    if(type & FC_PTP_ADMIN)      result |= CanAdmin(lpEntity,lpAddress);
    if(type & FC_PTP_MINE)       result |= CanMine(lpEntity,lpAddress);
    if(type & FC_PTP_ACTIVATE)   result |= CanActivate(lpEntity,lpAddress);
    
    return result;
}

/** Returns non-zero value if upgrade is approved */

int FC_Permissions::IsApproved(const void* lpUpgrade)
{
    unsigned char address[FC_PLS_SIZE_ADDRESS];
    int result;
    
    memset(address,0,FC_PLS_SIZE_ADDRESS);
    meFCpy(address,lpUpgrade,FC_PLS_SIZE_UPGRADE);
    
    Lock(0);
            
    result=GetPermission(upgrade_entity,address,FC_PTP_UPGRADE);
    
    UnLock();
    
    return result;    
}

/** Returns non-zero value if (entity,address) can connect */

int FC_Permissions::CanConnect(const void* lpEntity,const void* lpAddress)
{
    if(FC_gState->m_NetworkParams->IsProtocolFloatchain() == 0)
    {
        return FC_PTP_CONNECT;
    }
    
//    if(lpEntity == NULL)
    if(FC_IsNullEntity(lpEntity))
    {
        if(FCP_ANYONE_CAN_CONNECT)
        {
            return FC_PTP_CONNECT;
        }
    }
    
    int result;
    Lock(0);
            
    result=GetPermission(lpEntity,lpAddress,FC_PTP_CONNECT);
    
    UnLock();
    
    return result;
}

/** Returns non-zero value if (entity,address) can send */

int FC_Permissions::CanSend(const void* lpEntity,const void* lpAddress)
{
    int result;
    FC_MempoolPermissionRow row;
    
    if(FC_gState->m_NetworkParams->IsProtocolFloatchain() == 0)
    {
        return FC_PTP_SEND;
    }
    
    if(FC_IsNullEntity(lpEntity))
    {
        if(FCP_ANYONE_CAN_SEND)
        {
            return FC_PTP_SEND;
        }
    }
    
    Lock(0);
            
    result = GetPermission(lpEntity,lpAddress,FC_PTP_SEND);    
    
    if(result == 0)
    {
        if(FC_gState->m_Features->Streams())
        {
            result |=  GetPermission(lpEntity,lpAddress,FC_PTP_ISSUE);    
        }
    }
    
    if(result == 0)
    {
        if(FC_gState->m_Features->Streams())
        {
            result |=  GetPermission(lpEntity,lpAddress,FC_PTP_CREATE);    
        }
    }
    
    if(result == 0)
    {
        result |=  GetPermission(lpEntity,lpAddress,FC_PTP_ADMIN);    
    }
    
    if(result == 0)
    {
        result |=  GetPermission(lpEntity,lpAddress,FC_PTP_ACTIVATE);    
    }
        
    if(result)
    {
        result = FC_PTP_SEND; 
    }
    
    if(result)
    {
        if(m_CheckForMempoolFlag)
        {
            meFCpy(&row.m_Entity,lpEntity,FC_PLS_SIZE_ENTITY);
            meFCpy(&row.m_Address,lpAddress,FC_PLS_SIZE_ADDRESS);
            row.m_Type=FC_PTP_SEND;
            m_MempoolPermissions->Add(&row);
        }
    }
    
    UnLock();
    
    return result;
}

/** Returns non-zero value if (entity,address) can receive */

int FC_Permissions::CanReceive(const void* lpEntity,const void* lpAddress)
{
    int result;
    FC_MempoolPermissionRow row;

    if(FC_gState->m_NetworkParams->IsProtocolFloatchain() == 0)
    {
        return FC_PTP_RECEIVE;
    }
    
    if(FC_IsNullEntity(lpEntity))
    {
        if(FCP_ANYONE_CAN_RECEIVE)
        {
            return FC_PTP_RECEIVE;
        }
    }

    Lock(0);
    
    result = GetPermission(lpEntity,lpAddress,FC_PTP_RECEIVE);    
    
    if(result == 0)
    {
        result |=  GetPermission(lpEntity,lpAddress,FC_PTP_ADMIN);    
    }
    
    if(result == 0)
    {
        result |=  GetPermission(lpEntity,lpAddress,FC_PTP_ACTIVATE);    
    }
    
    if(result)
    {
        result = FC_PTP_RECEIVE; 
    }
    
    if(result)
    {
        if(m_CheckForMempoolFlag)
        {
            meFCpy(&row.m_Entity,lpEntity,FC_PLS_SIZE_ENTITY);
            meFCpy(&row.m_Address,lpAddress,FC_PLS_SIZE_ADDRESS);
            row.m_Type=FC_PTP_RECEIVE;
            m_MempoolPermissions->Add(&row);
        }
    }
    
    UnLock();
    
    return result;
}

/** Returns non-zero value if (entity,address) can write */

int FC_Permissions::CanWrite(const void* lpEntity,const void* lpAddress)
{
    int result;
    FC_MempoolPermissionRow row;

    if(FC_gState->m_NetworkParams->IsProtocolFloatchain() == 0)
    {
        return 0;
    }
    
    Lock(0);
    
    result = GetPermission(lpEntity,lpAddress,FC_PTP_WRITE);    
    
    if(result)
    {
        result = FC_PTP_WRITE; 
    }
    
    if(result)
    {
        if(m_CheckForMempoolFlag)
        {
            meFCpy(&row.m_Entity,lpEntity,FC_PLS_SIZE_ENTITY);
            meFCpy(&row.m_Address,lpAddress,FC_PLS_SIZE_ADDRESS);
            row.m_Type=FC_PTP_WRITE;
            m_MempoolPermissions->Add(&row);
        }
    }
    
    UnLock();
    
    return result;
}

/** Returns non-zero value if (entity,address) can write */

int FC_Permissions::CanCreate(const void* lpEntity,const void* lpAddress)
{
    int result;

//    if(lpEntity == NULL)
    if(FC_IsNullEntity(lpEntity))
    {
        if(FCP_ANYONE_CAN_RECEIVE)
        {
            return FC_PTP_CREATE;
        }
    }
    
    Lock(0);
    
    result = GetPermission(lpEntity,lpAddress,FC_PTP_CREATE);    
    
    if(result)
    {
        result = FC_PTP_CREATE; 
    }
    
    UnLock();
    
    return result;
}

/** Returns non-zero value if (entity,address) can issue */

int FC_Permissions::CanIssue(const void* lpEntity,const void* lpAddress)
{
//    if(lpEntity == NULL)
    if(FC_IsNullEntity(lpEntity))
    {
        if(FC_gState->m_NetworkParams->GetInt64Param("anyonecanissue"))
        {
            return FC_PTP_ISSUE;
        }
    }

    int result;
    Lock(0);
            
    result=GetPermission(lpEntity,lpAddress,FC_PTP_ISSUE);
    
    UnLock();
    
    return result;    
}

/** Returns 1 if we are still in setup period (NULL entity only) */

int FC_Permissions::IsSetupPeriod()
{
    if(m_Block+1<FC_gState->m_NetworkParams->GetInt64Param("setupfirstblocks"))
    {
        return 1;
    }
    return 0;
}

/** Returns number of active miners (NULL entity only) */

int FC_Permissions::GetActiveMinerCount()
{
    Lock(0);
    
    int miner_count=m_MinerCount;
    int diversity;
    if(!IsSetupPeriod())
    {
        diversity=(int)FC_gState->m_NetworkParams->GetInt64Param("miningdiversity");
        if(diversity > 0)
        {
            diversity=(int)((miner_count*diversity-1)/FC_PRM_DECIMAL_GRANULARITY);
        }
        diversity++;
        miner_count-=diversity-1;
        if(miner_count<1)
        {
            miner_count=1;
        }
        if(miner_count > m_MinerCount)
        {
            miner_count=m_MinerCount;
        }
    }    
    if(m_MinerCount <= 0)
    {
        miner_count=1;
    }
    
    UnLock();
    
    return miner_count;
}

/** Returns non-zero value if (entity,address) can mine */

int FC_Permissions::CanMine(const void* lpEntity,const void* lpAddress)
{
    FC_PermissionLedgerRow row;
    
    int result;    
    int32_t last;
        
    if(FC_gState->m_NetworkParams->IsProtocolFloatchain() == 0)
    {
        return FC_PTP_MINE;
    }
    
    if(FC_IsNullEntity(lpEntity))
    {
        if(FCP_ANYONE_CAN_MINE)
        {
             return FC_PTP_MINE;
        }
    }
    
    Lock(0);

    int miner_count=m_ClearedMinerCount;
    int check_mempool=0;        
    
    result = GetPermission(lpEntity,lpAddress,FC_PTP_MINE,&row,check_mempool);        

    if(result)
    {
        if(FC_IsNullEntity(lpEntity))                                           // Mining diversity is checked only for NULL object
        {
            GetPermission(lpEntity,lpAddress,FC_PTP_BLOCK_MINER,&row,0);    
            last=row.m_BlockFrom;
            if(last)
            {
                if(IsBarredByDiversity(m_Block+1,last,miner_count))
                {
                    result=0;                    
                }
            }        
        }
    }
    
    UnLock();
    
    return result;
}

/** Returns non-zero value if address can mine block with specific height */

// WARNING! Possible bug in this functiom, But function is not used

int FC_Permissions::CanMineBlock(const void* lpAddress,uint32_t block)
{
    FC_PermissionLedgerRow row;
    FC_BlockLedgerRow block_row;
    int result;    
    int miner_count;
//    int diversity;
    uint32_t last;
    
    if(FC_gState->m_NetworkParams->IsProtocolFloatchain() == 0)
    {
        return FC_PTP_MINE;
    }
    
    if(FCP_ANYONE_CAN_MINE)
    {
         return FC_PTP_MINE;
    }
    
    if(block == 0)                                                              
    {
        return 0;
    }
    
    Lock(0);

    result=1;
    if(m_Ledger->Open() <= 0)
    {
        LogString("Error: CanMineBlock: couldn't open ledger");
        result=0;
    }
    
    if(result)
    {
        GetPermission(NULL,lpAddress,FC_PTP_MINE,&row,0);        
        
        if(row.m_ThisRow)
        {
            m_Ledger->GetRow(row.m_ThisRow,&row);
            while( (row.m_PrevRow > 0 ) && (row.m_BlockReceived >= block) )
            {
                m_Ledger->GetRow(row.m_PrevRow,&row);
            }
            if(row.m_BlockReceived >= block)
            {
                result=0;
            }
        }
        else
        {
            result=0;
        }
    }
    
    if(result)
    {
        if( (block < row.m_BlockFrom) || (block >= row.m_BlockTo) )
        {
            result=0;
        }        
    }
    
    if(result>0)
    {
        GetPermission(NULL,lpAddress,FC_PTP_BLOCK_MINER,&row,0);    
        
        if(row.m_ThisRow)
        {
            m_Ledger->GetRow(row.m_ThisRow,&row);
            while( (row.m_PrevRow > 0 ) && (row.m_BlockReceived >= block) )
            {
                m_Ledger->GetRow(row.m_PrevRow,&row);
            }
            if(row.m_BlockReceived < block)
            {
                last=row.m_BlockFrom;                                           // block #1 is mined by genesis admin, no false negative here
                if(last)                                                         
                {
                    block_row.Zero();
                    sprintf((char*)block_row.m_Address,"Block %08X row",block-1);
                    GetPermission(block_row.m_Address,FC_PTP_BLOCK_INDEX,(FC_PermissionLedgerRow*)&block_row);
                    m_Ledger->GetRow(block_row.m_ThisRow,(FC_PermissionLedgerRow*)&block_row);
                    
                    miner_count=block_row.m_MinerCount;                         // Probably BUG here, should be cleared miner count, but function  not used
   
                    if(IsBarredByDiversity(block,last,miner_count))
                    {
                        result=0;
                    }
                }
            }
        }
    }
    
    m_Ledger->Close();
    
    UnLock();
    
    return result;
}

int FC_Permissions::FindLastAllowedMinerRow(FC_PermissionLedgerRow *row,uint32_t block,int prev_result)
{
    FC_PermissionLedgerRow pldRow;
    int result;
    
    if(row->m_ThisRow < m_Row-m_MemPool->GetCount())
    {
        return prev_result;
    }
    
    if(row->m_BlockReceived < block)
    {
        return prev_result;        
    }
    
    result=prev_result;
    
    meFCpy(&pldRow,row,sizeof(FC_PermissionLedgerRow));
    while( (pldRow.m_ThisRow >= m_Row-m_MemPool->GetCount() ) && (pldRow.m_BlockReceived >= block) && (pldRow.m_PrevRow > 0) )
    {
        meFCpy(&pldRow,m_MemPool->GetRow(pldRow.m_PrevRow),sizeof(FC_PermissionLedgerRow));
    }
    
    if(pldRow.m_PrevRow <=0 )
    {
        return 0;
    }

    if(pldRow.m_ThisRow < m_Row-m_MemPool->GetCount())
    {
        if(m_Ledger->Open() <= 0)
        {
            LogString("Error: CanMineBlock: couldn't open ledger");
            return 0;
        }
    
        m_Ledger->GetRow(pldRow.m_ThisRow,&pldRow);
        m_Ledger->Close();
    }
    
    result=0;
    if((uint32_t)block >= pldRow.m_BlockFrom)
    {
        if((uint32_t)block < pldRow.m_BlockTo)
        {
            result=FC_PTP_MINE;
        }                                
    }        
    
    return result;    
}

int FC_Permissions::CanMineBlockOnFork(const void* lpAddress,uint32_t block,uint32_t last_after_fork)
{
    FC_PermissionLedgerRow row;
    uint32_t last;
    int result;
    
    if(FC_gState->m_NetworkParams->IsProtocolFloatchain() == 0)
    {
        return FC_PTP_MINE;
    }
    
    if(FCP_ANYONE_CAN_MINE)
    {
         return FC_PTP_MINE;
    }
    
    if(block == 0)                                                              
    {
        return 0;
    }
    
    Lock(0);

    int miner_count=m_ClearedMinerCountForMinerVerification;
    
    result=GetPermission(NULL,lpAddress,FC_PTP_MINE,&row,1);                

    result=FindLastAllowedMinerRow(&row,block,result);
    
    if(result)
    {
        
        last=last_after_fork;

        if(last == 0)
        {
            GetPermission(NULL,lpAddress,FC_PTP_BLOCK_MINER,&row,0);    

            if(row.m_ThisRow)
            {
                last=row.m_BlockFrom;
            }    
        }
    
        if(last)
        {            
            if(IsBarredByDiversity(block,last,miner_count))
            {
                result=0;
            }            
        }
    }
    
    UnLock();
    return result;
    
}

int FC_Permissions::IsBarredByDiversity(uint32_t block,uint32_t last,int miner_count)
{
    int diversity;
    if(miner_count)
    {
        if(block >= FC_gState->m_NetworkParams->GetInt64Param("setupfirstblocks"))
        {                        
            diversity=(int)FC_gState->m_NetworkParams->GetInt64Param("miningdiversity");
            if(diversity > 0)
            {
                diversity=(int)((miner_count*diversity-1)/FC_PRM_DECIMAL_GRANULARITY);
            }
            diversity++;
            if(diversity<1)
            {
                diversity=1;
            }
            if(diversity > miner_count)
            {
                diversity=miner_count;
            }
            if((int)(block-last) <= diversity-1)
            {
                return 1;
            }
        }
    }

    return 0;
}


/** Returns non-zero value if (entity,address) can admin */

int FC_Permissions::CanAdmin(const void* lpEntity,const void* lpAddress)
{
    if(FC_gState->m_Features->Streams())
    {
        if(m_Block == -1)
        {
            return FC_PTP_ADMIN;
        }
    }
    else
    {
        if(m_AdminCount == 0)
        {
            return FC_PTP_ADMIN;
        }        
    }
    
    if(FC_IsNullEntity(lpEntity))
    {
        if(FCP_ANYONE_CAN_ADMIN)
        {
            return FC_PTP_ADMIN;
        }
    }
    
    int result;
    Lock(0);
            
    result=GetPermission(lpEntity,lpAddress,FC_PTP_ADMIN);
    
    UnLock();
    
    return result;    
}

/** Returns non-zero value if (entity,address) can activate */

int FC_Permissions::CanActivate(const void* lpEntity,const void* lpAddress)
{
    if(FC_IsNullEntity(lpEntity))
    {
        if(FCP_ANYONE_CAN_ACTIVATE)
        {
            return FC_PTP_ACTIVATE;
        }
    }
    
    int result;
    Lock(0);
            
    result=GetPermission(lpEntity,lpAddress,FC_PTP_ACTIVATE);
    
    UnLock();
    
    if(result == 0)
    {
        result = CanAdmin(lpEntity,lpAddress);
    }
    
    if(result)
    {
        result = FC_PTP_ACTIVATE; 
    }
    
    return result;    
}

/** Updates admin and miner counts (NULL entity only) */

int FC_Permissions::UpdateCounts()
{
    FC_PermissionDBRow pdbRow;
    FC_PermissionDBRow pdbAdminMinerRow;
    FC_PermissionLedgerRow row;
    
    unsigned char *ptr;
    int dbvalue_len,err,result;
    uint32_t type;
    err=FC_ERR_NOERROR;
    
    pdbAdminMinerRow.Zero();
    
    m_AdminCount=0;
    m_MinerCount=0;
    
//    m_DBRowCount=0;

    meFCpy(pdbAdminMinerRow.m_Entity,adminminerlist_entity,FC_PLS_SIZE_ENTITY);
    pdbAdminMinerRow.m_Type=FC_PTP_CONNECT;

    ptr=(unsigned char*)m_Database->m_DB->Read((char*)&pdbAdminMinerRow+m_Database->m_KeyOffset,m_Database->m_KeySize,&dbvalue_len,FC_OPT_DB_DATABASE_SEEK_ON_READ,&err);
    
    if(ptr)
    {
        ptr=(unsigned char*)m_Database->m_DB->MoveNext(&err);
        while(ptr)
        {
            meFCpy((char*)&pdbAdminMinerRow+m_Database->m_KeyOffset,ptr,m_Database->m_TotalSize);            
            if(meFCmp(pdbAdminMinerRow.m_Entity,adminminerlist_entity,FC_PLS_SIZE_ENTITY) == 0)
            {                   
                if(pdbAdminMinerRow.m_Type == FC_PTP_ADMIN)
                {
                    if(GetPermission(NULL,pdbAdminMinerRow.m_Address,FC_PTP_ADMIN))
                    {
                        m_AdminCount++;                        
                    }
                }                    
                if(pdbAdminMinerRow.m_Type == FC_PTP_MINE)
                {
                    result=GetPermission(NULL,pdbAdminMinerRow.m_Address,FC_PTP_MINE,&row,1);
                    if(m_CopiedRow > 0)
                    {
                        result=FindLastAllowedMinerRow(&row,m_Block+1,result);                        
                    }
                    if(result)
                    {
                        m_MinerCount++;
                    }
                }                    
            }
            else
            {
                ptr=NULL;
            }
                    
            if(ptr)
            {
                ptr=(unsigned char*)m_Database->m_DB->MoveNext(&err);
            }
        }        
    }
    else
    {        
        pdbAdminMinerRow.Zero();
        meFCpy(pdbAdminMinerRow.m_Entity,adminminerlist_entity,FC_PLS_SIZE_ENTITY);
        pdbAdminMinerRow.m_Type=FC_PTP_CONNECT;
        
        err=m_Database->m_DB->Write((char*)&pdbAdminMinerRow+m_Database->m_KeyOffset,m_Database->m_KeySize,
                                    (char*)&pdbAdminMinerRow+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
        if(err)
        {
            return err;
        }
        
        pdbRow.Zero();

        ptr=(unsigned char*)m_Database->m_DB->Read((char*)&pdbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,&dbvalue_len,FC_OPT_DB_DATABASE_SEEK_ON_READ,&err);
        if(err)
        {
            return err;
        }
        if(ptr == NULL)
        {
            return FC_ERR_NOERROR;
        }

        meFCpy((char*)&pdbRow+m_Database->m_ValueOffset,ptr,m_Database->m_ValueSize);

        while(ptr)
        {
//            m_DBRowCount++;
            type=pdbRow.m_Type;
            if( (type == FC_PTP_ADMIN) || (type == FC_PTP_MINE))
            {
                if(meFCmp(pdbRow.m_Entity,null_entity,FC_PLS_SIZE_ENTITY) == 0)
                {                    
                    pdbAdminMinerRow.m_Type=type;
                    meFCpy(pdbAdminMinerRow.m_Address,pdbRow.m_Address,FC_PLS_SIZE_ADDRESS);
                    err=m_Database->m_DB->Write((char*)&pdbAdminMinerRow+m_Database->m_KeyOffset,m_Database->m_KeySize,
                                                (char*)&pdbAdminMinerRow+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
                    if(err)
                    {
                        return err;
                    }
                    
                    if(pdbRow.InBlockRange(m_Block))
                    {
                        if(type == FC_PTP_ADMIN)
                        {
                            m_AdminCount++;
                        }
                        if(type == FC_PTP_MINE)
                        {
                            m_MinerCount++;
                        }
                    }            
                }
            }
            ptr=(unsigned char*)m_Database->m_DB->MoveNext(&err);
            if(err)
            {
                LogString("Error on MoveNext");            
            }
            if(ptr)
            {
                meFCpy((char*)&pdbRow+m_Database->m_KeyOffset,ptr,m_Database->m_TotalSize);            
            }
        }
        
        err=m_Database->m_DB->Commit(FC_OPT_DB_DATABASE_TRANSACTIONAL);
        if(err)
        {
            return err;
        }
    }
    
    return err;
}

/** Dumps database and ledger */

void FC_Permissions::Dump()
{    
    FC_PermissionDBRow pdbRow;
    FC_PermissionLedgerRow pldRow;
    
    unsigned char *ptr;
    int dbvalue_len,err,i;
    
    pdbRow.Zero();
    
    printf("\nDB\n");
    
    ptr=(unsigned char*)m_Database->m_DB->Read((char*)&pdbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,&dbvalue_len,FC_OPT_DB_DATABASE_SEEK_ON_READ,&err);
    if(err)
    {
        return;
    }

    if(ptr)
    {
        meFCpy((char*)&pdbRow+m_Database->m_ValueOffset,ptr,m_Database->m_ValueSize);
        while(ptr)
        {
            FC_MemoryDumpCharSize((char*)&pdbRow+m_Database->m_KeyOffset,0,m_Database->m_TotalSize,m_Database->m_TotalSize);        
            ptr=(unsigned char*)m_Database->m_DB->MoveNext(&err);
            if(ptr)
            {
                meFCpy((char*)&pdbRow+m_Database->m_KeyOffset,ptr,m_Database->m_TotalSize);            
            }
        }
    }
    
    printf("Ledger\n");

    m_Ledger->Open();
    
    for(i=0;i<(int)m_Row-m_MemPool->GetCount();i++)
    {
        m_Ledger->GetRow(i,&pldRow);
        FC_MemoryDumpCharSize((unsigned char*)&pldRow+m_Ledger->m_KeyOffset,0,m_Ledger->m_TotalSize,m_Ledger->m_TotalSize/2);                
    }
    
    m_Ledger->Close();
    
    FC_DumpSize("MemPool",m_MemPool->GetRow(0),m_MemPool->GetCount()*m_Ledger->m_TotalSize,m_Ledger->m_TotalSize/2);
    
}

/** Returns number of admins (NULL entity only) */

int FC_Permissions::GetAdminCount()
{
    return m_AdminCount;
}

/** Returns number of miners (NULL entity only) */

int FC_Permissions::GetMinerCount()
{
    return m_MinerCount;
}

/** Clears memory pool, external, with lock */

int FC_Permissions::ClearMemPool()
{
    int result;
    Lock(1);
    result=ClearMemPoolInternal();
    UnLock();
    return result;        
}

/** Clears memory pool, internal, without lock */

int FC_Permissions::ClearMemPoolInternal()
{
    char msg[256];
    if(m_MemPool->GetCount())
    {
        m_Row-=m_MemPool->GetCount();
        m_MemPool->Clear();
        m_AdminCount=m_ClearedAdminCount;
        m_MinerCount=m_ClearedMinerCount;
        UpdateCounts();
        sprintf(msg,"Mempool clr : %9d, Admin count: %d, Miner count: %d, Ledger Rows: %ld",m_Block,m_AdminCount,m_MinerCount,m_Row);
        LogString(msg);
    }
    m_ClearedAdminCount=m_AdminCount;
    m_ClearedMinerCount=m_MinerCount;
    m_ClearedMinerCountForMinerVerification=m_MinerCount;
    
    return FC_ERR_NOERROR;
}

/** Copies memory pool */

int FC_Permissions::CopyMemPool()
{
    int i,err;
    unsigned char *ptr;
    char msg[256];
    
    Lock(1);
    err=FC_ERR_NOERROR;
    
    m_CopiedMemPool->Clear();
    for(i=0;i<m_MemPool->GetCount();i++)
    {
        ptr=m_MemPool->GetRow(i);
        err=m_CopiedMemPool->Add(ptr,ptr+m_MemPool->m_KeySize);        
        if(err)
        {
            LogString("Error while copying mempool");
            goto exitlbl;
        }
    }
    
    m_CopiedAdminCount=m_AdminCount;
    m_CopiedMinerCount=m_MinerCount;
    
    if(m_MemPool->GetCount())
    {
//        UpdateCounts();
        sprintf(msg,"Mempool copy: %9d, Admin count: %d, Miner count: %d, Ledger Rows: %ld",m_Block,m_AdminCount,m_MinerCount,m_Row);
        LogString(msg);
    }

exitlbl:
    
    UnLock();
    return err;
}

/** Restores memory pool */

int FC_Permissions::RestoreMemPool()
{
    int i,err;
    unsigned char *ptr;
    char msg[256];
    
    err=FC_ERR_NOERROR;
    
    Lock(1);
    
    ClearMemPoolInternal();
    
    for(i=0;i<m_CopiedMemPool->GetCount();i++)
    {
        ptr=m_CopiedMemPool->GetRow(i);
        err=m_MemPool->Add(ptr,ptr+m_MemPool->m_KeySize);        
        if(err)
        {
            LogString("Error while restoring mempool");
            goto exitlbl;
        }
    }
    
    m_Row+=m_CopiedMemPool->GetCount();
    m_AdminCount=m_CopiedAdminCount;
    m_MinerCount=m_CopiedMinerCount;
    
    if(m_MemPool->GetCount())
    {
//        UpdateCounts();
        sprintf(msg,"Mempool rstr: %9d, Admin count: %d, Miner count: %d, Ledger Rows: %ld",m_Block,m_AdminCount,m_MinerCount,m_Row);
        LogString(msg);
    }
    
exitlbl:
    
    UnLock();

    return err;    
}

/** Sets back database pointer to point in the past, no real rollback is made */

int FC_Permissions::RollBackBeforeMinerVerification(uint32_t block)
{
    int err,take_it;
    uint64_t this_row;
    FC_PermissionLedgerRow pldRow;
    FC_BlockLedgerRow pldBlockRow;
    
    if(block > (uint32_t)m_Block)
    {
        return FC_ERR_INTERNAL_ERROR;
    }
    
    err=FC_ERR_NOERROR; 
    
    CopyMemPool();
//    ClearMemPool();

    Lock(1);

    if(m_MemPool->GetCount())
    {
        m_AdminCount=m_ClearedAdminCount;
        m_MinerCount=m_ClearedMinerCount;
        m_Row-=m_MemPool->GetCount();    
        m_MemPool->Clear();
    }
    
    m_CopiedBlock=m_Block;
    m_CopiedRow=m_Row;
    m_ForkBlock=block;
    m_ClearedMinerCountForMinerVerification=m_MinerCount;
    
    
    if(block == (uint32_t)m_Block)
    {
        goto exitlbl;
    }

    if(m_Ledger->Open() <= 0)
    {
        LogString("Error: RollBackBeforeMinerVerification: couldn't open ledger");
        return FC_ERR_DBOPEN_ERROR;
    }
    
    this_row=m_Row-1;
    take_it=1;
    if(this_row == 0)
    {
        take_it=0;
    }
    
    while(take_it && (this_row>0))
    {
        m_Ledger->GetRow(this_row,&pldRow);
        
        if(pldRow.m_BlockReceived > block)
        {
            this_row--;
        }
        else
        {
            take_it=0;
        }
    }
        
    this_row++;

    m_Row=this_row;
    m_Block=block;
    
    pldBlockRow.Zero();
    sprintf((char*)pldBlockRow.m_Address,"Block %08X row",block);
    GetPermission(pldBlockRow.m_Address,FC_PTP_BLOCK_INDEX,(FC_PermissionLedgerRow*)&pldBlockRow);
    m_Ledger->GetRow(pldBlockRow.m_ThisRow,(FC_PermissionLedgerRow*)&pldBlockRow);

    m_AdminCount=pldBlockRow.m_AdminCount;
    m_MinerCount=pldBlockRow.m_MinerCount;
    m_ClearedMinerCountForMinerVerification=m_MinerCount;

    /*
    char msg[256];
    sprintf(msg,"Verifier Rollback: %9d, Admin count: %d, Miner count: %d, Ledger Rows: %ld",
            m_Block,m_AdminCount,m_MinerCount,m_Row);
    LogString(msg);
    */

exitlbl:
    
    m_Ledger->Close();  
            
    UnLock();

    return err;    
}

/** Restores chain pointers and mempool after miner verification*/

int FC_Permissions::RestoreAfterMinerVerification()
{
    int i,err;
    unsigned char *ptr;
    char msg[256];
    
    Lock(1);
    
    m_MemPool->Clear();
    
    m_Block=m_CopiedBlock;
    m_ForkBlock=0;

    m_Row=m_CopiedRow;
    m_CopiedRow=0;
    m_CopiedBlock=0;
    
    for(i=0;i<m_CopiedMemPool->GetCount();i++)
    {
        ptr=m_CopiedMemPool->GetRow(i);
        err=m_MemPool->Add(ptr,ptr+m_MemPool->m_KeySize);        
        if(err)
        {
            LogString("Error while restoring mempool");
            goto exitlbl;
        }
    }
    
    m_Row+=m_CopiedMemPool->GetCount();
    m_AdminCount=m_CopiedAdminCount;
    m_MinerCount=m_CopiedMinerCount;
            
    if(m_CopiedMemPool->GetCount())
    {
        sprintf(msg,"Mempool ramv: %9d, Admin count: %d, Miner count: %d, Ledger Rows: %ld",m_Block,m_AdminCount,m_MinerCount,m_Row);
        LogString(msg);
    }
    
exitlbl:
    UnLock();

    return FC_ERR_NOERROR;
}


/** Returns number of admins required for consensus for specific permission type (NULL entity only) */

int FC_Permissions::AdminConsensus(const void* lpEntity,uint32_t type)
{
    int consensus;
        
    if(GetAdminCount()==0)
    {
        return 1;
    }
    
    if(FC_IsNullEntity(lpEntity))
    {
        switch(type)    
        {
            case FC_PTP_ADMIN:
            case FC_PTP_MINE:
            case FC_PTP_ACTIVATE:
            case FC_PTP_ISSUE:
            case FC_PTP_CREATE:
                if(IsSetupPeriod())            
                {
                    return 1;
                }

                consensus=0;
                if(type == FC_PTP_ADMIN)
                {
                    consensus=FC_gState->m_NetworkParams->GetInt64Param("adminconsensusadmin");
                }
                if(type == FC_PTP_MINE)
                {
                    consensus=FC_gState->m_NetworkParams->GetInt64Param("adminconsensusmine");
                }
                if(type == FC_PTP_ACTIVATE)
                {
                    consensus=FC_gState->m_NetworkParams->GetInt64Param("adminconsensusactivate");
                }
                if(type == FC_PTP_ISSUE)
                {
                    consensus=FC_gState->m_NetworkParams->GetInt64Param("adminconsensusissue");
                }
                if(type == FC_PTP_CREATE)
                {
                    consensus=FC_gState->m_NetworkParams->GetInt64Param("adminconsensuscreate");
                }

                if(consensus==0)
                {
                    return 1;
                }

                return (int)((GetAdminCount()*(uint32_t)consensus-1)/FC_PRM_DECIMAL_GRANULARITY)+1;            
        }
    }

    if(FC_IsUpgradeEntity(lpEntity))
    {
        switch(type)    
        {
            case FC_PTP_UPGRADE:
                if(IsSetupPeriod())            
                {
                    return 1;
                }

                consensus=FC_gState->m_NetworkParams->GetInt64Param("adminconsensusupgrade");
                if(consensus==0)
                {
                    return 1;
                }

                return (int)((GetAdminCount()*(uint32_t)consensus-1)/FC_PRM_DECIMAL_GRANULARITY)+1;            
        }
    }
    
    return 1;
}

/** Verifies whether consensus is reached. Update blockFrom/To fields if needed. */

int FC_Permissions::VerifyConsensus(FC_PermissionLedgerRow *newRow,FC_PermissionLedgerRow *lastRow,int *remaining)
{
    int consensus,required,found,exit_now;
    uint64_t prevRow,countLedgerRows;
    FC_PermissionLedgerRow pldRow;
    FC_PermissionLedgerRow *ptr;
    
    consensus=AdminConsensus(newRow->m_Entity,newRow->m_Type);
    required=consensus;
    
    if(remaining)
    {
        *remaining=0;
    }
    
    newRow->m_Consensus=consensus;    
    
    if(required <= 1)
    {
        return FC_ERR_NOERROR;
    }
        
    countLedgerRows=m_Row-m_MemPool->GetCount();

    m_TmpPool->Clear();
    
    exit_now=0;
    meFCpy(&pldRow,newRow,sizeof(FC_PermissionLedgerRow));
    while(required)
    {
        meFCpy(pldRow.m_Address,pldRow.m_Admin,FC_PLS_SIZE_ADDRESS);         // Looking for row from the same admin
        
        ptr=&pldRow;        
        
        found=m_TmpPool->Seek((unsigned char*)ptr);                                           
        if(found >= 0)
        {
            ptr=(FC_PermissionLedgerRow *)m_TmpPool->GetRow(found);
            if(ptr->m_Timestamp < pldRow.m_Timestamp)
            {
                if(ptr->m_GrantFrom == newRow->m_GrantFrom)
                {
                    if(ptr->m_GrantTo == newRow->m_GrantTo)
                    {
                        required++;                                             // Duplicate record, compensate for decrementing below 
                    }            
                }                
                m_TmpPool->PutRow(found,&pldRow,(unsigned char*)&pldRow+m_Database->m_ValueOffset);
                ptr=&pldRow;
            }
            else
            {
                ptr=NULL;                                                       // Ignore this record, we already have newer
            }
        }
        else
        {
            m_TmpPool->Add(&pldRow,(unsigned char*)&pldRow+m_Database->m_ValueOffset);            
        }
        
        if(ptr)                                                                 
        {
            if(ptr->m_GrantFrom == newRow->m_GrantFrom)
            {
                if(ptr->m_GrantTo == newRow->m_GrantTo)
                {
                    if((remaining == NULL) || (pldRow.m_BlockReceived > 0))     // Avoid decrementing when checking for RequiredForConsensus
                    {
                        required--;                        
                    }
                }            
            }
        }

        if(required)
        {
            prevRow=pldRow.m_PrevRow;
        
            if(prevRow == 0)
            {
                exit_now=1;
            }
            else
            {
                if(prevRow >= countLedgerRows)                                  // Previous row in mempool
                {
                    pldRow.Zero();
                    meFCpy((unsigned char*)&pldRow+m_Ledger->m_KeyOffset,m_MemPool->GetRow(prevRow-countLedgerRows),m_Ledger->m_TotalSize);
                }
                else                                                            // Previous row in ledger
                {
                    if(m_Ledger->Open() <= 0)
                    {
                        LogString("Error: VerifyConsensus: couldn't open ledger");
                        return FC_ERR_DBOPEN_ERROR;
                    }

                    if(m_Ledger->GetRow(prevRow,&pldRow))
                    {
                        LogString("Error: VerifyConsensus: couldn't get row");
                        m_Ledger->Close();
                        return FC_ERR_DBOPEN_ERROR;
                    }            
                    
                    if(meFCmp(&pldRow,newRow,m_Database->m_ValueOffset) != 0)
                    {
                        LogString("Error: VerifyConsensus: row key mismatch");
                        m_Ledger->Close();
                        return FC_ERR_DBOPEN_ERROR;                        
                    }
                    
                    if(prevRow != pldRow.m_ThisRow)
                    {
                        LogString("Error: VerifyConsensus: row id mismatch");
                        m_Ledger->Close();
                        return FC_ERR_DBOPEN_ERROR;                                                
                    }
                }                
            }
            
            if(pldRow.m_Consensus > 0)                                          // Reached previous consensus row
            {
                exit_now=1;
            }
            
            if(exit_now)                                                        // We didn't reach consensus, revert blockFrom/To to previous values
            {
                if(remaining)
                {
                    *remaining=required;
                }
                newRow->m_Consensus=0;                                          
                newRow->m_BlockFrom=lastRow->m_BlockFrom;
                newRow->m_BlockTo=lastRow->m_BlockTo;
                return FC_ERR_NOERROR;            
            }
        
        }                
        
    }
    
    m_Ledger->Close();
    
    return FC_ERR_NOERROR;
}

/** Fills details buffer for permission row */

int FC_Permissions::FillPermissionDetails(FC_PermissionDetails *plsRow,FC_Buffer *plsDetailsBuffer)
{
    int consensus,required,found,exit_now,phase,in_consensus,i;
    uint64_t prevRow,countLedgerRows;
    FC_PermissionLedgerRow pldRow;
    FC_PermissionLedgerRow pldAdmin;
    FC_PermissionDetails plsDetails;
    FC_PermissionLedgerRow *ptr;
    
    countLedgerRows=m_Row-m_MemPool->GetCount();
    consensus=AdminConsensus(plsRow->m_Entity,plsRow->m_Type);
    required=consensus;
    
    plsRow->m_RequiredAdmins=consensus;

    prevRow=plsRow->m_LastRow;

    phase=0;
    exit_now=0;
    
    int path;
    
    while(exit_now == 0)
    {    
        if(prevRow == 0)
        {
            LogString("Internal error: FillPermissionDetails: prevRow=0 ");
            return FC_ERR_INVALID_PARAMETER_VALUE;
        }
        
        if(prevRow >= countLedgerRows)                                          // in mempool
        {
            pldRow.Zero();
            meFCpy((unsigned char*)&pldRow+m_Ledger->m_KeyOffset,m_MemPool->GetRow(prevRow-countLedgerRows),m_Ledger->m_TotalSize);
        }
        else
        {
            if(m_Ledger->GetRow(prevRow,&pldRow))
            {
                m_Ledger->Close();
                LogString("Error: FillPermissionDetails: couldn't get row");
                return FC_ERR_DBOPEN_ERROR;
            }            
            if(meFCmp(&pldRow,plsRow,m_Database->m_ValueOffset) != 0)           // m_Database->m_ValueOffset are common for all structures
            {
                m_Ledger->Close();
                LogString("Error: FillPermissionDetails: row key mismatch");
                return FC_ERR_DBOPEN_ERROR;                        
            }
                    
            if(prevRow != pldRow.m_ThisRow)
            {
                m_Ledger->Close();
                LogString("Error: FillPermissionDetails: row id mismatch");
                return FC_ERR_DBOPEN_ERROR;                                                
            }
        }                
        
        path=0;
        
        if(phase == 0)
        {
            path+=1;
            if(pldRow.m_Consensus > 0)                                          // The last row is in consensus
            {
                meFCpy(plsRow->m_LastAdmin,pldRow.m_Admin,FC_PLS_SIZE_ADDRESS);
                if(required <= 1)                                               // No pending records, this is the only admin, no details required
                {
                    return FC_ERR_NOERROR;
                }
                phase=2;                                                        // We are in consensus, but want to find all admins
            }
            else
            {
                plsRow->m_Flags |= FC_PFL_HAVE_PENDING;
                phase=1;                                                        // There are pending records
            }
            if(plsDetailsBuffer == NULL)                                              // We have details, but they are not required
            {
                return FC_ERR_NOERROR;
            }
            else
            {
                m_TmpPool->Clear();                
            }
        }
        else
        {
            if(pldRow.m_Consensus > 0)                                          // Consensus record
            {
                path+=128;
                if(phase == 1)                                                  // We were waiting for consensus record, now we are just looking to fill list of admins
                {
                    path+=256;
                    phase=2;
                }
                else                                                            // This is previous consensus, we should not go beyond this point
                {
                    path+=512;
                    exit_now=1;
                }
            }                                
        }
        
        if(exit_now == 0)
        {
            path+=2;
            in_consensus=0;                                                     // This row match consensus
            if(pldRow.m_GrantFrom == plsRow->m_BlockFrom)
            {
                if(pldRow.m_GrantTo == plsRow->m_BlockTo)
                {
                    if(phase == 2)
                    {
                        in_consensus=1;
                    }
                }            
            }                
        
            ptr=NULL;
            if((phase == 1) || (in_consensus>0))                                // If we just fill consensus admin list (phase 2) we ignore non-consensus records 
            {                    
                path+=4;
                meFCpy(&pldAdmin,&pldRow,sizeof(FC_PermissionLedgerRow));
                meFCpy(pldAdmin.m_Address,pldRow.m_Admin,FC_PLS_SIZE_ADDRESS);
                pldAdmin.m_Type=in_consensus;
                
                ptr=&pldAdmin;                                                  // Row with address replaced by admin

                found=m_TmpPool->Seek((unsigned char*)ptr);                                           
                if(found >= 0)
                {
                    path+=8;
                    ptr=(FC_PermissionLedgerRow *)m_TmpPool->GetRow(found);
                    if(ptr->m_Timestamp < pldAdmin.m_Timestamp)
                    {
                        if(ptr->m_GrantFrom == plsRow->m_BlockFrom)
                        {
                            if(ptr->m_GrantTo == plsRow->m_BlockTo)
                            {
                                required++;                                             // Duplicate record, compensate for decrementing below 
                            }            
                        }                
                        m_TmpPool->PutRow(found,&pldAdmin,(unsigned char*)&pldAdmin+m_Database->m_ValueOffset);
                        ptr=&pldAdmin;
                    }
                    else
                    {
                        ptr=NULL;                                                       // Ignore this record, we already have newer
                    }
                }
                else
                {
                    m_TmpPool->Add(&pldAdmin,(unsigned char*)&pldAdmin+m_Database->m_ValueOffset);            
                }        
            }    

            if(ptr)
            {
                path+=16;
                if(in_consensus)
                {
                    path+=32;
                    required--;    
                    if(required <= 0)
                    {
                        exit_now=1;
                    }
                }
            }
        }
        
        if(required > 0)
        {
            prevRow=pldRow.m_PrevRow;

            if(prevRow == 0)
            {
                exit_now=1;
            }                
        }
        else
        {
            exit_now=1;
        }        
    }
    
    for(i=0;i<m_TmpPool->m_Count;i++)
    {
        ptr=(FC_PermissionLedgerRow*)m_TmpPool->GetRow(i);
        
        meFCpy(plsDetails.m_Entity,plsRow->m_Entity,FC_PLS_SIZE_ENTITY);
        meFCpy(plsDetails.m_Address,plsRow->m_Address,FC_PLS_SIZE_ADDRESS);
        plsDetails.m_Type=plsRow->m_Type;
        plsDetails.m_BlockFrom=ptr->m_GrantFrom;
        plsDetails.m_BlockTo=ptr->m_GrantTo;
        plsDetails.m_Flags=ptr->m_Flags;
        plsDetails.m_LastRow=ptr->m_ThisRow;
        meFCpy(plsDetails.m_LastAdmin,ptr->m_Address,FC_PLS_SIZE_ADDRESS);
        plsDetails.m_RequiredAdmins=ptr->m_Type;
        
        plsDetailsBuffer->Add(&plsDetails,(unsigned char*)&plsDetails+m_Ledger->m_ValueOffset);
    }
    
    return FC_ERR_NOERROR;
}

/** Returns number of admins required for consensus */

int FC_Permissions::RequiredForConsensus(const void* lpEntity,const void* lpAddress,uint32_t type,uint32_t from,uint32_t to)
{
    FC_PermissionLedgerRow pldRow;
    FC_PermissionLedgerRow pldLast;
    int required;
    
    if(FC_IsNullEntity(lpEntity) == 0)
    {
        return 1;
    }
    
    GetPermission(lpEntity,lpAddress,type,&pldLast,1);
    
    switch(type)
    {
        case FC_PTP_ADMIN:
        case FC_PTP_MINE:
        case FC_PTP_CONNECT:
        case FC_PTP_RECEIVE:
        case FC_PTP_SEND:
        case FC_PTP_ISSUE:
        case FC_PTP_CREATE:
        case FC_PTP_ACTIVATE:
        case FC_PTP_WRITE:
            if(pldLast.m_ThisRow)
            {
                if(pldLast.m_BlockFrom == from)
                {
                    if(pldLast.m_BlockTo == to)
                    {
                        return 0;
                    }                    
                }
                                
                pldRow.Zero();
                
                if(lpEntity)
                {
                    meFCpy(pldRow.m_Entity,lpEntity,FC_PLS_SIZE_ENTITY);
                }
                meFCpy(pldRow.m_Address,lpAddress,FC_PLS_SIZE_ADDRESS);
                pldRow.m_Type=type;
                pldRow.m_PrevRow=pldLast.m_ThisRow;
                pldRow.m_BlockFrom=from;
                pldRow.m_BlockTo=to;
                pldRow.m_BlockReceived=0;
                pldRow.m_GrantFrom=from;
                pldRow.m_GrantTo=to;
                pldRow.m_ThisRow=m_Row;
                pldRow.m_Timestamp=FC_TimeNowAsUInt();
                pldRow.m_Flags=0;
//                pldRow.m_FoundInDB=pldLast.m_FoundInDB;
                
                if(VerifyConsensus(&pldRow,&pldLast,&required))
                {
                    return -2;
                }
            }    
            else
            {
                required=AdminConsensus(lpEntity,type);
            }
            break;
        default:
            return -1;
    }
    
    return required;
}

/** Returns permission details for specific record, external, locks */

FC_Buffer *FC_Permissions::GetPermissionDetails(FC_PermissionDetails *plsRow)
{
    FC_Buffer *result;
    
    Lock(0);
    
    result=new FC_Buffer;

    result->Initialize(m_Ledger->m_ValueOffset,sizeof(FC_PermissionDetails),FC_BUF_MODE_DEFAULT);

    if(m_Ledger->Open() <= 0)
    {
        LogString("Error: FillPermissionDetails: couldn't open ledger");
        goto exitlbl;
    }
        
    if(FillPermissionDetails(plsRow,result))
    {
        delete result;
        result=NULL;
    }

    m_Ledger->Close();

exitlbl:    
    UnLock();

    return result;
}

/** Returns list of upgrade approval */

FC_Buffer *FC_Permissions::GetUpgradeList(const void* lpUpgrade,FC_Buffer *old_buffer)
{
    if(lpUpgrade)
    {    
        unsigned char address[FC_PLS_SIZE_ADDRESS];

        memset(address,0,FC_PLS_SIZE_ADDRESS);
        meFCpy(address,lpUpgrade,FC_PLS_SIZE_UPGRADE);

        return GetPermissionList(upgrade_entity,address,FC_PTP_CONNECT | FC_PTP_UPGRADE,old_buffer);
    }
    
    return GetPermissionList(upgrade_entity,NULL,FC_PTP_CONNECT | FC_PTP_UPGRADE,old_buffer);    
}

/** Returns list of permission states */

FC_Buffer *FC_Permissions::GetPermissionList(const void* lpEntity,const void* lpAddress,uint32_t type,FC_Buffer *old_buffer)
{    
    FC_PermissionDBRow pdbRow;
    FC_PermissionLedgerRow pldRow;
    FC_PermissionDetails plsRow;
    unsigned char *ptr;
    FC_PermissionDetails *ptrFound;
    
    int dbvalue_len,err,i,cur_type,found,first,take_it;
    unsigned char null_entity[32];
    memset(null_entity,0,FC_PLS_SIZE_ENTITY);

    Lock(0);
    
    FC_Buffer *result;
    if(old_buffer == NULL)
    {
        result=new FC_Buffer;
                                                                                // m_Database->m_ValueOffset bytes are common for all structures
        result->Initialize(m_Database->m_ValueOffset,sizeof(FC_PermissionDetails),FC_BUF_MODE_MAP);        
    }
    else
    {
        result=old_buffer;
    }
    
    if(lpAddress)
    {
        cur_type=1;
        plsRow.Zero();
        
        if(lpEntity)
        {
            meFCpy(plsRow.m_Entity,lpEntity,FC_PLS_SIZE_ENTITY);
        }
        meFCpy(plsRow.m_Address,lpAddress,FC_PLS_SIZE_ADDRESS);
        for(i=0;i<32;i++)
        {
            if(cur_type & type)
            {
                GetPermission(lpEntity,lpAddress,cur_type,&pldRow,1);
                if(pldRow.m_ThisRow)
                {
                    plsRow.m_Type=pldRow.m_Type;
                    plsRow.m_BlockFrom=pldRow.m_BlockFrom;
                    plsRow.m_BlockTo=pldRow.m_BlockTo;
                    plsRow.m_Flags=pldRow.m_Flags;
                    plsRow.m_LastRow=pldRow.m_ThisRow;
                    result->Add(&plsRow,(unsigned char*)&plsRow+m_Database->m_ValueOffset);
                }
            }
            cur_type=cur_type<<1;                    
        }
    }
    else
    {    
        pdbRow.Zero();

        if(lpEntity)
        {
            meFCpy(pdbRow.m_Entity,lpEntity,FC_PLS_SIZE_ENTITY);
            pdbRow.m_Type=FC_PTP_CONNECT;
        }
        
        first=1;
        ptr=(unsigned char*)m_Database->m_DB->Read((char*)&pdbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,&dbvalue_len,FC_OPT_DB_DATABASE_SEEK_ON_READ,&err);
        if(err)
        {
            LogString("Error: GetPermissionList: db read");
            delete result;
            result=NULL;
            goto exitlbl;
        }

        if(ptr)
        {
            meFCpy((char*)&pdbRow+m_Database->m_ValueOffset,ptr,m_Database->m_ValueSize);
            while(ptr)
            {
                if(first == 0)
                {
                    if(type & pdbRow.m_Type)
                    {
                        plsRow.Zero();
                        meFCpy(plsRow.m_Entity,pdbRow.m_Entity,FC_PLS_SIZE_ENTITY);
                        meFCpy(plsRow.m_Address,pdbRow.m_Address,FC_PLS_SIZE_ADDRESS);
                        plsRow.m_Type=pdbRow.m_Type;
                        plsRow.m_BlockFrom=pdbRow.m_BlockFrom;
                        plsRow.m_BlockTo=pdbRow.m_BlockTo;
                        plsRow.m_Flags=pdbRow.m_Flags;
                        plsRow.m_LastRow=pdbRow.m_LedgerRow;
                        result->Add(&plsRow,(unsigned char*)&plsRow+m_Database->m_ValueOffset);
                    }
                }
                first=0;
                ptr=(unsigned char*)m_Database->m_DB->MoveNext(&err);
                if(ptr)
                {
                    meFCpy((char*)&pdbRow+m_Database->m_KeyOffset,ptr,m_Database->m_TotalSize);            
                    if(FC_IsNullEntity(lpEntity))
                    {
                        if(meFCmp(pdbRow.m_Entity,null_entity,FC_PLS_SIZE_ENTITY))
                        {
                            ptr=NULL;
                        }
                    }
                    else
                    {
                        if(meFCmp(pdbRow.m_Entity,lpEntity,FC_PLS_SIZE_ENTITY))
                        {
                            ptr=NULL;
                        }                        
                    }
                }
            }
        }

        for(i=0;i<m_MemPool->GetCount();i++)
        {
            ptr=m_MemPool->GetRow(i);
            pldRow.Zero();
            meFCpy((unsigned char*)&pldRow+m_Ledger->m_KeyOffset,ptr,m_Ledger->m_TotalSize);            
            take_it=0;
            if(FC_IsNullEntity(lpEntity))
            {
                if(meFCmp(pldRow.m_Entity,null_entity,FC_PLS_SIZE_ENTITY) == 0)
                {
                    take_it=1;
                }
            }
            else
            {                    
                if(meFCmp(pldRow.m_Entity,lpEntity,FC_PLS_SIZE_ENTITY) == 0)
                {
                    take_it=1;
                }
            }
            if(take_it)
            {
                if(type & pldRow.m_Type)
                {
                    found=result->Seek(&pldRow);
                    if(found >= 0)
                    {
                        ptrFound=(FC_PermissionDetails*)(result->GetRow(found));
                        ptrFound->m_BlockFrom=pldRow.m_BlockFrom;
                        ptrFound->m_BlockTo=pldRow.m_BlockTo;
                        ptrFound->m_Flags=pldRow.m_Flags;
                        ptrFound->m_LastRow=pldRow.m_ThisRow;
                    }
                    else
                    {
                        if(first == 0)
                        {
                            plsRow.Zero();
                            meFCpy(plsRow.m_Entity,pldRow.m_Entity,FC_PLS_SIZE_ENTITY);
                            meFCpy(plsRow.m_Address,pldRow.m_Address,FC_PLS_SIZE_ADDRESS);
                            plsRow.m_Type=pldRow.m_Type;
                            plsRow.m_BlockFrom=pldRow.m_BlockFrom;
                            plsRow.m_BlockTo=pldRow.m_BlockTo;
                            plsRow.m_Flags=pldRow.m_Flags;
                            plsRow.m_LastRow=pldRow.m_ThisRow;
                            result->Add(&plsRow,(unsigned char*)&plsRow+m_Database->m_ValueOffset);                    
                        }
                        first=0;
                    }
                }
                else
                {
                    first=0;                    
                }
            }
        }
    }
     
    if(m_Ledger->Open() <= 0)
    {
        LogString("Error: GetPermissionList: couldn't open ledger");
        goto exitlbl;
    }
        
    for(i=0;i<result->m_Count;i++)
    {
        FillPermissionDetails((FC_PermissionDetails*)result->GetRow(i),NULL);
    }

    m_Ledger->Close();
    
exitlbl:
    UnLock();
    
    return result;
}

/** Frees buffer returned by GetPermissionList */

void FC_Permissions::FreePermissionList(FC_Buffer *permissions)
{    
    if(permissions)
    {
        delete permissions;
    }
}

/** Returns 1 if Activate permission is ebough for granting/revoking permission of specified type */


int FC_Permissions::IsActivateEnough(uint32_t type)
{
    if(type & ( FC_PTP_ADMIN | FC_PTP_ISSUE | FC_PTP_MINE | FC_PTP_ACTIVATE | FC_PTP_CREATE))
    {
        return 0;
    }    
    return 1;
}

/** Sets permission record, external, locks */

int FC_Permissions::SetPermission(const void* lpEntity,const void* lpAddress,uint32_t type,const void* lpAdmin,
                                  uint32_t from,uint32_t to,uint32_t timestamp,uint32_t flags,int update_mempool,int offset)
{
    int result;
    
    if(FC_IsNullEntity(lpEntity) || ((flags & FC_PFL_ENTITY_GENESIS) == 0))
    {
        if(!CanAdmin(lpEntity,lpAdmin))
        {
            if(IsActivateEnough(type))
            {
                if(!CanActivate(lpEntity,lpAdmin))
                {
                    return FC_ERR_NOT_ALLOWED;                
                }
            }
            else
            {
                return FC_ERR_NOT_ALLOWED;
            }
        }    
    }
    
    Lock(1);
    result=SetPermissionInternal(lpEntity,lpAddress,type,lpAdmin,from,to,timestamp,flags,update_mempool,offset);
    UnLock();
    return result;
}

/** Sets approval record, external, locks */

int FC_Permissions::SetApproval(const void* lpUpgrade,uint32_t approval,const void* lpAdmin,uint32_t from,uint32_t timestamp,uint32_t flags,int update_mempool,int offset)
{
    int result=FC_ERR_NOERROR;
    FC_PermissionLedgerRow row;
    unsigned char address[FC_PLS_SIZE_ADDRESS];
    memset(address,0,FC_PLS_SIZE_ADDRESS);
    Lock(1);
    
    if(GetPermission(upgrade_entity,address,FC_PTP_CONNECT,&row,1) == 0)
    {
        result=SetPermissionInternal(upgrade_entity,address,FC_PTP_CONNECT,address,0,(uint32_t)(-1),timestamp, FC_PFL_ENTITY_GENESIS ,update_mempool,offset);        
    }
    
    
    if(result == FC_ERR_NOERROR)
    {
        meFCpy(address,lpUpgrade,FC_PLS_SIZE_UPGRADE);
        if(lpAdmin == NULL)
        {
            result=SetPermissionInternal(upgrade_entity,address,FC_PTP_CONNECT,address,from,approval ? (uint32_t)(-1) : 0,timestamp, flags,update_mempool,offset);                
        }
        else
        {
            result=SetPermissionInternal(upgrade_entity,address,FC_PTP_UPGRADE,lpAdmin,from,approval ? (uint32_t)(-1) : 0,timestamp, flags,update_mempool,offset);                            
        }
    }

    UnLock();    
    return result;
}

/** Sets permission record, internal */

int FC_Permissions::SetPermissionInternal(const void* lpEntity,const void* lpAddress,uint32_t type,const void* lpAdmin,uint32_t from,uint32_t to,uint32_t timestamp,uint32_t flags,int update_mempool,int offset)
{
    FC_PermissionLedgerRow pldRow;
    FC_PermissionLedgerRow pldLast;
    int err,i,num_types,thisBlock,lastAllowed,thisAllowed;        
    char msg[256];
    uint32_t types[9];
    uint32_t pr_entity,pr_address,pr_admin;
    num_types=0;
    types[num_types]=FC_PTP_CONNECT;num_types++;
    types[num_types]=FC_PTP_SEND;num_types++;
    types[num_types]=FC_PTP_RECEIVE;num_types++;
    if(FC_gState->m_Features->Streams())
    {
        types[num_types]=FC_PTP_WRITE;num_types++;        
        types[num_types]=FC_PTP_CREATE;num_types++;        
    }
    types[num_types]=FC_PTP_ISSUE;num_types++;
    types[num_types]=FC_PTP_MINE;num_types++;
    types[num_types]=FC_PTP_ACTIVATE;num_types++;        
    types[num_types]=FC_PTP_ADMIN;num_types++;        
    if(FC_gState->m_Features->Upgrades())
    {
        types[num_types]=FC_PTP_UPGRADE;num_types++;                        
    }
    
    err=FC_ERR_NOERROR;


    for(i=0;i<num_types;i++)
    {
        if(types[i] & type)
        {
            if(types[i] == type)
            {                
                lastAllowed=GetPermission(lpEntity,lpAddress,type,&pldLast,1);
                                
                thisBlock=m_Block+1;

                pldRow.Zero();
                if(lpEntity)
                {
                    meFCpy(pldRow.m_Entity,lpEntity,FC_PLS_SIZE_ENTITY);
                }
                meFCpy(pldRow.m_Address,lpAddress,FC_PLS_SIZE_ADDRESS);
                pldRow.m_Type=type;
                pldRow.m_PrevRow=pldLast.m_ThisRow;
                pldRow.m_BlockFrom=from;
                pldRow.m_BlockTo=to;
                if(m_AdminCount)
                {
                    meFCpy(pldRow.m_Admin,lpAdmin,FC_PLS_SIZE_ADDRESS);
                }
                else
                {
                    meFCpy(pldRow.m_Admin,lpAddress,FC_PLS_SIZE_ADDRESS);
                }
                pldRow.m_BlockReceived=thisBlock;
                pldRow.m_GrantFrom=from;
                pldRow.m_GrantTo=to;
                pldRow.m_ThisRow=m_Row;
                pldRow.m_Timestamp=timestamp;
                pldRow.m_Flags=flags;
                pldRow.m_Offset=offset;
//                pldRow.m_FoundInDB=pldLast.m_FoundInDB;
                
                err=VerifyConsensus(&pldRow,&pldLast,NULL);
                
                if(err)
                {
                    return err;
                }
                
                if(update_mempool)
                {
                    if(m_MemPool->GetCount() == 0)
                    {
                        if(m_CopiedRow == 0)
                        {
                            m_ClearedAdminCount=m_AdminCount;
                            m_ClearedMinerCount=m_MinerCount;
                            m_ClearedMinerCountForMinerVerification=m_MinerCount;
                        }
                    }
                    m_MemPool->Add((unsigned char*)&pldRow+m_Ledger->m_KeyOffset,(unsigned char*)&pldRow+m_Ledger->m_ValueOffset);
                    m_Row++;                

                    if( (type == FC_PTP_ADMIN) || (type == FC_PTP_MINE))
                    {
                        if(FC_IsNullEntity(lpEntity))
                        {
                            thisAllowed=GetPermission(lpEntity,lpAddress,type);
                            if(lastAllowed != thisAllowed)
                            {
                                if(lastAllowed)
                                {
                                    thisAllowed=-1;
                                }
                                else
                                {
                                    thisAllowed=1;
                                }
                                if(type == FC_PTP_ADMIN)
                                {
                                    m_AdminCount+=thisAllowed;
                                }
                                if(type == FC_PTP_MINE)
                                {
                                    m_MinerCount+=thisAllowed;
                                }                        
                            }
                        }
                        
/*                        
                        if(type == FC_PTP_ADMIN)
                        {
                            sprintf(msg,"Admin grant: (%08x-%08x-%08x) (%ld-%ld, %ld), In consensus: %d, Admin count: %d, Miner count: %d",
                                    *(uint32_t*)((void*)pldRow.m_Entity),*(uint32_t*)((void*)pldRow.m_Address),*(uint32_t*)((void*)pldRow.m_Admin),
                                    (int64_t)from,(int64_t)to,(int64_t)timestamp,pldRow.m_Consensus,m_AdminCount,m_MinerCount);
                        }
                        if(type == FC_PTP_MINE)
                        {
                            sprintf(msg,"Miner grant: (%08x-%08x-%08x) (%ld-%ld, %ld), In consensus: %d, Admin count: %d, Miner count: %d",
                                    *(uint32_t*)((void*)pldRow.m_Entity),*(uint32_t*)((void*)pldRow.m_Address),*(uint32_t*)((void*)pldRow.m_Admin),
                                    (int64_t)from,(int64_t)to,(int64_t)timestamp,pldRow.m_Consensus,m_AdminCount,m_MinerCount);
                        }
 */ 
                    }                
                    meFCpy(&pr_entity,pldRow.m_Entity,sizeof(uint32_t));
                    meFCpy(&pr_address,pldRow.m_Address,sizeof(uint32_t));
                    meFCpy(&pr_admin,pldRow.m_Admin,sizeof(uint32_t));
                    sprintf(msg,"Grant: (%08x-%08x-%08x-%08x) (%ld-%ld, %ld), In consensus: %d, Admin count: %d, Miner count: %d",
                            pr_entity,pr_address,pldRow.m_Type,pr_admin,
                            (int64_t)from,(int64_t)to,(int64_t)timestamp,pldRow.m_Consensus,m_AdminCount,m_MinerCount);
                    LogString(msg);                        
                }
            }
            else
            {
                err=SetPermissionInternal(lpEntity,lpAddress,types[i],lpAdmin,from,to,timestamp,flags,update_mempool,offset);
                if(err)
                {
                    return err;
                }
            }
        }
    }
    return err;
}

/** Sets mempool checkpoint */


int FC_Permissions::SetCheckPoint()
{
    Lock(1);
    
    m_CheckPointRow=m_Row;
    m_CheckPointAdminCount=m_AdminCount;
    m_CheckPointMinerCount=m_MinerCount;
    m_CheckPointMemPoolSize=m_MemPool->GetCount();
    
    UnLock();
    
    return FC_ERR_NOERROR;
}

/** Rolls back to mempool checkpoint */

int FC_Permissions::RollBackToCheckPoint()
{
    Lock(1);
    
    m_Row=m_CheckPointRow;
    m_AdminCount=m_CheckPointAdminCount;
    m_MinerCount=m_CheckPointMinerCount;
    m_MemPool->SetCount(m_CheckPointMemPoolSize);

    UnLock();
    
    return FC_ERR_NOERROR;
}


/** Returns address of the specific block miner by height*/

int FC_Permissions::GetBlockMiner(uint32_t block,unsigned char* lpMiner)
{
    int err;
    int64_t last_block_row;
    FC_BlockLedgerRow pldRow;
    FC_BlockLedgerRow pldLast;    
    uint32_t type;
            
    Lock(0);
    type=FC_PTP_BLOCK_INDEX;        

    pldLast.Zero();
    pldRow.Zero();
    
    sprintf((char*)pldRow.m_Address,"Block %08X row",block);
    
    GetPermission(pldRow.m_Address,type,(FC_PermissionLedgerRow*)&pldLast);
            
    last_block_row=pldLast.m_ThisRow;
    
    if(last_block_row <= 1)
    {
        LogString("Error: GetBlockMiner: block row not found");
        err=FC_ERR_NOT_FOUND;
        goto exitlbl;
    }
    
    last_block_row--;
    err=FC_ERR_NOERROR;
    
    if(m_Ledger->Open() <= 0)
    {
        LogString("Error: GetBlockMiner: couldn't open ledger");
        err=FC_ERR_FILE_READ_ERROR;
        goto exitlbl;
    }

    m_Ledger->GetRow(last_block_row,(FC_PermissionLedgerRow*)&pldRow);

    if(pldRow.m_Type == FC_PTP_BLOCK_MINER)
    {
        meFCpy(lpMiner,pldRow.m_Address,FC_PLS_SIZE_ADDRESS);
    }
    else
    {
        LogString("Error: GetBlockMiner: row type mismatch");
        err=FC_ERR_CORRUPTED;
    }
    
    m_Ledger->Close();

exitlbl:
    UnLock();

    return err;    
}


/** Verifies if block of specific height has specified hash */

int FC_Permissions::VerifyBlockHash(int32_t block,const void* lpHash)
{
    int type;
    int64_t last_block_row;
    int result;
    
    FC_BlockLedgerRow pldRow;
    FC_BlockLedgerRow pldLast;
    
    type=FC_PTP_BLOCK_INDEX;        

    pldRow.Zero();
    pldLast.Zero();
    sprintf((char*)(pldRow.m_Address),"Block %08X row",block);
    
    GetPermission(pldRow.m_Address,type,(FC_PermissionLedgerRow*)&pldLast);
            
    last_block_row=pldLast.m_ThisRow;
    
    if(last_block_row == 0)
    {
        return 0;
    }
        
    if(m_Ledger->Open() <= 0)
    {
        LogString("Error: VerifyBlockHash: couldn't open ledger");
        return 0;
    }

    m_Ledger->GetRow(last_block_row,(FC_PermissionLedgerRow*)&pldRow);

    result=1;
    if(meFCmp(pldRow.m_CommitHash,lpHash,FC_PLS_SIZE_HASH))
    {
        result=0;
    }
        
    m_Ledger->Close();

    return result;
}

/** Commits block, external, locks */

int FC_Permissions::Commit(const void* lpMiner,const void* lpHash)
{
    int result;
    Lock(1);
    result=CommitInternal(lpMiner,lpHash);
    UnLock();
    return result;
}

/** Finds blocks in range with governance model change */

uint32_t FC_Permissions::FindGovernanceModelChange(uint32_t from,uint32_t to)
{
    int64_t last_block_row;
    FC_BlockLedgerRow pldRow;
    FC_BlockLedgerRow pldLast;    
    uint32_t type;
    uint32_t block,start;
    uint32_t found=0;
            
    Lock(0);
    
    type=FC_PTP_BLOCK_INDEX;        

    start=from;
    if(start == 0)
    {
        start=1;
    }
    block=to;
    if((int)block > m_Block)
    {
        block=m_Block;
    }
    
    if(block < from)
    {
        goto exitlbl;
    }
    
    pldLast.Zero();
    pldRow.Zero();
    
    sprintf((char*)pldRow.m_Address,"Block %08X row",block);

    GetPermission(pldRow.m_Address,type,(FC_PermissionLedgerRow*)&pldLast);
         
    if(m_Ledger->Open() <= 0)
    {
        LogString("Error: FindGovernanceModelChange: couldn't open ledger");
        goto exitlbl;
    }
    
    last_block_row=pldLast.m_ThisRow;
    while( (block >= start) && (found == 0) )
    {
        m_Ledger->GetRow(last_block_row,(FC_PermissionLedgerRow*)&pldRow);
        if(pldRow.m_BlockFlags & FC_PFB_GOVERNANCE_CHANGE)
        {
            found=block;
            goto exitlbl;
        }
        block--;
        last_block_row=pldRow.m_PrevRow;
    }
    
    m_Ledger->Close();

exitlbl:
    UnLock();

    return found;
}

/** Calculate block flags before commit */

uint32_t FC_Permissions::CalculateBlockFlags()
{
    int i,mprow;
    uint32_t flags=FC_PFB_NONE;
    FC_PermissionLedgerRow pldRow;
    FC_PermissionLedgerRow pldLast;
    FC_PermissionLedgerRow pldNext;
    
    for(i=0;i<m_MemPool->GetCount();i++)
    {
        if( (flags & FC_PFB_GOVERNANCE_CHANGE) == 0)
        {
            meFCpy((unsigned char*)&pldRow+m_Ledger->m_KeyOffset,m_MemPool->GetRow(i),m_Ledger->m_TotalSize);

            if( (pldRow.m_Type & (FC_PTP_ADMIN | FC_PTP_MINE)) &&
                (FC_IsNullEntity(pldRow.m_Entity)) &&
                (pldRow.m_Consensus > 0) )
            {                    
                meFCpy((unsigned char*)&pldNext,(unsigned char*)&pldRow,m_Ledger->m_TotalSize);
                pldNext.m_PrevRow=pldRow.m_ThisRow;
                mprow=0;
                while(mprow>=0)
                {
                    mprow=m_MemPool->Seek((unsigned char*)&pldNext+m_Ledger->m_KeyOffset);
                    if(mprow>=0)
                    {
                        meFCpy((unsigned char*)&pldNext+m_Ledger->m_KeyOffset,m_MemPool->GetRow(mprow),m_Ledger->m_TotalSize);
                        if(pldNext.m_Consensus)
                        {
                            mprow=-1;
                        }                            
                        pldNext.m_PrevRow=pldNext.m_ThisRow;
                    }
                }
                if( (pldNext.m_Consensus == 0) || (pldNext.m_ThisRow == pldRow.m_ThisRow) )
                {
                    pldLast.Zero();
                    GetPermission(pldRow.m_Entity,pldRow.m_Address,pldRow.m_Type,&pldLast,0);            
                    if(pldLast.m_ThisRow)
                    {
                        if(pldRow.m_BlockFrom != pldLast.m_BlockFrom)
                        {
                            if( ((uint32_t)(m_Block+1) < pldRow.m_BlockFrom) || ((uint32_t)(m_Block+1) < pldLast.m_BlockFrom) )
                            {
                                flags |= FC_PFB_GOVERNANCE_CHANGE;
                            }
                        }
                        if(pldRow.m_BlockTo != pldLast.m_BlockTo)
                        {
                            if( ((uint32_t)(m_Block+1) < pldRow.m_BlockTo) || ((uint32_t)(m_Block+1) < pldLast.m_BlockTo) )
                            {
                                flags |= FC_PFB_GOVERNANCE_CHANGE;
                            }
                        }
                    }
                    else
                    {
                        if(pldRow.m_BlockFrom < pldRow.m_BlockTo)
                        {
                            if( (uint32_t)(m_Block+1) < pldRow.m_BlockTo ) 
                            {
                                flags |= FC_PFB_GOVERNANCE_CHANGE;
                            }
                        }
                    }
                }
            }
        }
    }    
    
    
    return flags;
}

/** Commits block, external, no lock */

int FC_Permissions::CommitInternal(const void* lpMiner,const void* lpHash)
{
    int i,err,thisBlock,value_len,pld_items;
    uint32_t block_flags;
    char msg[256];
    
    FC_PermissionDBRow pdbRow;
    FC_PermissionDBRow pdbAdminMinerRow;
    FC_PermissionLedgerRow pldRow;
    FC_BlockLedgerRow pldBlockRow;
    FC_BlockLedgerRow pldBlockLast;
    
    unsigned char *ptr;
    
    err=FC_ERR_NOERROR;
    
    pdbAdminMinerRow.Zero();
    meFCpy(pdbAdminMinerRow.m_Entity,adminminerlist_entity,FC_PLS_SIZE_ENTITY);
    
    block_flags=CalculateBlockFlags();
    pld_items=m_MemPool->GetCount();
    
    if(lpMiner)
    {
        thisBlock=m_Block+1;
        
        GetPermission(lpMiner,FC_PTP_BLOCK_MINER,(FC_PermissionLedgerRow*)&pldBlockLast);
        
        pldBlockRow.Zero();
        meFCpy(pldBlockRow.m_Address,lpMiner,FC_PLS_SIZE_ADDRESS);
        pldBlockRow.m_Type=FC_PTP_BLOCK_MINER;
        pldBlockRow.m_PrevRow=pldBlockLast.m_ThisRow;
        pldBlockRow.m_BlockFrom=thisBlock;
        pldBlockRow.m_BlockTo=thisBlock;
        pldBlockRow.m_BlockReceived=thisBlock;
        meFCpy(pldBlockRow.m_CommitHash,lpHash,FC_PLS_SIZE_HASH);
        pldBlockRow.m_ThisRow=m_Row;
        pldBlockRow.m_BlockFlags=pldBlockLast.m_BlockFlags & FC_PFB_MINED_BEFORE;
        pldBlockRow.m_BlockFlags |= block_flags;
        m_MemPool->Add((unsigned char*)&pldBlockRow+m_Ledger->m_KeyOffset,(unsigned char*)&pldBlockRow+m_Ledger->m_ValueOffset);
        m_Row++;                
        
        pldBlockRow.Zero();
        sprintf((char*)(pldBlockRow.m_Address),"Block %08X row",thisBlock);
        pldBlockRow.m_Type=FC_PTP_BLOCK_INDEX;
        pldBlockRow.m_PrevRow=m_Row-m_MemPool->GetCount()-1;
        pldBlockRow.m_BlockFrom=thisBlock;
        pldBlockRow.m_BlockTo=thisBlock;
        pldBlockRow.m_BlockReceived=thisBlock;
        meFCpy(pldBlockRow.m_CommitHash,lpHash,FC_PLS_SIZE_HASH);
        pldBlockRow.m_ThisRow=m_Row;
        pldBlockRow.m_BlockFlags=pldBlockLast.m_BlockFlags & FC_PFB_MINED_BEFORE;
        pldBlockRow.m_BlockFlags |= block_flags;
        m_MemPool->Add((unsigned char*)&pldBlockRow+m_Ledger->m_KeyOffset,(unsigned char*)&pldBlockRow+m_Ledger->m_ValueOffset);
        m_Row++;                                
    }
    
    if(m_Ledger->Open() <= 0)
    {
        LogString("Error: Commit: couldn't open ledger");
        return FC_ERR_DBOPEN_ERROR;
    }

    thisBlock=m_Block+1;
    if(m_MemPool->GetCount())
    {
        for(i=0;i<pld_items;i++)
        {
            meFCpy((unsigned char*)&pldRow+m_Ledger->m_KeyOffset,m_MemPool->GetRow(i),m_Ledger->m_TotalSize);
            if(i)
            {
                m_Ledger->WriteRow(&pldRow);
            }
            else                
            {
                m_Ledger->SetRow(m_Row-m_MemPool->GetCount(),&pldRow);
            }
        }
        
        if(err == FC_ERR_NOERROR)
        {
            for(i=0;i<m_MemPool->GetCount();i++)
            {
                if(err == FC_ERR_NOERROR)
                {
                    meFCpy((unsigned char*)&pldRow+m_Ledger->m_KeyOffset,m_MemPool->GetRow(i),m_Ledger->m_TotalSize);

                    pdbRow.Zero();
                    meFCpy(pdbRow.m_Entity,pldRow.m_Entity,FC_PLS_SIZE_ENTITY);
                    meFCpy(pdbRow.m_Address,pldRow.m_Address,FC_PLS_SIZE_ADDRESS);
                    pdbRow.m_Type=pldRow.m_Type;
                    pdbRow.m_BlockFrom=pldRow.m_BlockFrom;
                    pdbRow.m_BlockTo=pldRow.m_BlockTo;
                    pdbRow.m_LedgerRow=pldRow.m_ThisRow;
                    pdbRow.m_Flags=pldRow.m_Flags;

/*                    
                    if(pldRow.m_FoundInDB == 0 )
                    {
                        m_DBRowCount++;
                    }
 */ 
                    err=m_Database->m_DB->Write((char*)&pdbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,
                                                (char*)&pdbRow+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
                    if(err)
                    {
                        LogString("Error: Commit: DB write error");                        
                    }
                    
                    if( (pdbRow.m_Type == FC_PTP_ADMIN) || (pdbRow.m_Type == FC_PTP_MINE))
                    {
                        if(meFCmp(pdbRow.m_Entity,null_entity,FC_PLS_SIZE_ENTITY) == 0)
                        {                    
                            pdbAdminMinerRow.m_Type=pdbRow.m_Type;
                            meFCpy(pdbAdminMinerRow.m_Address,pdbRow.m_Address,FC_PLS_SIZE_ADDRESS);
                            err=m_Database->m_DB->Write((char*)&pdbAdminMinerRow+m_Database->m_KeyOffset,m_Database->m_KeySize,
                                                        (char*)&pdbAdminMinerRow+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
                        }
                    }
                }                
            }
        }
    }
    
    if(err == FC_ERR_NOERROR)
    {
        pdbRow.Zero();
        
        ptr=(unsigned char*)m_Database->m_DB->Read((char*)&pdbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,&value_len,0,&err);
        if(ptr)
        {
            meFCpy((char*)&pdbRow+m_Database->m_ValueOffset,ptr,m_Database->m_ValueSize);
        }
/*        
        else
        {
            m_DBRowCount++;            
        }
*/
        pdbRow.m_BlockTo=thisBlock;
        pdbRow.m_LedgerRow=m_Row;
        err=m_Database->m_DB->Write((char*)&pdbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,
                                    (char*)&pdbRow+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
        if(err)
        {
            LogString("Error: Commit: DB write error (0)");                                    
        }
    }        
    
    if(err == FC_ERR_NOERROR)
    {
        for(i=pld_items;i<m_MemPool->GetCount();i++)
        {
            meFCpy((unsigned char*)&pldBlockRow+m_Ledger->m_KeyOffset,m_MemPool->GetRow(i),m_Ledger->m_TotalSize);
            if(i)
            {
                m_Ledger->WriteRow((FC_PermissionLedgerRow*)&pldBlockRow);
            }
            else                
            {
                m_Ledger->SetRow(m_Row-m_MemPool->GetCount(),(FC_PermissionLedgerRow*)&pldBlockRow);
            }
        }        
    }    
    
    if(err == FC_ERR_NOERROR)
    {
        m_Ledger->GetRow(0,&pldRow);

        thisBlock=m_Block+1;
        pldRow.m_BlockTo=thisBlock;
        pldRow.m_PrevRow=m_Row;
        m_Ledger->SetRow(0,&pldRow);
    }
    
    m_Ledger->Flush();
    m_Ledger->Close();
    
    if(err == FC_ERR_NOERROR)
    {
        err=m_Database->m_DB->Commit(FC_OPT_DB_DATABASE_TRANSACTIONAL);
        if(err)
        {
            LogString("Error: Commit: DB commit error");                                    
        }
    }    

    if(m_Ledger->Open() <= 0)
    {
        LogString("Error: Commit: couldn't open ledger");
        return FC_ERR_DBOPEN_ERROR;
    }

    if(err == FC_ERR_NOERROR)
    {
        m_Block++;
        UpdateCounts();        
        m_Block--;
        m_ClearedAdminCount=m_AdminCount;
        m_ClearedMinerCount=m_MinerCount;
        m_ClearedMinerCountForMinerVerification=m_MinerCount;
        for(i=pld_items;i<m_MemPool->GetCount();i++)
        {
            m_Ledger->GetRow(m_Row-m_MemPool->GetCount()+i,(FC_PermissionLedgerRow*)&pldBlockRow);
            pldBlockRow.m_AdminCount=m_AdminCount;
            pldBlockRow.m_MinerCount=m_MinerCount;
            m_Ledger->SetRow(m_Row-m_MemPool->GetCount()+i,(FC_PermissionLedgerRow*)&pldBlockRow);
        }        
    }    
    m_Ledger->Close();
    
    
    if(err)
    {
        RollBackInternal(m_Block);        
    }
    else
    {
        m_MemPool->Clear();
        StoreBlockInfoInternal(lpMiner,lpHash,0);
        m_Block++;
    }

    
    sprintf(msg,"Block commit: %9d (Hash: %08x, Miner: %08x), Admin count: %d, Miner count: %d, Ledger Rows: %ld",
            m_Block,*(uint32_t*)lpHash,*(uint32_t*)lpMiner,m_AdminCount,m_MinerCount,m_Row);
    LogString(msg);
    return err;
}

/** Returns address of the specific block by hash */

int FC_Permissions::GetBlockMiner(const void* lpHash, unsigned char* lpMiner,uint32_t *lpAdminMinerCount)
{
    int err,value_len;
    FC_BlockMinerDBRow pdbBlockMinerRow;
    
    unsigned char *ptr;
    
    Lock(0);
    
    err=FC_ERR_NOERROR;
    pdbBlockMinerRow.Zero();
    meFCpy(pdbBlockMinerRow.m_BlockHash,lpHash,FC_PLS_SIZE_ENTITY);
    pdbBlockMinerRow.m_Type=FC_PTP_BLOCK_MINER;

    ptr=(unsigned char*)m_Database->m_DB->Read((char*)&pdbBlockMinerRow+m_Database->m_KeyOffset,m_Database->m_KeySize,&value_len,0,&err);
    if(ptr)
    {
        meFCpy((char*)&pdbBlockMinerRow+m_Database->m_ValueOffset,ptr,m_Database->m_ValueSize);        
        meFCpy(lpMiner,pdbBlockMinerRow.m_Address,FC_PLS_SIZE_ADDRESS);
        *lpAdminMinerCount=pdbBlockMinerRow.m_AdminMinerCount;
    }
    else
    {
        err=FC_ERR_NOT_FOUND;
    }
    
    UnLock();

    return err;
}

int FC_Permissions::GetBlockAdminMinerGrants(const void* lpHash, int record, int32_t* offsets)
{
    int err,value_len;
    FC_AdminMinerGrantDBRow pdbAdminMinerGrantRow;
    
    unsigned char *ptr;
    
    Lock(0);
    
    err=FC_ERR_NOERROR;
    
    pdbAdminMinerGrantRow.Zero();
    meFCpy(pdbAdminMinerGrantRow.m_BlockHash,lpHash,FC_PLS_SIZE_ENTITY);
    pdbAdminMinerGrantRow.m_Type=FC_PTP_BLOCK_MINER;
    pdbAdminMinerGrantRow.m_RecordID=record;

    ptr=(unsigned char*)m_Database->m_DB->Read((char*)&pdbAdminMinerGrantRow+m_Database->m_KeyOffset,m_Database->m_KeySize,&value_len,0,&err);
    
    if(ptr)
    {
        meFCpy((char*)&pdbAdminMinerGrantRow+m_Database->m_ValueOffset,ptr,m_Database->m_ValueSize);        
        meFCpy(offsets,pdbAdminMinerGrantRow.m_Offsets,FC_PLS_SIZE_OFFSETS_PER_ROW*sizeof(int32_t));
    }
    else
    {
        err=FC_ERR_NOT_FOUND;
    }
    
    UnLock();

    return err;    
}

int FC_Permissions::IncrementBlock(uint32_t admin_miner_count)
{
    Lock(1);
    m_Block++;
    if(admin_miner_count)
    {
        m_AdminCount=(admin_miner_count >> 16) & 0xffff;
        m_MinerCount=admin_miner_count & 0xffff;
    }
    else
    {
        UpdateCounts();        
    }
    if(m_CopiedRow > 0)
    {
        m_ClearedMinerCountForMinerVerification=m_MinerCount;
    }
/*    
    char msg[256];
    sprintf(msg,"Verifier Increment: %9d, Admin count: %d, Miner count: %d, Ledger Rows: %ld",
            m_Block,m_AdminCount,m_MinerCount,m_Row);
    LogString(msg);
*/    
    UnLock();
    return FC_ERR_NOERROR;
}


/** Stores info about block miner and admin/miner grant transactions */

int FC_Permissions::StoreBlockInfo(const void* lpMiner,const void* lpHash)
{    
    int result;
    Lock(1);
    result=StoreBlockInfoInternal(lpMiner,lpHash,1);
    m_Block++;
    UnLock();
    return result;
}

void FC_Permissions::SaveTmpCounts()
{
    Lock(1);
    m_TmpSavedAdminCount=m_AdminCount;
    m_TmpSavedMinerCount=m_MinerCount;
    UnLock();    
}

int FC_Permissions::StoreBlockInfoInternal(const void* lpMiner,const void* lpHash,int update_counts)
{    
    int i,err,amg_items,last_offset;
    
    FC_PermissionDBRow pdbAdminMinerRow;
    FC_PermissionLedgerRow pldRow;
    FC_BlockMinerDBRow pdbBlockMinerRow;
    FC_AdminMinerGrantDBRow pdbAdminMinerGrantRow;
    
    if(FC_gState->m_Features->CachedInputScript() == 0)
    {
        return FC_ERR_NOERROR;
    }
    
    if(FC_gState->m_NetworkParams->GetInt64Param("supportminerprecheck") == 0)                                
    {
        return FC_ERR_NOERROR;        
    }    
    
    if(FCP_ANYONE_CAN_MINE)                                
    {
        return FC_ERR_NOERROR;        
    }    
    
    err=FC_ERR_NOERROR;
    
    amg_items=0;
    last_offset=0;
    pdbAdminMinerRow.Zero();
    meFCpy(pdbAdminMinerRow.m_Entity,adminminerlist_entity,FC_PLS_SIZE_ENTITY);
    pdbAdminMinerGrantRow.Zero();
    meFCpy(pdbAdminMinerGrantRow.m_BlockHash,lpHash,FC_PLS_SIZE_ENTITY);
    pdbAdminMinerGrantRow.m_Type=FC_PTP_BLOCK_MINER;
    
    for(i=0;i<m_MemPool->GetCount();i++)
    {
        if(err == FC_ERR_NOERROR)
        {
            meFCpy((unsigned char*)&pldRow+m_Ledger->m_KeyOffset,m_MemPool->GetRow(i),m_Ledger->m_TotalSize);

            if( (pldRow.m_Type == FC_PTP_ADMIN) || (pldRow.m_Type == FC_PTP_MINE))
            {
                if(meFCmp(pldRow.m_Entity,null_entity,FC_PLS_SIZE_ENTITY) == 0)
                {                    
                    if( pldRow.m_BlockReceived == (uint32_t)(m_Block+1) )
                    {                       
                        pdbAdminMinerRow.m_Type=pldRow.m_Type;
                        meFCpy(pdbAdminMinerRow.m_Address,pldRow.m_Address,FC_PLS_SIZE_ADDRESS);
                        err=m_Database->m_DB->Write((char*)&pdbAdminMinerRow+m_Database->m_KeyOffset,m_Database->m_KeySize,
                                                    (char*)&pdbAdminMinerRow+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
                        if(err)
                        {
                            LogString("Error: StoreBlockInfoInternal: DB write error");                        
                        }

                        if(pldRow.m_Offset != last_offset)
                        {
                            if( (amg_items > 0) && ((amg_items % FC_PLS_SIZE_OFFSETS_PER_ROW) == 0) )
                            {
                                err=m_Database->m_DB->Write((char*)&pdbAdminMinerGrantRow+m_Database->m_KeyOffset,m_Database->m_KeySize,
                                                            (char*)&pdbAdminMinerGrantRow+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
                                if(err)
                                {
                                    LogString("Error: StoreBlockInfoInternal: DB write error");                        
                                }
                                memset(pdbAdminMinerGrantRow.m_Offsets,0,FC_PLS_SIZE_OFFSETS_PER_ROW*sizeof(int32_t));
                            }                    
                            pdbAdminMinerGrantRow.m_RecordID=(amg_items / FC_PLS_SIZE_OFFSETS_PER_ROW) + 1;
                            pdbAdminMinerGrantRow.m_Offsets[amg_items % FC_PLS_SIZE_OFFSETS_PER_ROW]=pldRow.m_Offset;
                            last_offset=pldRow.m_Offset;
                            amg_items++;
                        }
                    }
                }                
            }
        }                
    }
    
    if(err == FC_ERR_NOERROR)
    {
        if(amg_items > 0)
        {
            err=m_Database->m_DB->Write((char*)&pdbAdminMinerGrantRow+m_Database->m_KeyOffset,m_Database->m_KeySize,
                                        (char*)&pdbAdminMinerGrantRow+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
            if(err)
            {
                LogString("Error: StoreBlockInfoInternal: DB write error");                        
            }
        }                            
    }
    
    if(err == FC_ERR_NOERROR)
    {
        err=m_Database->m_DB->Commit(FC_OPT_DB_DATABASE_TRANSACTIONAL);
        if(err)
        {
            LogString("Error: StoreBlockInfoInternal: DB commit error");                                    
        }
    }
    
    if(update_counts)
    {
        m_Block++;
        UpdateCounts();
        m_ClearedMinerCountForMinerVerification=m_MinerCount;
        m_Block--;
    }
    
    pdbBlockMinerRow.Zero();
    meFCpy(pdbBlockMinerRow.m_BlockHash,lpHash,FC_PLS_SIZE_ENTITY);
    pdbBlockMinerRow.m_Type=FC_PTP_BLOCK_MINER;
    pdbBlockMinerRow.m_AdminMinerCount=0;
    if( (m_AdminCount <= 0xffff) && (m_MinerCount <= 0xffff) )
    {
        pdbBlockMinerRow.m_AdminMinerCount = (m_AdminCount << 16) + m_MinerCount;
    }
    meFCpy(pdbBlockMinerRow.m_Address,lpMiner,FC_PLS_SIZE_ADDRESS);
    err=m_Database->m_DB->Write((char*)&pdbBlockMinerRow+m_Database->m_KeyOffset,m_Database->m_KeySize,
                                (char*)&pdbBlockMinerRow+m_Database->m_ValueOffset,m_Database->m_ValueSize,0);
    if(err)
    {
        LogString("Error: StoreBlockInfoInternal: DB write error");                        
    }
    
    if(err == FC_ERR_NOERROR)
    {
        err=m_Database->m_DB->Commit(0);
        if(err)
        {
            LogString("Error: StoreBlockInfoInternal: DB commit error");                                    
        }
    }
    
/*    
    char msg[256];

    sprintf(msg,"Block store: %9d (Hash: %08x, Miner: %08x), Admin count: %d, Miner count: %d, Ledger Rows: %ld",
            m_Block+1,*(uint32_t*)lpHash,*(uint32_t*)lpMiner,m_AdminCount,m_MinerCount,m_Row);
    LogString(msg);
*/    
    return FC_ERR_NOERROR;
}

/** Rolls one block back */

int FC_Permissions::RollBack()
{
    return RollBack(m_Block-1);
}

/** Rolls back to specific height, external, locks */

int FC_Permissions::RollBack(int block)
{
    int result;
    Lock(1);
    result=RollBackInternal(block);
    UnLock();
    return result;    
}

/** Rolls back to specific height, internal, no lock */

int FC_Permissions::RollBackInternal(int block)
{
    int err;
    int take_it,value_len;
    uint64_t this_row,prev_row;
    FC_PermissionDBRow pdbRow;
    FC_PermissionLedgerRow pldRow;
    unsigned char *ptr;
    char msg[256];
    
    err=FC_ERR_NOERROR;
    
    if(m_Ledger->Open() <= 0)
    {
        LogString("Error: Rollback: couldn't open ledger");
        return FC_ERR_DBOPEN_ERROR;
    }

    ClearMemPoolInternal();
    
    this_row=m_Row-1;
    take_it=1;
    if(this_row == 0)
    {
        take_it=0;
    }
    
    while(take_it && (this_row>0))
    {
        m_Ledger->GetRow(this_row,&pldRow);
        
        if((int32_t)pldRow.m_BlockReceived > block)
        {
            pdbRow.Zero();
            meFCpy(pdbRow.m_Entity,pldRow.m_Entity,FC_PLS_SIZE_ENTITY);
            meFCpy(pdbRow.m_Address,pldRow.m_Address,FC_PLS_SIZE_ADDRESS);
            pdbRow.m_Type=pldRow.m_Type;
            prev_row=pldRow.m_PrevRow;
            if(prev_row)
            {
                m_Ledger->GetRow(prev_row,&pldRow);
                pdbRow.m_BlockFrom=pldRow.m_BlockFrom;
                pdbRow.m_BlockTo=pldRow.m_BlockTo;
                pdbRow.m_LedgerRow=prev_row;
                pdbRow.m_Flags=pldRow.m_Flags;
                err=m_Database->m_DB->Write((char*)&pdbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,
                                            (char*)&pdbRow+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);                
                if(err)
                {
                    LogString("Error: Rollback: db write error");                    
                }
            }
            else
            {
//                m_DBRowCount--;
                err=m_Database->m_DB->Delete((char*)&pdbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
                if(err)
                {
                    LogString("Error: Rollback: db delete error");                    
                }
            }
        }
        else
        {
            take_it=0;
        }
        if(err)
        {
            take_it=0;
        }
        if(take_it)
        {
            this_row--;
        }
    }
        
    this_row++;

    if(err == FC_ERR_NOERROR)
    {
        pdbRow.Zero();
        
        ptr=(unsigned char*)m_Database->m_DB->Read((char*)&pdbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,&value_len,0,&err);
        if(ptr)
        {
            meFCpy((char*)&pdbRow+m_Database->m_ValueOffset,ptr,m_Database->m_ValueSize);
        }
/*        
        else
        {
            m_DBRowCount++;            
        }
*/
        pdbRow.m_BlockTo=block;
        pdbRow.m_LedgerRow=this_row;
        err=m_Database->m_DB->Write((char*)&pdbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,
                                    (char*)&pdbRow+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);        
        if(err)
        {
            LogString("Error: Rollback: db write error (0)");                    
        }
    }        
    
    if(err == FC_ERR_NOERROR)
    {
        err=m_Database->m_DB->Commit(FC_OPT_DB_DATABASE_TRANSACTIONAL);
    }    
    
    if(err == FC_ERR_NOERROR)
    {
        m_Ledger->GetRow(0,&pldRow);

        pldRow.m_BlockTo=block;
        pldRow.m_PrevRow=this_row;
        m_Ledger->SetRow(0,&pldRow);        
    }
    
    m_Ledger->Close();  
    
    if(err == FC_ERR_NOERROR)
    {
        m_Block=block;
        m_Row=this_row;
        UpdateCounts();        
        m_ClearedAdminCount=m_AdminCount;
        m_ClearedMinerCount=m_MinerCount;
    }
    
    sprintf(msg,"Block rollback: %9d, Admin count: %d, Miner count: %d, Ledger Rows: %ld",m_Block,m_AdminCount,m_MinerCount,m_Row);
    LogString(msg);
    return err;   
}



