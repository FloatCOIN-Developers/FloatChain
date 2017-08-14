// Copyright (c) 2014-2017 Coin Sciences Ltd
// FloatChain code distributed under the GPLv3 license, see COPYING file.

#include "floatchain/floatchain.h"

#define FC_AST_ASSET_REF_TYPE_OFFSET        32
#define FC_AST_ASSET_REF_TYPE_SIZE           4
#define FC_AST_ASSET_SCRIPT_TYPE_OFFSET     44
#define FC_AST_ASSET_SCRIPT_TYPE_SIZE        4

void FC_EntityDBRow::Zero()
{
    memset(this,0,sizeof(FC_EntityDBRow));
}

void FC_EntityLedgerRow::Zero()
{
    memset(this,0,sizeof(FC_EntityLedgerRow));
}

void FC_EntityDetails::Zero()
{
    memset(this,0,sizeof(FC_EntityDetails));
    m_LedgerRow.Zero();    
}



/** Set initial database object values */

void FC_EntityDB::Zero()
{
    m_FileName[0]=0;
    m_DB=0;
    m_KeyOffset=0;
    m_KeySize=36;                                   
    m_ValueOffset=36;
    m_ValueSize=28;  
    m_TotalSize=m_KeySize+m_ValueSize;
}

/** Set database file name */

void FC_EntityDB::SetName(const char* name)
{
    FC_GetFullFileName(name,"entities",".db",FC_FOM_RELATIVE_TO_DATADIR | FC_FOM_CREATE_DIR,m_FileName);
}

/** Open database */

int FC_EntityDB::Open() 
{
    
    m_DB=new FC_Database;
    
    m_DB->SetOption("KeySize",0,m_KeySize);
    m_DB->SetOption("ValueSize",0,m_ValueSize);
        
    return m_DB->Open(m_FileName,FC_OPT_DB_DATABASE_CREATE_IF_MISSING | FC_OPT_DB_DATABASE_TRANSACTIONAL | FC_OPT_DB_DATABASE_LEVELDB);
}

/** Close database */

int FC_EntityDB::Close()
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

void FC_EntityLedger::Zero()
{
    m_FileName[0]=0;
    m_FileHan=0;
    m_KeyOffset=0;
    m_KeySize=36;                                   
    m_ValueOffset=36;
    m_ValueSize=60;  
    m_TotalSize=m_KeySize+m_ValueSize;
}

/** Set ledger file name */

void FC_EntityLedger::SetName(const char* name)
{
    FC_GetFullFileName(name,"entities",".dat",FC_FOM_RELATIVE_TO_DATADIR,m_FileName);
}

/** Open ledger file */

int FC_EntityLedger::Open()
{
    if(m_FileHan>0)
    {
        return m_FileHan;
    }
    
    m_FileHan=open(m_FileName,_O_BINARY | O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    return m_FileHan;            
}

void FC_EntityLedger::Flush()
{
    if(m_FileHan>0)
    {
        __US_FlushFile(m_FileHan);
    }    
}

/** Close ledger file */

int FC_EntityLedger::Close()
{
    if(m_FileHan>0)
    {
        close(m_FileHan);
    }    
    m_FileHan=0;
    return 0;    
}

int FC_EntityLedger::GetRow(int64_t pos, FC_EntityLedgerRow* row)
{
    int size;
    if(m_FileHan<=0)
    {
        return FC_ERR_INTERNAL_ERROR;
    }
    
    if(lseek64(m_FileHan,pos,SEEK_SET) != pos)
    {
        return FC_ERR_NOT_FOUND;
    }
    
    row->Zero();

    if(read(m_FileHan,(unsigned char*)row+m_KeyOffset,m_TotalSize) != m_TotalSize)
    {
        return FC_ERR_FILE_READ_ERROR;
    }
    
    size=row->m_ScriptSize;

    if((size>0) && (size<=FC_ENT_SCRIPT_ALLOC_SIZE))
    {
        size=FC_AllocSize(size,m_TotalSize,1);
        if(read(m_FileHan,row->m_Script,size) != size)
        {
            return FC_ERR_FILE_READ_ERROR;
        }        
    }
    
    
    return FC_ERR_NOERROR;
}

int64_t FC_EntityLedger::GetSize()
{
    int64_t pos;
    
    FC_EntityLedgerRow aldRow;
    if(m_FileHan<=0)
    {
        return 0;
    }

    GetRow(0,&aldRow);
    pos=aldRow.m_PrevPos;
    GetRow(pos,&aldRow);
    pos+=FC_AllocSize(m_TotalSize+aldRow.m_ScriptSize,m_TotalSize,1);
    
    return pos;
}

int FC_EntityLedger::SetRow(int64_t pos, FC_EntityLedgerRow* row)
{
    int size;
    
    if(m_FileHan<=0)
    {
        return -1;
    }
    
    if(lseek64(m_FileHan,pos,SEEK_SET) < 0)
    {
        return -1;
    }
       
    size=row->m_ScriptSize;
    
    if((size>=0) && (size<=FC_ENT_SCRIPT_ALLOC_SIZE))
    {
        size=FC_AllocSize(size,m_TotalSize,1);
        if(write(m_FileHan,row,m_TotalSize) != m_TotalSize)
        {
            return -1;
        }        
        if(size)
        {
            if(write(m_FileHan,row->m_Script,size) != size)
            {
                return -1;
            }        
        }
    }
    else
    {
        return -1;        
    }
    
    return m_TotalSize+size;
}

int FC_EntityLedger::SetZeroRow(FC_EntityLedgerRow* row)
{
    return SetRow(0,row);
}


int FC_AssetDB::Zero()
{
    m_Database = NULL;
    m_Ledger = NULL;
    m_MemPool = NULL;
    m_Name[0]=0x00; 
    m_Block=-1;    
    m_PrevPos=-1;
    m_Pos=0;
    m_DBRowCount=0;            
    
    return FC_ERR_NOERROR;
}

int FC_AssetDB::Initialize(const char *name,int mode)
{
    int err,value_len,take_it;    
    int32_t adbBlock,aldBlock;
    uint64_t adbLastPos,aldLastPos,this_pos;
    
    unsigned char *ptr;

    FC_EntityDBRow adbRow;
    FC_EntityLedgerRow aldRow;
        
    strcpy(m_Name,name);
    
    err=FC_ERR_NOERROR;
    
    m_Ledger=new FC_EntityLedger;
    m_Database=new FC_EntityDB;
     
    m_Ledger->SetName(name);
    m_Database->SetName(name);
    
    strcpy(m_Name,name);

    err=m_Database->Open();
    
    if(err)
    {
        return err;
    }
    
    adbBlock=-1;    
    adbLastPos=0;
    
    adbRow.Zero();
    
    ptr=(unsigned char*)m_Database->m_DB->Read((char*)&adbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,&value_len,0,&err);
    if(err)
    {
        return err;
    }

    if(ptr)                                                                     
    {
        meFCpy((char*)&adbRow+m_Database->m_ValueOffset,ptr,m_Database->m_ValueSize);
        adbBlock=adbRow.m_Block;
        adbLastPos=adbRow.m_LedgerPos;
    }
    else
    {
        adbRow.Zero();
        adbRow.m_Block=(uint32_t)adbBlock;
        adbRow.m_LedgerPos=adbLastPos;
        err=m_Database->m_DB->Write((char*)&adbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,(char*)&adbRow+m_Database->m_ValueOffset,m_Database->m_ValueSize,0);
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
    
    m_MemPool=new FC_Buffer;    
    err=m_MemPool->Initialize(m_Ledger->m_KeySize,sizeof(FC_EntityLedgerRow),FC_BUF_MODE_MAP);
    
    m_Block=adbBlock;
    m_PrevPos=adbLastPos;            
    m_Pos=adbLastPos+m_Ledger->m_TotalSize;            
    
    aldBlock=-1;
    aldLastPos=0;
    if(m_Ledger->Open() <= 0)
    {
        return FC_ERR_DBOPEN_ERROR;
    }
    if(m_Ledger->GetRow(0,&aldRow) == 0)
    {
        aldBlock=aldRow.m_Block;
        aldLastPos=aldRow.m_PrevPos;
        m_PrevPos=adbLastPos;                    
        if(m_Ledger->GetRow(m_PrevPos,&aldRow))
        {
            return FC_ERR_CORRUPTED;
        }
        
        m_Pos=m_PrevPos+FC_AllocSize(m_Ledger->m_TotalSize+aldRow.m_ScriptSize,m_Ledger->m_TotalSize,1);
    }
    else
    {
        aldRow.Zero();
        aldRow.m_Block=(uint32_t)aldBlock;
        aldRow.m_PrevPos=aldLastPos;
        m_Ledger->SetZeroRow(&aldRow);
    }        
    if(m_Pos != m_Ledger->GetSize())
    {
        m_Ledger->Close();
        return FC_ERR_CORRUPTED;        
    }

    m_Ledger->Close();
    
    if(adbBlock < aldBlock)
    {
        if(m_Ledger->Open() <= 0)
        {
            return FC_ERR_DBOPEN_ERROR;
        }
    
        this_pos=aldLastPos;
        take_it=1;

        while(take_it && (this_pos>0))
        {            
            m_Ledger->GetRow(this_pos,&aldRow);
        
            if(aldRow.m_Block <= adbBlock)
            {
                take_it=0;
            }
            if(take_it)
            {
                this_pos=aldRow.m_PrevPos;
            }
        }

        aldLastPos=this_pos;

        m_Ledger->GetRow(0,&aldRow);
        aldRow.m_Block=adbBlock;
        aldRow.m_PrevPos=m_PrevPos;
        m_Ledger->SetZeroRow(&aldRow);
        
        m_Ledger->Close();  

        aldBlock=adbBlock;
    }
    
    if(adbBlock != aldBlock)
    {
        return FC_ERR_CORRUPTED;
    }

    if(adbLastPos != aldLastPos)
    {
        return FC_ERR_CORRUPTED;
    }

    return FC_ERR_NOERROR;
}

void FC_AssetDB::RemoveFiles()
{
    FC_EntityDBRow adbRow;
    if(m_Database)
    {
        adbRow.Zero();
        m_Database->m_DB->Delete((char*)&adbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,0);
        m_Database->m_DB->Commit(0);
    }
    if(m_Ledger)
    {
        m_Ledger->Close();
        remove(m_Ledger->m_FileName);
    }
}

int FC_AssetDB::Destroy()
{
    if(m_Database)
    {
        m_Database->Close();        
        delete m_Database;
    }
    
    if(m_Ledger)
    {
        delete m_Ledger;
    }
    
    if(m_MemPool)
    {
        delete m_MemPool;
    }  
    
    Zero();
    
    return FC_ERR_NOERROR;
}

int FC_AssetDB::GetEntity(FC_EntityLedgerRow* row)
{    
    int err,value_len,mprow;
    int result;
    FC_EntityDBRow adbRow;
    
    unsigned char *ptr;

    adbRow.Zero();
    meFCpy(adbRow.m_Key,row->m_Key,FC_ENT_KEY_SIZE);
    adbRow.m_KeyType=row->m_KeyType;
    ptr=(unsigned char*)m_Database->m_DB->Read((char*)row+m_Database->m_KeyOffset,m_Database->m_KeySize,&value_len,0,&err);
    if(err)
    {
        return 0;
    }
    
    if(ptr)
    {         
        meFCpy((char*)&adbRow+m_Database->m_ValueOffset,ptr,m_Database->m_ValueSize);
        
        if(m_Ledger->Open() <= 0)
        {
            return 0;
        }

        result=1;        
        if(m_Ledger->GetRow(adbRow.m_LedgerPos,row))
        {
            result=0;
        }
        
        row->m_ChainPos=adbRow.m_ChainPos;
        m_Ledger->Close();
        
        return result;
    }
    
    mprow=m_MemPool->Seek((unsigned char*)row);
    if(mprow>=0)
    {
        if( (((FC_EntityLedgerRow*)(m_MemPool->GetRow(mprow)))->m_KeyType  & FC_ENT_KEYTYPE_MASK) != FC_ENT_KEYTYPE_TXID)
        {
            mprow--;
            if(mprow>=0)
            {
                if( (((FC_EntityLedgerRow*)(m_MemPool->GetRow(mprow)))->m_KeyType  & FC_ENT_KEYTYPE_MASK) != FC_ENT_KEYTYPE_TXID)
                {
                    mprow--;                    
                    if(mprow>=0)
                    {
                        if( (((FC_EntityLedgerRow*)(m_MemPool->GetRow(mprow)))->m_KeyType  & FC_ENT_KEYTYPE_MASK) != FC_ENT_KEYTYPE_TXID)
                        {
                            mprow--;                    
                            if(mprow>=0)
                            {
                                if( (((FC_EntityLedgerRow*)(m_MemPool->GetRow(mprow)))->m_KeyType  & FC_ENT_KEYTYPE_MASK) != FC_ENT_KEYTYPE_TXID)
                                {
                                    mprow=-1;
                                }
                            }
                        }
                    }
                }
            }
        }
        if(mprow<0)
        {
            return 0;
        }
        meFCpy(row,m_MemPool->GetRow(mprow),sizeof(FC_EntityLedgerRow));
        return 1;
    }


    return 0;
}

void FC_EntityDetails::Set(FC_EntityLedgerRow* row)
{    
    uint32_t block,i;
    int offset,script_size;
    uint32_t value_offset;
    size_t value_size;
    unsigned char dname_buf[6];
    
    Zero();
    
    meFCpy(&m_LedgerRow,row,sizeof(FC_EntityLedgerRow));
    
    block=row->m_Block;
    offset=row->m_Offset;    
    script_size=row->m_ScriptSize;
    
    m_Flags=0;
    
    if(offset)
    {
        FC_PutLE(m_Ref,&block,4);
        FC_PutLE(m_Ref+4,&offset,4);
        for(i=0;i<FC_ENT_REF_PREFIX_SIZE;i++)
        {
            m_Ref[8+i]=*(row->m_Key+FC_ENT_KEY_SIZE-1-i);
        }
        m_Flags |= FC_ENT_FLAG_OFFSET_IS_SET;
    }
    
    if(script_size)
    {
        value_offset=FC_FindSpecialParamInDetailsScript(m_LedgerRow.m_Script,m_LedgerRow.m_ScriptSize,FC_ENT_SPRM_NAME,&value_size);
        if(value_offset == m_LedgerRow.m_ScriptSize)
        {
            strcpy((char*)dname_buf+1,"name");
            dname_buf[0]=0xff;
            value_offset=FC_FindNamedParamInDetailsScript(m_LedgerRow.m_Script,m_LedgerRow.m_ScriptSize,(char*)dname_buf,&value_size);
        }
        if(FC_gState->m_Features->Streams())
        {
            if(value_offset < m_LedgerRow.m_ScriptSize)
            {            
                if(value_size == 2)
                {
                    if((char)m_LedgerRow.m_Script[value_offset] == '*')
                    {
                        value_offset=m_LedgerRow.m_ScriptSize;
                        value_size=0;
                    }
                }
            }
        }
        if(value_offset < m_LedgerRow.m_ScriptSize)
        {            
            meFCpy(m_Name,m_LedgerRow.m_Script+value_offset,value_size);
            FC_StringLowerCase(m_Name,value_size);
            m_Flags |= FC_ENT_FLAG_NAME_IS_SET;
        }
    }
    
    FC_ZeroABRaw(m_FullRef);
    if(FC_gState->m_Features->ShortTxIDInTx())
    {
        meFCpy(m_FullRef+FC_AST_SHORT_TXID_OFFSET,m_LedgerRow.m_Key+FC_AST_SHORT_TXID_OFFSET,FC_AST_SHORT_TXID_SIZE);
        FC_SetABRefType(m_FullRef,FC_AST_ASSET_REF_TYPE_SHORT_TXID);
    }
    else
    {
        meFCpy(m_FullRef,m_Ref,FC_AST_ASSET_REF_SIZE);
        FC_SetABRefType(m_FullRef,FC_AST_ASSET_REF_TYPE_REF);
    }
}

int FC_AssetDB::InsertEntity(const void* txid, int offset, int entity_type, const void *script,size_t script_size, const void* special_script, size_t special_script_size,int update_mempool)
{
    FC_EntityLedgerRow aldRow;
    FC_EntityDetails details;
    
    int pass;
    uint32_t value_offset;
    size_t value_size;
    char stream_name[FC_ENT_MAX_NAME_SIZE+1];
    uint32_t upgrade_start_block;
    
    aldRow.Zero();
    meFCpy(aldRow.m_Key,txid,FC_ENT_KEY_SIZE);
    aldRow.m_KeyType=FC_ENT_KEYTYPE_TXID;
    aldRow.m_Block=m_Block+1;
    aldRow.m_Offset=offset;
    if(offset<0)
    {
        aldRow.m_Offset=-(m_MemPool->GetCount()+1);
    }
    aldRow.m_Quantity=0;
    aldRow.m_EntityType=entity_type;
    aldRow.m_FirstPos=-1;
    aldRow.m_LastPos=0;
    aldRow.m_ChainPos=-1;
    aldRow.m_PrevPos=-1;
    
    FC_Script *lpDetails;
    lpDetails=new FC_Script;
    lpDetails->AddElement();

    if(special_script_size)
    {
        lpDetails->SetData((const unsigned char*)special_script,special_script_size);
    }
        
    if(script)
    {            
        value_offset=FC_FindSpecialParamInDetailsScript((unsigned char*)script,script_size,FC_ENT_SPRM_NAME,&value_size);
        if(value_offset != script_size)
        {
            if(value_size)
            {
                if(*((unsigned char*)script+value_offset+value_size-1))
                {
                    meFCpy(stream_name,(unsigned char*)script+value_offset,value_size);
                    stream_name[value_size]=0x00;
                    lpDetails->SetSpecialParamValue(FC_ENT_SPRM_NAME,(unsigned char*)stream_name,strlen(stream_name)+1);            
                }
            }
        }
    }

    upgrade_start_block=0;
    if(FC_gState->m_Features->Upgrades())
    {
        if(entity_type == FC_ENT_TYPE_UPGRADE)
        {
            if(script)
            {
                value_offset=FC_FindSpecialParamInDetailsScript((unsigned char*)script,script_size,FC_ENT_SPRM_UPGRADE_PROTOCOL_VERSION,&value_size);
                if(value_offset == script_size)
                {
                    return FC_ERR_ERROR_IN_SCRIPT;                                            
                }
                if( (value_size <=0) || (value_size > 4) )
                {
                    return FC_ERR_ERROR_IN_SCRIPT;                        
                }
                if((int)FC_GetLE((unsigned char*)script+value_offset,value_size) < 0)
                {
                    return FC_ERR_ERROR_IN_SCRIPT;                        
                }
                value_offset=FC_FindSpecialParamInDetailsScript((unsigned char*)script,script_size,FC_ENT_SPRM_UPGRADE_START_BLOCK,&value_size);
                if(value_offset != script_size)
                {
                    if( (value_size <=0) || (value_size > 4) )
                    {
                        return FC_ERR_ERROR_IN_SCRIPT;                        
                    }
                    upgrade_start_block=(uint32_t)FC_GetLE((unsigned char*)script+value_offset,value_size);
                }
            }
        }
    }

    
    if(lpDetails->m_Size)
    {
        meFCpy(aldRow.m_Script,lpDetails->GetData(0,NULL),lpDetails->m_Size);        
    }    

    aldRow.m_ScriptSize=script_size+lpDetails->m_Size;
    if(script_size)
    {
        meFCpy(aldRow.m_Script+lpDetails->m_Size,script,script_size);        
    }
    
    delete lpDetails;
    
    details.Set(&aldRow);
        
    for(pass=0;pass<1+update_mempool;pass++)
    {
        memset(aldRow.m_Key,0,FC_ENT_KEY_SIZE);
        meFCpy(aldRow.m_Key,details.m_LedgerRow.m_Key,FC_ENT_KEY_SIZE);
        aldRow.m_KeyType=FC_ENT_KEYTYPE_TXID;

        if(pass)
        {
            m_MemPool->Add((unsigned char*)&aldRow,(unsigned char*)&aldRow+m_Ledger->m_ValueOffset);
        }
        else
        {
            if(GetEntity(&aldRow))
            {
                return FC_ERR_FOUND;
            }            
        }

        if(details.m_Flags & FC_ENT_FLAG_OFFSET_IS_SET)
        {
            memset(aldRow.m_Key,0,FC_ENT_KEY_SIZE);
            meFCpy(aldRow.m_Key,details.m_Ref,FC_ENT_REF_SIZE);
            aldRow.m_KeyType=FC_ENT_KEYTYPE_REF;
            
            if(pass)
            {
                m_MemPool->Add((unsigned char*)&aldRow,(unsigned char*)&aldRow+m_Ledger->m_ValueOffset);
            }
            else
            {
                if(GetEntity(&aldRow))
                {
                    return FC_ERR_FOUND;
                }            
            }
        }

        if(details.m_Flags & FC_ENT_FLAG_NAME_IS_SET)
        {
            memset(aldRow.m_Key,0,FC_ENT_KEY_SIZE);
            meFCpy(aldRow.m_Key,details.m_Name,FC_ENT_MAX_NAME_SIZE);
            aldRow.m_KeyType=FC_ENT_KEYTYPE_NAME;
                        
            if(pass)
            {
                m_MemPool->Add((unsigned char*)&aldRow,(unsigned char*)&aldRow+m_Ledger->m_ValueOffset);
            }
            else
            {
                if(GetEntity(&aldRow))
                {
                    return FC_ERR_FOUND;
                }            
            }
        }

        memset(aldRow.m_Key,0,FC_ENT_KEY_SIZE);
        meFCpy(aldRow.m_Key,details.m_LedgerRow.m_Key+FC_AST_SHORT_TXID_OFFSET,FC_AST_SHORT_TXID_SIZE);
        aldRow.m_KeyType=FC_ENT_KEYTYPE_SHORT_TXID;

        if(pass)
        {
            m_MemPool->Add((unsigned char*)&aldRow,(unsigned char*)&aldRow+m_Ledger->m_ValueOffset);
        }
        else
        {
            if(GetEntity(&aldRow))
            {
                return FC_ERR_FOUND;
            }            
        }
    }    

    if(FC_gState->m_Features->Upgrades())
    {
        if(entity_type == FC_ENT_TYPE_UPGRADE)
        {
            return FC_gState->m_Permissions->SetApproval((unsigned char*)txid+FC_AST_SHORT_TXID_OFFSET,1,NULL,upgrade_start_block,FC_TimeNowAsUInt(),FC_PFL_ENTITY_GENESIS,update_mempool,offset);
        }
    }
    
    return FC_ERR_NOERROR;    
}


int FC_AssetDB::InsertAsset(const void* txid, int offset, uint64_t quantity, const char *name, int multiple, const void* script, size_t script_size, const void* special_script, size_t special_script_size,int update_mempool)
{
    FC_EntityLedgerRow aldRow;
    FC_EntityDetails details;
    
    int pass;
    uint32_t value_offset;
    size_t value_size;
    int add_param;
    
    aldRow.Zero();
    meFCpy(aldRow.m_Key,txid,FC_ENT_KEY_SIZE);
    aldRow.m_KeyType=FC_ENT_KEYTYPE_TXID;
    aldRow.m_Block=m_Block+1;
    aldRow.m_Offset=offset;
    if(offset<0)
    {
        aldRow.m_Offset=-(m_MemPool->GetCount()+1);
    }
    aldRow.m_Quantity=quantity;
    aldRow.m_EntityType=FC_ENT_TYPE_ASSET;
    aldRow.m_FirstPos=-(m_MemPool->GetCount()+1);//-1;                          // Unconfirmed issue, from 10007 we can create followons for them, so we should differentiate
    aldRow.m_LastPos=0;
    aldRow.m_ChainPos=-1;
    aldRow.m_PrevPos=-1;
    
    FC_Script *lpDetails;
    lpDetails=new FC_Script;
    lpDetails->AddElement();

    if(special_script_size)
    {
        lpDetails->SetData((const unsigned char*)special_script,special_script_size);
    }

    add_param=true;
    if(FC_gState->m_Features->OpDropDetailsScripts())
    {
        if(script)
        {
            if(FC_FindSpecialParamInDetailsScript((unsigned char*)script,script_size,FC_ENT_SPRM_ASSET_MULTIPLE,&value_size) != script_size)
            {
                add_param=false;                                
            }
        }
    }

    if(add_param)
    {
        lpDetails->SetSpecialParamValue(FC_ENT_SPRM_ASSET_MULTIPLE,(unsigned char*)&multiple,sizeof(multiple));
    }
    
    add_param=true;
    if(script)
    {            
        value_offset=FC_FindSpecialParamInDetailsScript((unsigned char*)script,script_size,FC_ENT_SPRM_NAME,&value_size);
        if(value_offset != script_size)
        {
            if(value_size)
            {
                if(*((unsigned char*)script+value_offset+value_size-1) == 0)
                {
                    add_param=false;                                                        
                }
            }
        }
    }

    if(add_param)
    {
        if(name && (strlen(name) > 0))
        {
            lpDetails->SetSpecialParamValue(FC_ENT_SPRM_NAME,(unsigned char*)name,strlen(name)+1);            
        }
    }
    
    if(lpDetails->m_Size)
    {
        meFCpy(aldRow.m_Script,lpDetails->GetData(0,NULL),lpDetails->m_Size);        
    }    
    
    aldRow.m_ScriptSize=script_size+lpDetails->m_Size;
    if(script)
    {
        meFCpy(aldRow.m_Script+lpDetails->m_Size,script,script_size);        
    }
    
    delete lpDetails;
    
    details.Set(&aldRow);
        
    for(pass=0;pass<1+update_mempool;pass++)
    {
        memset(aldRow.m_Key,0,FC_ENT_KEY_SIZE);
        meFCpy(aldRow.m_Key,details.m_LedgerRow.m_Key,FC_ENT_KEY_SIZE);
        aldRow.m_KeyType=FC_ENT_KEYTYPE_TXID;

        if(pass)
        {
            m_MemPool->Add((unsigned char*)&aldRow,(unsigned char*)&aldRow+m_Ledger->m_ValueOffset);
        }
        else
        {
            if(GetEntity(&aldRow))
            {
                return FC_ERR_FOUND;
            }            
        }


        if(details.m_Flags & FC_ENT_FLAG_OFFSET_IS_SET)
        {
            memset(aldRow.m_Key,0,FC_ENT_KEY_SIZE);
            meFCpy(aldRow.m_Key,details.m_Ref,FC_ENT_REF_SIZE);
            aldRow.m_KeyType=FC_ENT_KEYTYPE_REF;
            
            if(pass)
            {
                m_MemPool->Add((unsigned char*)&aldRow,(unsigned char*)&aldRow+m_Ledger->m_ValueOffset);
            }
            else
            {
                if(GetEntity(&aldRow))
                {
                    return FC_ERR_FOUND;
                }            
            }
        }

        if(details.m_Flags & FC_ENT_FLAG_NAME_IS_SET)
        {
            memset(aldRow.m_Key,0,FC_ENT_KEY_SIZE);
            meFCpy(aldRow.m_Key,details.m_Name,FC_ENT_MAX_NAME_SIZE);
            aldRow.m_KeyType=FC_ENT_KEYTYPE_NAME;
                        
            if(pass)
            {
                m_MemPool->Add((unsigned char*)&aldRow,(unsigned char*)&aldRow+m_Ledger->m_ValueOffset);
            }
            else
            {
                if(GetEntity(&aldRow))
                {
                    return FC_ERR_FOUND;
                }            
            }
        }
        
        memset(aldRow.m_Key,0,FC_ENT_KEY_SIZE);
        meFCpy(aldRow.m_Key,details.m_LedgerRow.m_Key+FC_AST_SHORT_TXID_OFFSET,FC_AST_SHORT_TXID_SIZE);
        aldRow.m_KeyType=FC_ENT_KEYTYPE_SHORT_TXID;

        if(pass)
        {
            m_MemPool->Add((unsigned char*)&aldRow,(unsigned char*)&aldRow+m_Ledger->m_ValueOffset);
        }
        else
        {
            if(GetEntity(&aldRow))
            {
                return FC_ERR_FOUND;
            }            
        }
        
    }    

    return FC_ERR_NOERROR;
}

int FC_AssetDB::InsertAssetFollowOn(const void* txid, int offset, uint64_t quantity, const void* script, size_t script_size, const void* special_script, size_t special_script_size, const void* original_txid, int update_mempool)
{
    FC_EntityLedgerRow aldRow;
    
    int pass,i;
    int64_t size,first_pos,last_pos,tot_pos;
    uint64_t value_offset;
    size_t value_size;
    int64_t total;
    uint32_t entity_type;

    aldRow.Zero();
    meFCpy(aldRow.m_Key,original_txid,FC_ENT_KEY_SIZE);
    aldRow.m_KeyType=FC_ENT_KEYTYPE_TXID;
    if(!GetEntity(&aldRow))
    {        
        return FC_ERR_NOT_FOUND;
    }    
    
    if(FC_gState->m_Features->ShortTxIDInTx() == 0)
    {
        if(aldRow.m_PrevPos < 0)                                                // Unconfirmed genesis for protocol < 10007
        {
            return FC_ERR_NOT_FOUND;        
        }        
    }
    
    value_offset=FC_FindSpecialParamInDetailsScript(aldRow.m_Script,aldRow.m_ScriptSize,FC_ENT_SPRM_FOLLOW_ONS,&value_size);
    if(value_offset == aldRow.m_ScriptSize)
    {
        return FC_ERR_NOT_ALLOWED;
    }
    if( (value_size==0) || (value_size > 4))
    {
        return FC_ERR_INVALID_PARAMETER_VALUE;
    }
    if(FC_GetLE(aldRow.m_Script+value_offset,value_size) == 0)
    {
        return FC_ERR_NOT_ALLOWED;        
    }
    
    total=GetTotalQuantity(&aldRow);
    if((int64_t)(total+quantity)<0)
    {
        return FC_ERR_INVALID_PARAMETER_VALUE;        
    }        
    
    
    first_pos=aldRow.m_FirstPos;
    last_pos=aldRow.m_ChainPos;
    entity_type=aldRow.m_EntityType;
    
    tot_pos=m_Pos;
    
    for(i=0;i<m_MemPool->GetCount();i++)
    {
        if( (((FC_EntityLedgerRow*)(m_MemPool->GetRow(i)))->m_KeyType  & FC_ENT_KEYTYPE_MASK) == FC_ENT_KEYTYPE_TXID)
        {
            size=m_Ledger->m_TotalSize+FC_AllocSize(((FC_EntityLedgerRow*)(m_MemPool->GetRow(i)))->m_ScriptSize,m_Ledger->m_TotalSize,1);
            if( ((FC_EntityLedgerRow*)(m_MemPool->GetRow(i)))->m_KeyType == (FC_ENT_KEYTYPE_FOLLOW_ON | FC_ENT_KEYTYPE_TXID) )
            {
                if(((FC_EntityLedgerRow*)(m_MemPool->GetRow(i)))->m_FirstPos == first_pos)
                {
                    last_pos=tot_pos;
                }
            }
            tot_pos+=size;
        }        
    }
    
    
    aldRow.Zero();
    meFCpy(aldRow.m_Key,txid,FC_ENT_KEY_SIZE);
    aldRow.m_KeyType=FC_ENT_KEYTYPE_FOLLOW_ON | FC_ENT_KEYTYPE_TXID;
    aldRow.m_Block=m_Block+1;
    aldRow.m_Offset=offset;
    if(offset<0)
    {
        aldRow.m_Offset=-(m_MemPool->GetCount()+1);
    }
    aldRow.m_Quantity=quantity;
    aldRow.m_EntityType=entity_type;
    aldRow.m_FirstPos=first_pos;
    aldRow.m_LastPos=last_pos;
    aldRow.m_ChainPos=-1;
    aldRow.m_PrevPos=-1;

    FC_Script *lpDetails;
    lpDetails=new FC_Script;
    lpDetails->AddElement();

    if(special_script_size)
    {
        lpDetails->SetData((const unsigned char*)special_script,special_script_size);
    }
    
    if(lpDetails->m_Size)
    {
        meFCpy(aldRow.m_Script,lpDetails->GetData(0,NULL),lpDetails->m_Size);        
    }    
    
    aldRow.m_ScriptSize=script_size+lpDetails->m_Size;
    if(script)
    {
        meFCpy(aldRow.m_Script+lpDetails->m_Size,script,script_size);        
    }
    
    delete lpDetails;
    
    for(pass=0;pass<1+update_mempool;pass++)
    {
        if(pass)
        {
            m_MemPool->Add((unsigned char*)&aldRow,(unsigned char*)&aldRow+m_Ledger->m_ValueOffset);
        }
        else
        {
            if(GetEntity(&aldRow))
            {
                return FC_ERR_FOUND;
            }            
        }
    }    

    return FC_ERR_NOERROR;
    
}

int FC_AssetDB::Commit()
{ 
    int i,size,err,value_len;
    
    FC_EntityDBRow adbRow;
    FC_EntityLedgerRow aldRow;
    FC_EntityLedgerRow aldGenesisRow;
    FC_EntityDetails details;
    unsigned char *ptr;
    
    err=FC_ERR_NOERROR;    
    
    if(m_Ledger->Open() <= 0)
    {
        return FC_ERR_DBOPEN_ERROR;
    }

    if(m_MemPool->GetCount())
    {
        if(err == FC_ERR_NOERROR)
        {
            size=0;
            for(i=0;i<m_MemPool->GetCount();i++)
            {
                if(err == FC_ERR_NOERROR)
                {
                    meFCpy(&aldRow,m_MemPool->GetRow(i),sizeof(FC_EntityLedgerRow));
                    aldGenesisRow.Zero();
                    if( (aldRow.m_KeyType  & FC_ENT_KEYTYPE_MASK) == FC_ENT_KEYTYPE_TXID)
                    {
                        m_Pos+=size;
                        aldRow.m_PrevPos=m_PrevPos;
                        if(aldRow.m_KeyType == (FC_ENT_KEYTYPE_FOLLOW_ON | FC_ENT_KEYTYPE_TXID))
                        {
                            if(aldRow.m_FirstPos < 0)
                            {
                                meFCpy(&aldGenesisRow,m_MemPool->GetRow(-aldRow.m_FirstPos-1),sizeof(FC_EntityLedgerRow));     
                                aldRow.m_LastPos=aldGenesisRow.m_ChainPos;
                                aldGenesisRow.m_ChainPos=m_Pos;
                                meFCpy(m_MemPool->GetRow(-aldRow.m_FirstPos-1),&aldGenesisRow,sizeof(FC_EntityLedgerRow));     
                                aldRow.m_FirstPos=aldGenesisRow.m_FirstPos;
                            }
                            else
                            {
                                err=m_Ledger->GetRow(aldRow.m_FirstPos,&aldGenesisRow);                                
                            }
                        }
                        else
                        {
                            if(aldRow.m_FirstPos < 0)
                            {
                                aldRow.m_FirstPos=m_Pos;
                            }                            
                        }
                        if(aldRow.m_ChainPos < 0)
                        {
                            aldRow.m_ChainPos=m_Pos;
                        }
                        meFCpy(m_MemPool->GetRow(i),&aldRow,sizeof(FC_EntityLedgerRow));
                        m_PrevPos=m_Pos;
                        if(aldRow.m_Offset < 0)
                        {
                            err=FC_ERR_INTERNAL_ERROR;
                        }
                        else
                        {
                            size=m_Ledger->SetRow(m_PrevPos,&aldRow);
                            if(size<0)
                            {
                                err=FC_ERR_INTERNAL_ERROR;
                            }
                        }                    
                    }
                }
                
                if(err == FC_ERR_NOERROR)
                {
                    adbRow.Zero();
                    meFCpy(adbRow.m_Key,aldRow.m_Key,FC_ENT_KEY_SIZE);
                    adbRow.m_KeyType=aldRow.m_KeyType;
                    adbRow.m_EntityType=aldRow.m_EntityType;
                    adbRow.m_Block=aldRow.m_Block;
                    adbRow.m_LedgerPos=m_Pos;
                    adbRow.m_ChainPos=m_Pos;

                    err=m_Database->m_DB->Write((char*)&adbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,
                                                (char*)&adbRow+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
                    
                    if(err == FC_ERR_NOERROR)
                    {
                        if(aldGenesisRow.m_KeyType)
                        {
                            details.Set(&aldGenesisRow);
                            adbRow.Zero();
                            meFCpy(adbRow.m_Key,aldGenesisRow.m_Key,FC_ENT_KEY_SIZE);
                            adbRow.m_KeyType=aldGenesisRow.m_KeyType;
                            adbRow.m_EntityType=aldGenesisRow.m_EntityType;
                            adbRow.m_Block=aldGenesisRow.m_Block;
                            adbRow.m_LedgerPos=aldGenesisRow.m_FirstPos;
                            adbRow.m_ChainPos=m_Pos;

                            err=m_Database->m_DB->Write((char*)&adbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,
                                                        (char*)&adbRow+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
                            
                            memset(adbRow.m_Key,0,FC_ENT_KEY_SIZE);
                            meFCpy(adbRow.m_Key,details.m_LedgerRow.m_Key+FC_AST_SHORT_TXID_OFFSET,FC_AST_SHORT_TXID_SIZE);
                            adbRow.m_KeyType=FC_ENT_KEYTYPE_SHORT_TXID;
                            err=m_Database->m_DB->Write((char*)&adbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,
                                            (char*)&adbRow+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
                            
                            if(details.m_Flags & FC_ENT_FLAG_OFFSET_IS_SET)
                            {
                                memset(adbRow.m_Key,0,FC_ENT_KEY_SIZE);
                                meFCpy(adbRow.m_Key,details.m_Ref,FC_ENT_REF_SIZE);
                                adbRow.m_KeyType=FC_ENT_KEYTYPE_REF;                                    
                                err=m_Database->m_DB->Write((char*)&adbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,
                                                (char*)&adbRow+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
                            }
                            if(details.m_Flags & FC_ENT_FLAG_NAME_IS_SET)
                            {
                                memset(adbRow.m_Key,0,FC_ENT_KEY_SIZE);
                                meFCpy(adbRow.m_Key,details.m_Name,FC_ENT_MAX_NAME_SIZE);
                                adbRow.m_KeyType=FC_ENT_KEYTYPE_NAME;                                    
                                err=m_Database->m_DB->Write((char*)&adbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,
                                                (char*)&adbRow+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
                            }
                        }
                    }
                }                       
            }
            m_Pos+=size;
        }        
    }
    
    if(err == FC_ERR_NOERROR)
    {
        m_Ledger->GetRow(0,&aldRow);
        aldRow.m_Block=m_Block+1;
        aldRow.m_PrevPos=m_PrevPos;
        m_Ledger->SetZeroRow(&aldRow);
    }
    
    m_Ledger->Flush();
    m_Ledger->Close();    
    
    if(err == FC_ERR_NOERROR)
    {
        adbRow.Zero();
        
        ptr=(unsigned char*)m_Database->m_DB->Read((char*)&adbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,&value_len,0,&err);
        if(ptr)
        {
            meFCpy((char*)&adbRow+m_Database->m_ValueOffset,ptr,m_Database->m_ValueSize);
        }

        
        adbRow.m_Block=m_Block+1;
        adbRow.m_LedgerPos=m_PrevPos;
        err=m_Database->m_DB->Write((char*)&adbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,
                        (char*)&adbRow+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
    }        
    
    if(err == FC_ERR_NOERROR)
    {
        err=m_Database->m_DB->Commit(FC_OPT_DB_DATABASE_TRANSACTIONAL);
    }    
    if(err)
    {
        RollBack(m_Block);        
    }
    else
    {
        m_MemPool->Clear();
        m_Block++;
    }
    
    return err;
}

int FC_AssetDB::RollBack()
{
    return RollBack(m_Block-1);
}

int FC_AssetDB::RollBack(int block)
{
    int err;
    int take_it,value_len;
    int64_t this_pos,prev_pos,new_chain_pos;
    FC_EntityDBRow adbRow;
    FC_EntityLedgerRow aldRow;
    FC_EntityDetails details;
    unsigned char *ptr;
    
    err=FC_ERR_NOERROR;
    

    ClearMemPool();
    
    if(m_Ledger->Open() <= 0)
    {
        return FC_ERR_DBOPEN_ERROR;
    }
    
    this_pos=m_PrevPos;
    take_it=1;
    
    while(take_it && (this_pos>0))
    {
        m_Ledger->GetRow(this_pos,&aldRow);
        prev_pos=aldRow.m_PrevPos;
        details.Set(&aldRow);
        
        if(details.m_LedgerRow.m_Block > block)
        {
            adbRow.Zero();
            meFCpy(adbRow.m_Key,aldRow.m_Key,FC_ENT_KEY_SIZE);
            adbRow.m_KeyType=aldRow.m_KeyType;
            err=m_Database->m_DB->Delete((char*)&adbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,FC_OPT_DB_DATABASE_TRANSACTIONAL);

            adbRow.Zero();
            memset(adbRow.m_Key,0,FC_ENT_KEY_SIZE);
            meFCpy(adbRow.m_Key,aldRow.m_Key+FC_AST_SHORT_TXID_OFFSET,FC_AST_SHORT_TXID_SIZE);
            adbRow.m_KeyType=FC_ENT_KEYTYPE_SHORT_TXID;
            err=m_Database->m_DB->Delete((char*)&adbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,FC_OPT_DB_DATABASE_TRANSACTIONAL);                
            
            if((adbRow.m_KeyType & FC_ENT_KEYTYPE_FOLLOW_ON) == 0)
            {
                if( (details.m_Flags & FC_ENT_FLAG_OFFSET_IS_SET))
                {
                    memset(adbRow.m_Key,0,FC_ENT_KEY_SIZE);
                    meFCpy(adbRow.m_Key,details.m_Ref,FC_ENT_REF_SIZE);
                    adbRow.m_KeyType=FC_ENT_KEYTYPE_REF;                                    
                    err=m_Database->m_DB->Delete((char*)&adbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
                }

                if(details.m_Flags & FC_ENT_FLAG_NAME_IS_SET)
                {
                    memset(adbRow.m_Key,0,FC_ENT_KEY_SIZE);
                    meFCpy(adbRow.m_Key,details.m_Name,FC_ENT_MAX_NAME_SIZE);
                    adbRow.m_KeyType=FC_ENT_KEYTYPE_NAME;                                    
                    err=m_Database->m_DB->Delete((char*)&adbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
                }            
            }
            
            if(err == FC_ERR_NOERROR)
            {
                if(aldRow.m_KeyType == (FC_ENT_KEYTYPE_FOLLOW_ON | FC_ENT_KEYTYPE_TXID))
                {
                    new_chain_pos=aldRow.m_LastPos;
                    err=m_Ledger->GetRow(aldRow.m_FirstPos,&aldRow);                        
                    details.Set(&aldRow);

                    adbRow.Zero();

                    memset(adbRow.m_Key,0,FC_ENT_KEY_SIZE);
                    meFCpy(adbRow.m_Key,details.m_LedgerRow.m_Key,FC_ENT_KEY_SIZE);
                    adbRow.m_KeyType=FC_ENT_KEYTYPE_TXID;

                    ptr=(unsigned char*)m_Database->m_DB->Read((char*)&adbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,&value_len,0,&err);

                    if(ptr)
                    {         
                        meFCpy((char*)&adbRow+m_Database->m_ValueOffset,ptr,m_Database->m_ValueSize);
                        adbRow.m_ChainPos=new_chain_pos;
                        err=m_Database->m_DB->Write((char*)&adbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,
                                        (char*)&adbRow+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
                        
                        memset(adbRow.m_Key,0,FC_ENT_KEY_SIZE);
                        meFCpy(adbRow.m_Key,details.m_LedgerRow.m_Key+FC_AST_SHORT_TXID_OFFSET,FC_AST_SHORT_TXID_SIZE);
                        adbRow.m_KeyType=FC_ENT_KEYTYPE_SHORT_TXID;
                        err=m_Database->m_DB->Write((char*)&adbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,
                                        (char*)&adbRow+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
                        if(details.m_Flags & FC_ENT_FLAG_OFFSET_IS_SET)
                        {
                            memset(adbRow.m_Key,0,FC_ENT_KEY_SIZE);
                            meFCpy(adbRow.m_Key,details.m_Ref,FC_ENT_REF_SIZE);
                            adbRow.m_KeyType=FC_ENT_KEYTYPE_REF;                                    
                            err=m_Database->m_DB->Write((char*)&adbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,
                                            (char*)&adbRow+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
                        }
                        if(details.m_Flags & FC_ENT_FLAG_NAME_IS_SET)
                        {
                            memset(adbRow.m_Key,0,FC_ENT_KEY_SIZE);
                            meFCpy(adbRow.m_Key,details.m_Name,FC_ENT_MAX_NAME_SIZE);
                            adbRow.m_KeyType=FC_ENT_KEYTYPE_NAME;                                    
                            err=m_Database->m_DB->Write((char*)&adbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,
                                            (char*)&adbRow+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);
                        }
                    }
                    else
                    {
                        err=FC_ERR_INTERNAL_ERROR;
                    }
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
            this_pos=prev_pos;
        }
    }

    m_PrevPos=this_pos;
    
    if(err == FC_ERR_NOERROR)
    {
        adbRow.Zero();
        
        ptr=(unsigned char*)m_Database->m_DB->Read((char*)&adbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,&value_len,0,&err);
        if(ptr)
        {
            meFCpy((char*)&adbRow+m_Database->m_ValueOffset,ptr,m_Database->m_ValueSize);
        }
            
        adbRow.m_Block=block;
        adbRow.m_LedgerPos=m_PrevPos;
        err=m_Database->m_DB->Write((char*)&adbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,
                        (char*)&adbRow+m_Database->m_ValueOffset,m_Database->m_ValueSize,FC_OPT_DB_DATABASE_TRANSACTIONAL);        
    }        
    
    if(err == FC_ERR_NOERROR)
    {
        err=m_Database->m_DB->Commit(FC_OPT_DB_DATABASE_TRANSACTIONAL);
    }    
    
    if(err == FC_ERR_NOERROR)
    {
        m_Ledger->GetRow(0,&aldRow);
        aldRow.m_Block=block;
        aldRow.m_PrevPos=m_PrevPos;
        m_Ledger->SetZeroRow(&aldRow);
    }
    
    if(err == FC_ERR_NOERROR)
    {
        m_Block=block;
        m_Ledger->GetRow(m_PrevPos,&aldRow);
        m_Pos=m_PrevPos+FC_AllocSize(m_Ledger->m_TotalSize+aldRow.m_ScriptSize,m_Ledger->m_TotalSize,1);            
    }
    
    m_Ledger->Close();
    return err;   
}

int FC_AssetDB::ClearMemPool()
{
    FC_EntityLedgerRow aldRow;
    
    if(m_MemPool)
    {
        if(m_MemPool->GetCount())
        {
            m_MemPool->Clear();
            if(m_Ledger->Open() <= 0)
            {
                return FC_ERR_DBOPEN_ERROR;
            }
        
            if(m_Ledger->GetRow(0,&aldRow))
            {
                return FC_ERR_CORRUPTED;        
            }
    
            m_PrevPos=aldRow.m_PrevPos;
            if(m_Ledger->GetRow(m_PrevPos,&aldRow))
            {
                return FC_ERR_CORRUPTED;        
            }
            
            m_Pos=m_PrevPos+FC_AllocSize(m_Ledger->m_TotalSize+aldRow.m_ScriptSize,m_Ledger->m_TotalSize,1);            
            
            m_Ledger->Close();
        }
    }
    
    return FC_ERR_NOERROR;
}


int FC_AssetDB::FindEntityByTxID(FC_EntityDetails *entity,const unsigned char* txid)
{
    FC_EntityLedgerRow aldRow;

    entity->Zero();
    aldRow.Zero();
    
    meFCpy(aldRow.m_Key,txid,FC_ENT_KEY_SIZE);
    aldRow.m_KeyType=FC_ENT_KEYTYPE_TXID;

    if(GetEntity(&aldRow))            
    {
        entity->Set(&aldRow);
        return 1;
    }            

    aldRow.m_KeyType=FC_ENT_KEYTYPE_TXID | FC_ENT_KEYTYPE_FOLLOW_ON;

    if(GetEntity(&aldRow))            
    {
        entity->Set(&aldRow);
        return 1;
    }            
    
    return 0;
}

int FC_AssetDB::FindEntityByShortTxID (FC_EntityDetails *entity, const unsigned char* short_txid)
{
    FC_EntityLedgerRow aldRow;

    entity->Zero();
    aldRow.Zero();
    
    meFCpy(aldRow.m_Key,short_txid,FC_AST_SHORT_TXID_SIZE);
    aldRow.m_KeyType=FC_ENT_KEYTYPE_SHORT_TXID;

    if(GetEntity(&aldRow))            
    {
        entity->Set(&aldRow);
        return 1;
    }            

    return 0;        
}


int FC_AssetDB::FindEntityByRef (FC_EntityDetails *entity,const unsigned char* asset_ref)
{
    FC_EntityLedgerRow aldRow;

    entity->Zero();
    aldRow.Zero();
    
    meFCpy(aldRow.m_Key,asset_ref,FC_ENT_REF_SIZE);
    aldRow.m_KeyType=FC_ENT_KEYTYPE_REF;

    if(GetEntity(&aldRow))            
    {
        entity->Set(&aldRow);
        return 1;
    }            

    return 0;    
}

int FC_AssetDB::FindEntityByFullRef (FC_EntityDetails *entity, unsigned char* full_ref)
{
    entity->Zero();
    switch(FC_GetABRefType(full_ref))
    {
        case FC_AST_ASSET_REF_TYPE_REF:
            return FindEntityByRef(entity,full_ref);
        case FC_AST_ASSET_REF_TYPE_SHORT_TXID:
            return FindEntityByShortTxID(entity,full_ref+FC_AST_SHORT_TXID_OFFSET);
        case FC_AST_ASSET_REF_TYPE_TXID:
            return FindEntityByTxID(entity,full_ref);            
    }
    return false;
}


int FC_AssetDB::FindEntityByName(FC_EntityDetails *entity,const char* name)
{
    FC_EntityLedgerRow aldRow;

    entity->Zero();
    aldRow.Zero();
    
    meFCpy(aldRow.m_Key,name,strlen(name));
    FC_StringLowerCase((char*)(aldRow.m_Key),FC_ENT_MAX_NAME_SIZE);
    aldRow.m_KeyType=FC_ENT_KEYTYPE_NAME;

    if(GetEntity(&aldRow))            
    {
        entity->Set(&aldRow);
        return 1;
    }            

    return 0;    
}

int FC_AssetDB::FindEntityByFollowOn(FC_EntityDetails *entity,const unsigned char* txid)
{
    FC_EntityLedgerRow aldRow;

    entity->Zero();
    aldRow.Zero();
    
    meFCpy(aldRow.m_Key,txid,FC_ENT_KEY_SIZE);
    aldRow.m_KeyType=FC_ENT_KEYTYPE_FOLLOW_ON | FC_ENT_KEYTYPE_TXID;

    if(GetEntity(&aldRow))            
    {
        m_Ledger->Open();
        
        if(aldRow.m_FirstPos < 0)
        {
            meFCpy(&aldRow,m_MemPool->GetRow(-aldRow.m_FirstPos-1),sizeof(FC_EntityLedgerRow));                                
        }
        else
        {
            m_Ledger->GetRow(aldRow.m_FirstPos,&aldRow);
        }
        m_Ledger->Close();
        entity->Set(&aldRow);
        return 1;
    }            

    return 0;
}



const char* FC_EntityDetails::GetName()
{
    uint32_t value_offset;
    size_t value_size;
    unsigned char dname_buf[6];
    
    if(m_LedgerRow.m_ScriptSize)
    {
        value_offset=FC_FindSpecialParamInDetailsScript(m_LedgerRow.m_Script,m_LedgerRow.m_ScriptSize,FC_ENT_SPRM_NAME,&value_size);
        if(value_offset == m_LedgerRow.m_ScriptSize)
        {
            strcpy((char*)dname_buf+1,"name");
            dname_buf[0]=0xff;
            value_offset=FC_FindNamedParamInDetailsScript(m_LedgerRow.m_Script,m_LedgerRow.m_ScriptSize,(char*)dname_buf,&value_size);
        }
        if(FC_gState->m_Features->Streams())
        {
            if(value_offset < m_LedgerRow.m_ScriptSize)
            {            
                if(value_size == 2)
                {
                    if((char)m_LedgerRow.m_Script[value_offset] == '*')
                    {
                        value_offset=m_LedgerRow.m_ScriptSize;
                        value_size=0;
                    }
                }
            }
        }
        if(value_offset < m_LedgerRow.m_ScriptSize)
        {            
            return (char*)(m_LedgerRow.m_Script+value_offset);
        }
    }
    
    return m_Name;
}

const unsigned char* FC_EntityDetails::GetTxID()
{
    return m_LedgerRow.m_Key;
}

const unsigned char* FC_EntityDetails::GetRef()
{
    return m_Ref;
}

int FC_EntityDetails::IsUnconfirmedGenesis()
{
    return ((int)FC_GetLE(m_Ref+4,4)<0) ? 1 : 0; 
}

const unsigned char* FC_EntityDetails::GetFullRef()
{
    return m_FullRef;
}

const unsigned char* FC_EntityDetails::GetShortRef()
{
    if(FC_gState->m_Features->ShortTxIDInTx())
    {
        return GetTxID()+FC_AST_SHORT_TXID_OFFSET;
    }    
    return m_Ref;
}

const unsigned char* FC_EntityDetails::GetScript()
{
    return m_LedgerRow.m_Script;
}

int FC_EntityDetails::GetAssetMultiple()
{
    int multiple;
    size_t size;
    void* ptr;
    
    multiple=1; 

    ptr=NULL;
    ptr=(void*)GetSpecialParam(FC_ENT_SPRM_ASSET_MULTIPLE,&size);
    
    if(ptr)
    {
        if(size==sizeof(multiple))
        {
            multiple=(int)FC_GetLE(ptr,size);
        }
    }
        
    if(multiple <= 0)
    {
        multiple=1;
    }
    
    return multiple;
}

int FC_EntityDetails::IsFollowOn()
{
    if(m_LedgerRow.m_KeyType & FC_ENT_KEYTYPE_FOLLOW_ON)
    {
        return 1;
    }
    return 0;
}

int FC_EntityDetails::AllowedFollowOns()
{
    unsigned char *ptr;
    size_t bytes;
    ptr=(unsigned char *)GetSpecialParam(FC_ENT_SPRM_FOLLOW_ONS,&bytes);
    if(ptr)
    {
        if((bytes>0) && (bytes<=4))
        {
            return (int)FC_GetLE(ptr,bytes);
        }
    }
    return 0;
}

int FC_EntityDetails::AnyoneCanWrite()
{
    unsigned char *ptr;
    size_t bytes;
    ptr=(unsigned char *)GetSpecialParam(FC_ENT_SPRM_ANYONE_CAN_WRITE,&bytes);
    if(ptr)
    {
        if((bytes>0) && (bytes<=4))
        {
            return (int)FC_GetLE(ptr,bytes);
        }
    }
    return 0;
}

uint32_t FC_EntityDetails::UpgradeStartBlock()
{
    unsigned char *ptr;
    size_t bytes;
    ptr=(unsigned char *)GetSpecialParam(FC_ENT_SPRM_UPGRADE_START_BLOCK,&bytes);
    if(ptr)
    {
        if((bytes>0) && (bytes<=4))
        {
            return (int)FC_GetLE(ptr,bytes);
        }
    }
    return 0;
}

int FC_EntityDetails::UpgradeProtocolVersion()
{
    unsigned char *ptr;
    size_t bytes;
    ptr=(unsigned char *)GetSpecialParam(FC_ENT_SPRM_UPGRADE_PROTOCOL_VERSION,&bytes);
    if(ptr)
    {
        if((bytes>0) && (bytes<=4))
        {
            return (int)FC_GetLE(ptr,bytes);
        }
    }
    return 0;
}


uint64_t FC_EntityDetails::GetQuantity()
{
    return m_LedgerRow.m_Quantity;
}

uint32_t FC_EntityDetails::GetEntityType()
{
    return m_LedgerRow.m_EntityType;
}

int32_t FC_EntityDetails::NextParam(uint32_t offset,uint32_t* param_value_start,size_t *bytes)
{
    int32_t new_offset;
    
    new_offset=(int32_t)FC_GetParamFromDetailsScript(m_LedgerRow.m_Script,m_LedgerRow.m_ScriptSize,offset,param_value_start,bytes);
    if(new_offset == (int32_t)m_LedgerRow.m_ScriptSize)
    {
        new_offset = -1;
    }
    
    return new_offset;
}

const void* FC_EntityDetails::GetSpecialParam(uint32_t param,size_t* bytes)
{
    uint32_t offset;
    offset=FC_FindSpecialParamInDetailsScript(m_LedgerRow.m_Script,m_LedgerRow.m_ScriptSize,param,bytes);
    if(offset == m_LedgerRow.m_ScriptSize)
    {
        return NULL;
    }
    return m_LedgerRow.m_Script+offset;
}

const void* FC_EntityDetails::GetParam(const char *param,size_t* bytes)
{
    uint32_t offset;
    offset=FC_FindNamedParamInDetailsScript(m_LedgerRow.m_Script,m_LedgerRow.m_ScriptSize,param,bytes);
    if(offset == m_LedgerRow.m_ScriptSize)
    {
        return NULL;
    }
    return m_LedgerRow.m_Script+offset;
}

void FC_AssetDB::Dump()
{    
    FC_EntityDBRow adbRow;
    FC_EntityLedgerRow aldRow;
    unsigned char *ptr;       
    int dbvalue_len,err,i;
    int64_t pos,total;
    int size,row_size;
    
        
    
    printf("\nDB\n");
    adbRow.Zero();
    
    ptr=(unsigned char*)m_Database->m_DB->Read((char*)&adbRow+m_Database->m_KeyOffset,m_Database->m_KeySize,&dbvalue_len,FC_OPT_DB_DATABASE_SEEK_ON_READ,&err);
    if(err)
    {
        return;
    }

    row_size=m_Database->m_TotalSize;
    while(row_size > 64)
    {
        row_size /= 2;
    }
    if(ptr)
    {
        meFCpy((unsigned char*)&adbRow+m_Database->m_ValueOffset,ptr,m_Database->m_ValueOffset);    
        while(ptr)
        {            
            FC_MemoryDumpCharSize((unsigned char*)&adbRow,0,m_Database->m_TotalSize,row_size);        
            ptr=(unsigned char*)m_Database->m_DB->MoveNext(&err);
            if(ptr)
            {
                meFCpy((unsigned char*)&adbRow,ptr,m_Database->m_TotalSize);                    
            }
        }
    }
    
    printf("Ledger\n");
    
    m_Ledger->Open();

    row_size=m_Ledger->m_TotalSize;
    while(row_size > 64)
    {
        row_size /= 2;
    }
    
    total=m_Ledger->GetSize();
    pos=0;
    
    while(pos<total)
    {        
        m_Ledger->GetRow(pos,&aldRow);

        size=FC_AllocSize(m_Ledger->m_TotalSize+aldRow.m_ScriptSize,m_Ledger->m_TotalSize,1);
        FC_DumpSize("",&aldRow,size,row_size);
        pos+=size;
    }
    
    m_Ledger->Close();
    
    printf("MemPool\n");
    for(i=0;i<m_MemPool->GetCount();i++)
    {
        meFCpy(&aldRow,m_MemPool->GetRow(i),sizeof(FC_EntityLedgerRow));
        size=FC_AllocSize(m_Ledger->m_TotalSize+aldRow.m_ScriptSize,m_Ledger->m_TotalSize,1);
        FC_DumpSize("",&aldRow,size,row_size);
        pos+=size;
    }
    
}

uint32_t FC_AssetDB::MaxEntityType()
{
    if(FC_gState->m_Features->Upgrades() == 0)
    {
        return FC_ENT_TYPE_STREAM_MAX; 
    }
    return FC_ENT_TYPE_MAX; 
}

int FC_AssetDB::MaxStoredIssuers()
{
    return FC_ENT_MAX_STORED_ISSUERS; 
}

FC_Buffer *FC_AssetDB::GetEntityList(FC_Buffer *old_result,const void* txid,uint32_t entity_type)
{    
    FC_EntityDBRow adbRow;
    FC_EntityLedgerRow aldRow;
    
    unsigned char *ptr;
    int dbvalue_len,err,i;        
    FC_Buffer *result;    
    
    if(old_result)
    {
        result=old_result;
    }
    else
    {
        result=new FC_Buffer;
        result->Initialize(FC_ENT_KEY_SIZE,FC_ENT_KEY_SIZE,FC_BUF_MODE_DEFAULT);
    }
    
    if(txid)
    {
        result->Add(txid,(unsigned char *)txid+FC_ENT_KEY_SIZE);
        return result;        
    }
    
    adbRow.Zero();

    ptr=(unsigned char*)m_Database->m_DB->Read((char*)&adbRow+m_Database->m_KeyOffset,m_Database->m_ValueOffset,&dbvalue_len,FC_OPT_DB_DATABASE_SEEK_ON_READ,&err);
    if(err)
    {
        delete result;
        return NULL;
    }

    if(ptr)
    {
        meFCpy((unsigned char*)&adbRow+m_Database->m_ValueOffset,ptr,dbvalue_len);   
        while(ptr)
        {
            if(adbRow.m_KeyType == FC_ENT_KEYTYPE_TXID)
            {
                if( (entity_type == 0) || (adbRow.m_EntityType == entity_type) )
                {
                    result->Add(adbRow.m_Key,NULL);
                }
            }
            ptr=(unsigned char*)m_Database->m_DB->MoveNext(&err);
            if(ptr)
            {
                meFCpy((unsigned char*)&adbRow,ptr,m_Database->m_TotalSize);                    
            }
        }
    }
    
    for(i=0;i<m_MemPool->GetCount();i++)
    {
        meFCpy(&aldRow,m_MemPool->GetRow(i),sizeof(FC_EntityLedgerRow));
        if(aldRow.m_KeyType == FC_ENT_KEYTYPE_TXID)
        {
            if( (entity_type == 0) || (aldRow.m_EntityType == entity_type) )
            {
                result->Add(aldRow.m_Key,NULL);
            }
        }
    }

    return result;
}

int64_t FC_AssetDB::GetTotalQuantity(FC_EntityDetails *entity)
{
    return GetTotalQuantity(&(entity->m_LedgerRow));
}

int64_t FC_AssetDB::GetTotalQuantity(FC_EntityLedgerRow *row)
{
    FC_EntityLedgerRow aldRow;
    int64_t pos,first_pos;
    int take_it,i;
    int64_t total;
    
    total=0;
    pos=row->m_ChainPos;
    first_pos=row->m_FirstPos;
    
    for(i=m_MemPool->GetCount()-1;i>=0;i--)
    {
        meFCpy(&aldRow,m_MemPool->GetRow(i),sizeof(FC_EntityLedgerRow));            
        if( (aldRow.m_KeyType  & FC_ENT_KEYTYPE_MASK) == FC_ENT_KEYTYPE_TXID)
        {
            if(aldRow.m_FirstPos == first_pos)
            {
                total+=aldRow.m_Quantity;
            }
        }
    }
        
    take_it=1;

    if(first_pos >= 0)
    {
        m_Ledger->Open();
        while(take_it)
        {
            m_Ledger->GetRow(pos,&aldRow);
            total+=aldRow.m_Quantity;
            if(pos != first_pos)
            {
                pos=aldRow.m_LastPos;
                if(pos<=0)
                {
                    take_it=0;
                    total=0xFFFFFFFFFFFFFFFF;
                }
            }
            else
            {
                take_it=0;
            }
        }
        m_Ledger->Close();
    }
    
    return total;
}

FC_Buffer *FC_AssetDB::GetFollowOns(const void* txid)
{
    FC_EntityLedgerRow aldRow;
    int64_t pos,first_pos;
    int take_it,i;
    FC_Buffer *result;
    result=new FC_Buffer;
        
    result->Initialize(FC_ENT_KEY_SIZE,FC_ENT_KEY_SIZE,FC_BUF_MODE_DEFAULT);

    result->Clear();
    
    aldRow.Zero();
    
    meFCpy(aldRow.m_Key,txid,FC_ENT_KEY_SIZE);
    aldRow.m_KeyType=FC_ENT_KEYTYPE_TXID;

    if(GetEntity(&aldRow))            
    {
        pos=aldRow.m_ChainPos;
        first_pos=aldRow.m_FirstPos;

        for(i=m_MemPool->GetCount()-1;i>=0;i--)
        {
            meFCpy(&aldRow,m_MemPool->GetRow(i),sizeof(FC_EntityLedgerRow));            
            if( (aldRow.m_KeyType  & FC_ENT_KEYTYPE_MASK) == FC_ENT_KEYTYPE_TXID)
            {
                if(aldRow.m_FirstPos != (-i-1))
                {
                    if(aldRow.m_FirstPos == first_pos)
                    {
                        result->Add(aldRow.m_Key,NULL);                
                    }
                }
            }
        }

        if(pos > 0)
        {
            take_it=1;

            m_Ledger->Open();
            while(take_it)
            {
                m_Ledger->GetRow(pos,&aldRow);
                result->Add(aldRow.m_Key,NULL);
                if(pos != first_pos)
                {
                    pos=aldRow.m_LastPos;
                    if(pos<=0)
                    {
                        take_it=0;
                        result->Clear();
                    }
                }
                else
                {
                    take_it=0;
                }
            }
            m_Ledger->Close();
        }
        else
        {
            result->Add(aldRow.m_Key,NULL);                            
        }
    }
    
    if(result->GetCount() == 0)
    {
        delete result;
        return NULL;
    }
    
    return result;
}



int FC_AssetDB::HasFollowOns(const void* txid)
{
    FC_EntityLedgerRow aldRow;
    int64_t first_pos;
    int i;
    
    aldRow.Zero();
    
    meFCpy(aldRow.m_Key,txid,FC_ENT_KEY_SIZE);
    aldRow.m_KeyType=FC_ENT_KEYTYPE_TXID;

    if(GetEntity(&aldRow))            
    {
        if(aldRow.m_FirstPos >= 0)
        {
            if(aldRow.m_FirstPos != aldRow.m_ChainPos)
            {
                return 1;
            }
        }
        first_pos=aldRow.m_FirstPos;

        for(i=0;i<m_MemPool->GetCount();i++)
        {
            meFCpy(&aldRow,m_MemPool->GetRow(i),sizeof(FC_EntityLedgerRow));            
            
            if(aldRow.m_FirstPos != (-i-1))
            {
                if(aldRow.m_FirstPos == first_pos)
                {
                    if(aldRow.m_KeyType & FC_ENT_KEYTYPE_FOLLOW_ON)
                    {
                        return 1;
                    }
                }
            }
        }
    }
    
    return 0;
}

void FC_AssetDB::FreeEntityList(FC_Buffer *assets)
{
    if(assets)
    {
        delete assets;
    }
}

uint32_t FC_GetABScriptType(void *ptr)
{
    return (uint32_t)FC_GetLE((unsigned char*)ptr+FC_AST_ASSET_SCRIPT_TYPE_OFFSET,FC_AST_ASSET_SCRIPT_TYPE_SIZE);
}

void FC_SetABScriptType(void *ptr,uint32_t type)
{
    FC_PutLE((unsigned char*)ptr+FC_AST_ASSET_SCRIPT_TYPE_OFFSET,&type,FC_AST_ASSET_SCRIPT_TYPE_SIZE);
}

uint32_t FC_GetABRefType(void *ptr)
{
    return (uint32_t)FC_GetLE((unsigned char*)ptr+FC_AST_ASSET_REF_TYPE_OFFSET,FC_AST_ASSET_REF_TYPE_SIZE);    
}

void FC_SetABRefType(void *ptr,uint32_t type)
{
    FC_PutLE((unsigned char*)ptr+FC_AST_ASSET_REF_TYPE_OFFSET,&type,FC_AST_ASSET_REF_TYPE_SIZE);    
}

int64_t FC_GetABQuantity(void *ptr)
{
    return (int64_t)FC_GetLE((unsigned char*)ptr+FC_AST_ASSET_QUANTITY_OFFSET,FC_AST_ASSET_QUANTITY_SIZE);        
}

void FC_SetABQuantity(void *ptr,int64_t quantity)
{
    FC_PutLE((unsigned char*)ptr+FC_AST_ASSET_QUANTITY_OFFSET,&quantity,FC_AST_ASSET_QUANTITY_SIZE);        
}

unsigned char* FC_GetABRef(void *ptr)
{
    return (unsigned char*)ptr;
}

void FC_SetABRef(void *ptr,void *ref)
{
    meFCpy(ptr,ref,FC_AST_ASSET_BUFFER_REF_SIZE);
}

void FC_ZeroABRaw(void *ptr)
{
    memset(ptr,0,FC_AST_ASSET_FULLREF_SIZE);
}

void FC_InitABufferMap(FC_Buffer *buf)
{
    buf->Initialize(FC_AST_ASSET_QUANTITY_OFFSET,FC_AST_ASSET_FULLREF_BUF_SIZE,FC_BUF_MODE_MAP);    
}

void FC_InitABufferDefault(FC_Buffer *buf)
{
    buf->Initialize(FC_AST_ASSET_QUANTITY_OFFSET,FC_AST_ASSET_FULLREF_BUF_SIZE,FC_BUF_MODE_DEFAULT);    
}


