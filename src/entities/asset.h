// Copyright (c) 2014-2017 Coin Sciences Ltd
// FloatChain code distributed under the GPLv3 license, see COPYING file.

#ifndef FLOATCHAIN_ASSET_H
#define	FLOATCHAIN_ASSET_H

#include "utils/declare.h"
#include "utils/dbwrapper.h"

#define FC_AST_ASSET_REF_SIZE        10
#define FC_AST_ASSET_BUF_TOTAL_SIZE  22
#define FC_AST_SHORT_TXID_OFFSET     16
#define FC_AST_SHORT_TXID_SIZE       16

#define FC_AST_ASSET_BUFFER_REF_SIZE        32
#define FC_AST_ASSET_FULLREF_SIZE           36
#define FC_AST_ASSET_QUANTITY_OFFSET        36
#define FC_AST_ASSET_QUANTITY_SIZE           8
#define FC_AST_ASSET_FULLREF_BUF_SIZE       48

#define FC_AST_ASSET_REF_TYPE_REF            0
#define FC_AST_ASSET_REF_TYPE_SHORT_TXID     1
#define FC_AST_ASSET_REF_TYPE_TXID           2

#define FC_AST_ASSET_REF_TYPE_GENESIS      256 
#define FC_AST_ASSET_REF_TYPE_SPECIAL      512


#define FC_ENT_REF_SIZE                 10
#define FC_ENT_REF_PREFIX_SIZE           2
#define FC_ENT_MAX_NAME_SIZE            32
#define FC_ENT_MAX_ITEM_KEY_SIZE       256
#define FC_ENT_MAX_SCRIPT_SIZE        4096
#define FC_ENT_MAX_FIXED_FIELDS_SIZE   128 
#define FC_ENT_MAX_STORED_ISSUERS      128 
#define FC_ENT_SCRIPT_ALLOC_SIZE      8192 // > FC_ENT_MAX_SCRIPT_SIZE + FC_ENT_MAX_FIXED_FIELDS_SIZE + 27*FC_ENT_MAX_STORED_ISSUERS

#define FC_ENT_KEY_SIZE              32
#define FC_ENT_KEYTYPE_TXID           0x00000001
#define FC_ENT_KEYTYPE_REF            0x00000002
#define FC_ENT_KEYTYPE_NAME           0x00000003
#define FC_ENT_KEYTYPE_SHORT_TXID     0x00000004
#define FC_ENT_KEYTYPE_MASK           0x000000FF
#define FC_ENT_KEYTYPE_FOLLOW_ON      0x00000100

#define FC_ENT_TYPE_ANY               0xFF
#define FC_ENT_TYPE_NONE              0x00
#define FC_ENT_TYPE_ASSET             0x01
#define FC_ENT_TYPE_STREAM            0x02
#define FC_ENT_TYPE_STREAM_MAX        0x0F
#define FC_ENT_TYPE_UPGRADE           0x10
#define FC_ENT_TYPE_MAX               0x10

#define FC_ENT_SPRM_NAME                      0x01
#define FC_ENT_SPRM_FOLLOW_ONS                0x02
#define FC_ENT_SPRM_ISSUER                    0x03
#define FC_ENT_SPRM_ANYONE_CAN_WRITE          0x04
#define FC_ENT_SPRM_ASSET_MULTIPLE            0x41
#define FC_ENT_SPRM_UPGRADE_PROTOCOL_VERSION  0x42
#define FC_ENT_SPRM_UPGRADE_START_BLOCK       0x43

#define FC_ENT_FLAG_OFFSET_IS_SET     0x00000001
#define FC_ENT_FLAG_NAME_IS_SET       0x00000010



/** Database record structure */

typedef struct FC_EntityDBRow
{    
    unsigned char m_Key[FC_ENT_KEY_SIZE];                                       // Entity key size - txid/entity-ref/name
    uint32_t m_KeyType;                                                         // Entity key type - FC_ENT_KEYTYPE_ constants
    int32_t m_Block;                                                            // Block entity is confirmed in
    int64_t m_LedgerPos;                                                        // Position in the ledger corresponding to this key
    int64_t m_ChainPos;                                                         // Position in the ledger corresponding to last object in the chain
    uint32_t m_EntityType;                                                      // Entity type - FC_ENT_TYPE_ constants
    uint32_t m_Reserved1;                                                       // Reserved to align to 64 bytes
    
    void Zero();
} FC_EntityDBRow;

/** Database */

typedef struct FC_EntityDB
{   
    char m_FileName[FC_DCT_DB_MAX_PATH];                                        // Full file name
    FC_Database *m_DB;                                                          // Database object
    uint32_t m_KeyOffset;                                                       // Offset of the key in FC_EntityDBRow structure, 0
    uint32_t m_KeySize;                                                         // Size of the database key, 36
    uint32_t m_ValueOffset;                                                     // Offset of the value in FC_EntityDBRow structure, 36 
    uint32_t m_ValueSize;                                                       // Size of the database value, 12 for protocol<=10003,28 otherwise 
    uint32_t m_TotalSize;                                                       // Totals size of the database row
    FC_EntityDB()
    {
        Zero();
    }
    
    ~FC_EntityDB()
    {
        Close();
    }
    void Zero();
    int Open();
    int Close();
    void SetName(const char *name);
} FC_EntityDB;

/** Ledger and mempool record structure */

typedef struct FC_EntityLedgerRow
{    
    unsigned char m_Key[FC_ENT_KEY_SIZE];                                       // Entity key size - txid/entity-ref/name
    uint32_t m_KeyType;                                                         // Entity key type - FC_ENT_KEYTYPE_ constants
    int32_t m_Block;                                                            // Block entity is confirmed in
    int32_t m_Offset;                                                           // Offset of the entity in the block
    uint32_t m_ScriptSize;                                                      // Script Size
    int64_t m_Quantity;                                                         // Total quantity of the entity (including follow-ons)
    uint32_t m_EntityType;                                                      // Entity type - FC_ENT_TYPE_ constants
    uint32_t m_Reserved1;                                                       // Reserved to align to 96 bytes
    int64_t m_PrevPos;                                                          // Position of the previous entity in the ledger
    int64_t m_FirstPos;                                                         // Position in the ledger corresponding to first object in the chain
    int64_t m_LastPos;                                                          // Position in the ledger corresponding to last object in the chain before this object
    int64_t m_ChainPos;                                                         // Position in the ledger corresponding to last object in the chain
    unsigned char m_Script[FC_ENT_SCRIPT_ALLOC_SIZE];                           // Script > FC_ENT_MAX_SCRIPT_SIZE + FC_ENT_MAX_FIXED_FIELDS_SIZE + 27*FC_ENT_MAX_STORED_ISSUERS
    
    void Zero();
} FC_EntityLedgerRow;

/** Entity details structure */

typedef struct FC_EntityDetails
{
    unsigned char m_Ref[FC_ENT_REF_SIZE];                                       // Entity reference
    unsigned char m_FullRef[FC_AST_ASSET_QUANTITY_OFFSET];                      // Full Entity reference, derived from short txid from v 10007
    char m_Name[FC_ENT_MAX_NAME_SIZE+6];                                        // Entity name
    uint32_t m_Flags;
    unsigned char m_Reserved[36];   
    FC_EntityLedgerRow m_LedgerRow;
    void Zero();
    void Set(FC_EntityLedgerRow *row);
    const char* GetName();
    const unsigned char* GetTxID();
    const unsigned char* GetRef();    
    const unsigned char* GetFullRef();    
    const unsigned char* GetShortRef();
    const unsigned char* GetScript();    
    int IsUnconfirmedGenesis();    
    int GetAssetMultiple();
    int IsFollowOn(); 
//    int HasFollowOns(); 
    int AllowedFollowOns(); 
    int AnyoneCanWrite(); 
    int UpgradeProtocolVersion(); 
    uint32_t UpgradeStartBlock(); 
    uint64_t GetQuantity();
    uint32_t GetEntityType();    
    const void* GetSpecialParam(uint32_t param,size_t* bytes);
    const void* GetParam(const char *param,size_t* bytes);
    int32_t NextParam(uint32_t offset,uint32_t* param_value_start,size_t *bytes);
}FC_EntityDetails;

/** Ledger */

typedef struct FC_EntityLedger
{   
    char m_FileName[FC_DCT_DB_MAX_PATH];                                        // Full file name
    int m_FileHan;                                                              // File handle
    uint32_t m_KeyOffset;                                                       // Offset of the key in FC_EntityLedgerRow structure, 0
    uint32_t m_KeySize;                                                         // Size of the ledger key, 36
    uint32_t m_ValueOffset;                                                     // Offset of the value in FC_EntityLedgerRow structure, 36 
    uint32_t m_ValueSize;                                                       // Size of the ledger value 28 if protocol<=10003, 60 otherwise
    uint32_t m_TotalSize;                                                       // Totals size of the ledger row
   
    FC_EntityLedger()
    {
        Zero();
    }
    
    ~FC_EntityLedger()
    {
        Close();
    }
    
    void Zero();
    int Open();
    int Close();
    void Flush();
    void SetName(const char *name);
    int GetRow(int64_t pos,FC_EntityLedgerRow *row);
    int64_t GetSize();
    int SetRow(int64_t pos,FC_EntityLedgerRow *row);
    int SetZeroRow(FC_EntityLedgerRow *row);
    
} FC_EntityLedger;


typedef struct FC_AssetDB
{    
    FC_EntityDB *m_Database;
    FC_EntityLedger *m_Ledger;
    
    FC_Buffer   *m_MemPool;
    
    char m_Name[FC_PRM_NETWORK_NAME_MAX_SIZE+1]; 
    int m_Block;
    int64_t m_PrevPos;
    int64_t m_Pos;
    int m_DBRowCount;

    FC_AssetDB()
    {
        Zero();
    }
    
    ~FC_AssetDB()
    {
        Destroy();
    }
    

// External functions    

    int Initialize(const char *name,int mode);
        
    int InsertEntity(const void* txid, int offset, int entity_type, const void *script,size_t script_size, const void* special_script, size_t special_script_size,int update_mempool);
    int InsertAsset(const void* txid, int offset, uint64_t quantity,const char *name,int multiple,const void *script,size_t script_size, const void* special_script, size_t special_script_size,int update_mempool);
    int InsertAssetFollowOn(const void* txid, int offset, uint64_t quantity, const void *script,size_t script_size, const void* special_script, size_t special_script_size,const void* original_txid,int update_mempool);
    int Commit();
    int RollBack(int block);
    int RollBack();
    int ClearMemPool();
    
    int GetEntity(FC_EntityLedgerRow *row);

    int FindEntityByTxID(FC_EntityDetails *entity, const unsigned char* txid);
    int FindEntityByShortTxID (FC_EntityDetails *entity, const unsigned char* short_txid);
    int FindEntityByRef (FC_EntityDetails *entity, const unsigned char* asset_ref);
    int FindEntityByName(FC_EntityDetails *entity, const char* name);    
    int FindEntityByFollowOn(FC_EntityDetails *entity, const unsigned char* txid);    
    int FindEntityByFullRef (FC_EntityDetails *entity, unsigned char* full_ref);
    
    void Dump();
    FC_Buffer *GetEntityList(FC_Buffer *old_result,const void* txid,uint32_t entity_type);
    void FreeEntityList(FC_Buffer *entities);
    FC_Buffer *GetFollowOns(const void* txid);
    int HasFollowOns(const void* txid);
    int64_t GetTotalQuantity(FC_EntityDetails *entity);
    
    void RemoveFiles();
    uint32_t MaxEntityType();
    int MaxStoredIssuers();
    
//Internal functions    
    int Zero();
    int Destroy();
    int64_t GetTotalQuantity(FC_EntityLedgerRow *row);
        
} FC_AssetDB;



uint32_t FC_GetABScriptType(void *ptr);
void FC_SetABScriptType(void *ptr,uint32_t type);
uint32_t FC_GetABRefType(void *ptr);
void FC_SetABRefType(void *ptr,uint32_t type);
int64_t FC_GetABQuantity(void *ptr);
void FC_SetABQuantity(void *ptr,int64_t quantity);
unsigned char* FC_GetABRef(void *ptr);
void FC_SetABRef(void *ptr,void *ref);
void FC_ZeroABRaw(void *ptr);
void FC_InitABufferMap(FC_Buffer *buf);
void FC_InitABufferDefault(FC_Buffer *buf);


#endif	/* FLOATCHAIN_ASSET_H */

