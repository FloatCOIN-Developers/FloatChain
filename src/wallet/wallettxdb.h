// Copyright (c) 2014-2017 Coin Sciences Ltd
// FloatChain code distributed under the GPLv3 license, see COPYING file.

#ifndef FLOATCHAIN_WALLETTXDB_H
#define	FLOATCHAIN_WALLETTXDB_H

#include "utils/declare.h"
#include "utils/dbwrapper.h"

#define FC_TDB_TXID_SIZE             32
#define FC_TDB_ENTITY_KEY_SIZE       32
#define FC_TDB_ENTITY_ID_SIZE        20
#define FC_TDB_ENTITY_TYPE_SIZE       4
#define FC_TDB_GENERATION_SIZE        4
#define FC_TDB_POS_SIZE               4
#define FC_TDB_ROW_SIZE              80

#define FC_TDB_MAX_IMPORTS           16

#define FC_TDB_WALLET_VERSION         2


#define FC_TET_NONE                             0x00000000
#define FC_TET_WALLET_ALL                       0x00000001
#define FC_TET_WALLET_SPENDABLE                 0x00000002
#define FC_TET_PUBKEY_ADDRESS                   0x00000003
#define FC_TET_SCRIPT_ADDRESS                   0x00000004
#define FC_TET_STREAM                           0x00000005
#define FC_TET_STREAM_KEY                       0x00000006
#define FC_TET_STREAM_PUBLISHER                 0x00000007
#define FC_TET_ASSET                            0x00000008
#define FC_TET_SUBKEY_STREAM_KEY                0x00000046
#define FC_TET_SUBKEY_STREAM_PUBLISHER          0x00000047
#define FC_TET_SUBKEY                           0x00000040
#define FC_TET_TYPE_MASK                        0x000000FF
#define FC_TET_CHAINPOS                         0x00000100
#define FC_TET_TIMERECEIVED                     0x00000200
#define FC_TET_ORDERMASK                        0x0000FF00
#define FC_TET_DB_STAT                          0x01000000
#define FC_TET_IMPORT                           0x02000000
#define FC_TET_SPECIALMASK                      0xFF000000

#define FC_EFL_NOT_IN_SYNC                      0x01000000
#define FC_EFL_UNSUBSCRIBED                     0x10000000

#define FC_SFL_NONE             0x00000000
#define FC_SFL_IS_SCRIPTHASH    0x00000001
#define FC_SFL_IS_ADDRESS       0x00000002
#define FC_SFL_NODATA           0x01000000
#define FC_SFL_SUBKEY           0x02000000


/** Entity - wallet, address, stream, etc. **/

typedef struct FC_TxEntity
{
    unsigned char m_EntityID[FC_TDB_ENTITY_ID_SIZE];                            // 20-byte entity id = pubkeyhash, scripthash, stream-reference
    uint32_t m_EntityType;                                                      // Entity type, FC_TET_ constants
    void Zero();
    void Init(unsigned char *entity_id,uint32_t entity_type);
} FC_TxEntity;

/** Entity row pos->txid **/

typedef struct FC_TxEntityRow
{
    FC_TxEntity m_Entity;                                                       // Entity
    int m_Generation;                                                           // Generation of entity data
    uint32_t m_Pos;                                                             // position of the row in generation, 1-based
    unsigned char m_TxId[FC_TDB_TXID_SIZE];                                     // txID  
    int m_Block;                                                                // block confirmed in
    uint32_t m_Flags;                                                           // Flags passed from higher level
    uint32_t m_LastSubKeyPos;                                                   // Position of the last subkey row (only for the first)
    uint32_t m_TempPos;                                                         // Not stored in DB. Equal to m_Pos, added for ability to search in mempool
    void Zero();
    void SwapPosBytes();                                                            
} FC_TxEntityRow;

/** Entity stats - last position, etc. In-memory structure **/

typedef struct FC_TxEntityStat
{
    FC_TxEntity m_Entity;                                                       // Entity
    uint32_t m_PosInImport;                                                     // Position of the entity in the import
    uint32_t m_KeyReserved1;
    uint64_t m_ValueReserved1;
    uint64_t m_ValueReserved2;
    uint64_t m_ValueReserved3;
    uint32_t m_TimeAdded;
    uint32_t m_Flags;
    int m_Generation;                                                           // Generation of entity within import
    int m_LastImportedBlock;                                                    // Last imported block before merging with chain. O for the chain import.
    uint32_t m_LastPos;                                                         // Current last position
    uint32_t m_LastClearedPos;                                                  // Last position without  mempool
    void Zero();    
} FC_TxEntityStat;

/** Import row - image of FC_TxEntityStat on disk **/

typedef struct FC_TxImportRow
{
    unsigned char m_Zero[FC_TDB_ENTITY_ID_SIZE];                                       
    uint32_t m_RowType;                                                         // FC_TET_IMPORT
    int m_ImportID;                                                             // Import ID.
    uint32_t m_Pos;                                                             // Entity Position within Import, 1-based
    FC_TxEntity m_Entity;                                                       // Entity
    int m_Block;                                                                // Last block
    int m_LastImportedBlock;                                                    // Last imported block before merging with chain. O for the chain import.
    uint32_t m_LastPos;                                                         // Current last position
    int m_Generation;                                                           // Generation of entity within import
    uint32_t m_TimeAdded;
    uint32_t m_Flags;
    void Zero();
    void SwapPosBytes();                                                            
} FC_TxImportRow;

/** Global database stats row **/

typedef struct FC_TxEntityDBStat
{
    unsigned char m_Zero[FC_TDB_ENTITY_ID_SIZE];                                       
    uint32_t m_RowType;                                                         // FC_TET_DB_STAT
    uint64_t m_KeyReserved1;
    uint64_t m_ValueReserved1;
    uint64_t m_ValueReserved2;
    uint32_t m_InitMode;
    uint32_t m_WalletVersion;
    int m_Block;                                                                // Last block of the chain import
    int m_LastGeneration;                                                       // Last ImportID used
    uint32_t m_Count;                                                           // Total tx count
    uint32_t m_FullSize;                                                        // Total tx size
    uint32_t m_LastFileID;                                                      // Last data file ID
    uint32_t m_LastFileSize;                                                    // Last data file size
    void Zero();
} FC_TxEntityDBStat;

/** Import structure, in-memory **/

typedef struct FC_TxImport
{
    int m_ImportID;                                                             // Import ID
    int m_Block;                                                                // last block index
    FC_Buffer *m_Entities;                                                     // List of import entities (FC_TxEntityStat)
    FC_Buffer *m_TmpEntities;                                                  // Temporary list of entities (FC_TxEntity)
    FC_TxImport()
    {
        Zero();
    }
    
    ~FC_TxImport()
    {
        Destroy();
    }
    void Zero();
    int Init(int generation,int block);
    int AddEntity(FC_TxEntity *entity);
    int AddEntity(FC_TxEntityStat *entity);
    int FindEntity(FC_TxEntity *entity);
    int FindEntity(FC_TxEntityStat *entity);
    FC_TxEntityStat *GetEntity(int row);
    void Destroy();
} FC_TxImport;


/** Tx definition row **/

typedef struct FC_TxDefRow
{
    unsigned char m_TxId[FC_TDB_TXID_SIZE];                                     // TxID
    uint32_t m_Size;                                                            // Tx Size
    uint32_t m_FullSize;                                                        // Original Tx size, before chopping long OP_RETURNs
    uint32_t m_InternalFileID;                                                  // Data file ID
    uint32_t m_InternalFileOffset;                                              // Offset in the data file
    int m_Block;                                                                // Block confirmed in
    int m_BlockFileID;                                                          // Block file ID
    uint32_t m_BlockOffset;                                                     // Position of the block in the block file
    uint32_t m_BlockTxOffset;                                                   // Position of the tx within block (relative to header))
    uint32_t m_TimeReceived;                                                    // Time received by this wallet
    uint32_t m_Flags;                                                           // Flags passed from higher level
    uint32_t m_BlockIndex;                                                      // Tx index in block
    uint32_t m_ValueReserved1;
    void Zero();
} FC_TxDefRow;

/** Internal database **/

typedef struct FC_TxEntityDB
{   
    char m_FileName[FC_DCT_DB_MAX_PATH];                                        // Full file name
    FC_Database *m_DB;                                                          // Database object
    uint32_t m_KeyOffset;                                                       // Offset of the key in FC_PermissionDBRow structure, 32 for protocol<=10003, 0 otherwise
    uint32_t m_KeySize;                                                         // Size of the database key, 24 for protocol<=10003, 56 otherwise
    uint32_t m_ValueOffset;                                                     // Offset of the value in FC_PermissionDBRow structure, 56 
    uint32_t m_ValueSize;                                                       // Size of the database value, 24
    uint32_t m_TotalSize;                                                       // Totals size of the database row
    FC_TxEntityDB()
    {
        Zero();
    }
    
    ~FC_TxEntityDB()
    {
        Close();
    }
    void Zero();
    int Open();
    int Close();
    void SetName(const char *name);
} FC_TxEntityDB;

/** Transactions DB **/

typedef struct FC_TxDB
{    
    FC_TxEntityDB *m_Database;                                                  // Database 
    FC_Buffer *m_MemPools[FC_TDB_MAX_IMPORTS];                                  // FC_TxEntityRow mempool
    FC_Buffer *m_RawMemPools[FC_TDB_MAX_IMPORTS];                               // FC_TxDefRow mempool
    FC_Buffer *m_RawUpdatePool;                                                 // Updated txs mempool
    FC_TxImport m_Imports[FC_TDB_MAX_IMPORTS];                                  // Imports, 0 - chain
    FC_TxEntityDBStat m_DBStat;                                                 // Database stats
    
    char m_Name[FC_PRM_NETWORK_NAME_MAX_SIZE+1];                                // Chain name
    char m_LobFileNamePrefix[FC_DCT_DB_MAX_PATH];                               // Full data file name
    char m_LogFileName[FC_DCT_DB_MAX_PATH];                                     // Full log file name    
    
    uint32_t m_Mode;
    void *m_Semaphore;                                                          // FC_TxDB object semaphore
    uint64_t m_LockedBy;                                                        // ID of the thread locking it

    FC_TxDB()
    {
        Zero();
    }
    
    ~FC_TxDB()
    {
        Destroy();
    }
    
    int Initialize(                                                             // Initalization
              const char *name,                                                 // Chain name
              uint32_t mode);                                                   // Unused

    int AddEntity(FC_TxEntity *entity,uint32_t flags);                          // Adds entity to chain import
    int AddEntity(FC_TxImport *import,FC_TxEntity *entity,uint32_t flags);      // Adds entity to import
       
    int FindEntity(FC_TxImport *import,FC_TxEntity *entity);                    // Finds entity in import
    int FindEntity(FC_TxImport *import,FC_TxEntityStat *entity);                // Finds entity in import

    int FindSubKey(
              FC_TxImport *import,                                              // Import object, if NULL - chain
              FC_TxEntity *parent_entity,                                       // Parent entity - stream
              FC_TxEntity *entity);                                             // Subkey entity
    
    int AddSubKeyDef(
              FC_TxImport *import,                                              // Import object, if NULL - chain
              const unsigned char *hash,                                        // SubKey hash 
              const unsigned char *subkey,                                      // Subkey
              uint32_t subkeysize,                                              // Subkey size
              uint32_t flags                                                    // Subkey flags
    );
    
    int IncrementSubKey(
              FC_TxImport *import,                                              // Import object, if NULL - chain
              FC_TxEntity *parent_entity,                                       // Parent entity - stream
              FC_TxEntity *entity,                                              // Subkey entity
              const unsigned char *subkey_hash,                                 // Subkey hash
              const unsigned char *tx_hash,                                     // Tx hash (before chopping)    
              int block,                                                        // Block we are processing now, -1 for mempool
              uint32_t flags,                                                   // Flags passed by the higher level                
              int newtx                                                         // New tx flag
    );
    
    int DecrementSubKey(
              FC_TxImport *import,                                              // Import object, if NULL - chain
              FC_TxEntity *parent_entity,                                       // Parent entity - stream
              FC_TxEntity *entity);                                             // Subkey entity
    
    int AddTx(                                                                  // Adds transaction to mempool
              FC_TxImport *import,                                              // Import object, if NULL - chain
              const unsigned char *hash,                                        // Tx hash (before chopping)    
              const unsigned char *tx,                                          // Tx (chopped))
              uint32_t txsize,                                                  // Tx size
              uint32_t txfullsize,                                              // Original Tx size, before chopping long OP_RETURNs
              int block,                                                        // Block we are processing now, -1 for mempool
              int block_file,                                                   // Block file ID
              uint32_t block_offset,                                            // Block offset in the block file
              uint32_t block_tx_offset,                                         // Tx offset in the block, relative to the header
              uint32_t block_tx_index,                                          // Tx index in block
              uint32_t flags,                                                   // Flags passed by the higher level                
              uint32_t timestamp,                                               // timestamp to be stored as timereceived
              FC_Buffer *entities);                                             // List of relevant entities for this tx
    
    int GetTx(                                                                  // Returns tx definition if found, error if not found
              FC_TxDefRow *txdef,                                               // Output. Tx def
              const unsigned char *hash);                                       // Input. Tx hash
    
    int GetList(
                FC_TxImport *import,                                            // Import object, if NULL - chain
                FC_TxEntity *entity,                                            // Returns Txs in range for specific entity
                    int from,                                                   // If positive - from this tx, if not positive - this number from the end
                    int count,                                                  // Number of txs to return
                    FC_Buffer *txs);                                            // Output list. FC_TxEntityRow
    
    int GetList(FC_TxEntity *entity,                                            // Returns Txs in range for specific entity
                    int from,                                                   // If positive - from this tx, if not positive - this number from the end
                    int count,                                                  // Number of txs to return
                    FC_Buffer *txs);                                            // Output list. FC_TxEntityRow

    int GetList(
                FC_TxImport *import,                                            // Import object, if NULL - chain
                FC_TxEntity *entity,                                            // Returns Txs in range for specific entity
                    int generation,                                             // Entity generation
                    int from,                                                   // If positive - from this tx, if not positive - this number from the end
                    int count,                                                  // Number of txs to return
                    FC_Buffer *txs);                                            // Output list. FC_TxEntityRow
    
    int GetList(FC_TxEntity *entity,                                            // Returns Txs in range for specific entity
                    int generation,                                             // Entity generation
                    int from,                                                   // If positive - from this tx, if not positive - this number from the end
                    int count,                                                  // Number of txs to return
                    FC_Buffer *txs);                                            // Output list. FC_TxEntityRow

    int GetRow(
               FC_TxEntityRow *erow);
    
    int GetListSize(                                                            // Return total number of tx in the list for specific entity
                    FC_TxEntity *entity,                                        // Entity to return info for
                    int *confirmed);                                            // Out: number of confirmed items    
    
    int GetListSize(                                                            // Return total number of tx in the list for specific entity
                    FC_TxEntity *entity,                                        // Entity to return info for
                    int generation,                                             // Entity generation
                    int *confirmed);                                            // Out: number of confirmed items    
    
    int GetBlockItemIndex(                                                      // Returns item id for the last item confirmed in this block or before
                    FC_TxImport *import,                                        // Import object, if NULL - chain
                    FC_TxEntity *entity,                                        // Entity to return info for
                    int block);                                                 // Block to find item for
    
    int BeforeCommit(FC_TxImport *import);                                      // Should be called before re-adding tx while processing block
    int Commit(FC_TxImport *import);                                            // Commit when block was processed
    int RollBack(FC_TxImport *import,int block);                                // Rollback to specific block

    int Unsubscribe(FC_Buffer *lpEntities);                                     // List of the entities to unsubscribe from
    
    FC_TxImport *StartImport(                                                   // Starts new import
                             FC_Buffer *lpEntities,                             // List of entities to import
                             int block,                                         // Star from this block            
                             int *err);                                         // Output. Error
    
    FC_TxImport *FindImport(                                                    // Find import with specific ID
                            int import_id);
    
    int ImportGetBlock(                                                         // Returns last processed block in the import
                       FC_TxImport *import);
    
    int CompleteImport(FC_TxImport *import);                                    // Completes import - merges with chain
    
    int DropImport(FC_TxImport *import);                                        // Drops uncompleted import

    int SaveTxFlag(                                                             // Changes tx flag setting (if tx is found))
                   const unsigned char *hash,                                   // Tx ID
                   uint32_t flag,                                               // Flag to set/unset
                   int set_flag);                                               // 1 if set, 0 if unset
    
// Internal functions    
    void Zero();    
    int Destroy();
    void Dump(const char *message);
    
    int Lock(int write_mode, int allow_secondary);
    void UnLock();
    
    int AddToFile(const unsigned char *tx,
                  uint32_t txsize,
                  uint32_t fileid,
                  uint32_t offset);
    
    int FlushDataFile(uint32_t fileid);
    
    void LogString(const char *message);
    
    
} FC_TxDB;


#endif	/* FLOATCHAIN_WALLETTXDB_H */

