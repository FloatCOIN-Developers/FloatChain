// Copyright (c) 2014-2016 The Bitcoin Core developers
// Original code was distributed under the MIT software license.
// Copyright (c) 2014-2017 Coin Sciences Ltd
// FloatChain code distributed under the GPLv3 license, see COPYING file.

#ifndef FLOATCHAIN_WALLETTXS_H
#define	FLOATCHAIN_WALLETTXS_H

#include "utils/util.h"
#include "structs/base58.h"
#include "wallet/wallet.h"
#include "floatchain/floatchain.h"
#include "wallet/wallettxdb.h"

#define FC_TDB_MAX_OP_RETURN_SIZE             256

typedef struct FC_WalletCachedSubKey
{
    FC_TxEntity m_Entity;
    FC_TxEntity m_SubkeyEntity;
    uint160 m_SubKeyHash;
    uint32_t m_Flags;
    
    void Zero();
    void Set(FC_TxEntity* entity,FC_TxEntity* subkey_entity,uint160 subkey_hash,uint32_t flags);
    
    FC_WalletCachedSubKey()
    {
        Zero();
    }
    
} FC_WalletCachedSubKey;

typedef struct FC_WalletCachedAddTx
{
    std::vector<FC_Coin> m_TxOutsIn;
    std::vector<FC_Coin> m_TxOutsOut;
    std::vector<FC_WalletCachedSubKey> m_SubKeys;
    std::vector<FC_TxEntity> m_Entities;
    uint32_t m_Flags;
    bool fSingleInputEntity;

    
} FC_WalletCachedAddTx;

typedef struct FC_WalletTxs
{
    FC_TxDB *m_Database;
    CWallet *m_lpWallet;
    uint32_t m_Mode;
    std::map<COutPoint, FC_Coin> m_UTXOs[FC_TDB_MAX_IMPORTS];    
    std::map<uint256,CWalletTx> m_UnconfirmedSends;
    std::vector<uint256> m_UnconfirmedSendsHashes;
    std::map<uint256, CWalletTx> vAvailableCoins;    
    
    FC_WalletTxs()
    {
        Zero();
    }
    
    ~FC_WalletTxs()
    {
        Destroy();
    }
    
    int Initialize(                                                             // Initialization
              const char *name,                                                 // Chain name
              uint32_t mode);                                                   // Unused

    int SetMode(                                                                // Sets wallet mode
                uint32_t mode,                                                  // Mode to set
                uint32_t mask);                                                 // Mask to set, old mode outside this mask will be untouched
    
    void BindWallet(CWallet *lpWallet);
    
    int AddEntity(FC_TxEntity *entity,uint32_t flags);                          // Adds entity to chain import
    FC_Buffer* GetEntityList();                                                 // Retruns list of chain import entities
       
    int AddTx(                                                                  // Adds tx to the wallet
              FC_TxImport *import,                                              // Import object, NULL if chain update
              const CTransaction& tx,                                           // Tx to add
              int block,                                                        // block height, -1 for mempool
              CDiskTxPos* block_pos,                                            // Position in the block
              uint32_t block_tx_index,                                          // Tx index in block
              uint256 block_hash);                                              // Block hash
                                           
    
    int AddTx(                                                                  // Adds tx to the wallet
              FC_TxImport *import,                                              // Import object, NULL if chain update
              const CWalletTx& tx,                                              // Tx to add
              int block,                                                        // block height, -1 for mempool
              CDiskTxPos* block_pos,                                            // Position in the block
              uint32_t block_tx_index,                                          // Tx index in block
              uint256 block_hash);                                              // Block hash
    
    int BeforeCommit(FC_TxImport *import);                                      // Should be called before re-adding tx while processing block
    int Commit(FC_TxImport *import);                                            // Commit when block was processed
    int RollBack(FC_TxImport *import,int block);                                // Rollback to specific block
    int CleanUpAfterBlock(FC_TxImport *import,int block,int prev_block);        // Should be called after full processing of the block, prev_block - block before UpdateTip
    
    int FindWalletTx(uint256 hash,FC_TxDefRow *txdef);                          // Returns only txdef
    CWalletTx GetWalletTx(uint256 hash,FC_TxDefRow *txdef,int *errOut);         // Returns wallet transaction. If not found returns empty transaction
    
    CWalletTx GetInternalWalletTx(uint256 hash,FC_TxDefRow *txdef,int *errOut);   
    
    int GetBlockItemIndex(                                                      // Returns item id for the last item confirmed in this block or before
                    FC_TxEntity *entity,                                        // Entity to return info for
                    int block);                                                 // Block to find item for
    
    
    int GetList(FC_TxEntity *entity,                                            // Returns Txs in range for specific entity
                    int from,                                                   // If positive - from this tx, if not positive - this number from the end
                    int count,                                                  // Number of txs to return
                    FC_Buffer *txs);                                            // Output list. FC_TxEntityRow
    
    int GetList(FC_TxEntity *entity,                                            // Returns Txs in range for specific entity
                    int generation,                                             // Entity generation
                    int from,                                                   // If positive - from this tx, if not positive - this number from the end
                    int count,                                                  // Number of txs to return
                    FC_Buffer *txs);                                            // Output list. FC_TxEntityRow
    
    int GetListSize(                                                            // Return total number of tx in the list for specific entity
                    FC_TxEntity *entity,                                       // Entity to return info for
                    int *confirmed);                                            // Out: number of confirmed items    

    int GetListSize(                                                            // Return total number of tx in the list for specific entity
                    FC_TxEntity *entity,                                        // Entity to return info for
                    int generation,                                            // Entity generation
                    int *confirmed);                                            // Out: number of confirmed items    
    
    int GetRow(
               FC_TxEntityRow *erow);
    
    int Unsubscribe(FC_Buffer *lpEntities);                                     // List of the entities to unsubscribe from
    
    FC_TxImport *StartImport(                                                   // Starts new import
                             FC_Buffer *lpEntities,                             // List of entities to import
                             int block,                                         // Star from this block            
                             int *err);                                         // Output. Error
    
    FC_TxImport *FindImport(                                                    // Find import with specific ID
                            int import_id);
    
    bool FindEntity(FC_TxEntityStat *entity);                                    // Finds entity in chain import
    
    int ImportGetBlock(                                                         // Returns last processed block in the import
                       FC_TxImport *import);
    
    int CompleteImport(FC_TxImport *import);                                    // Completes import - merges with chain
    
    int DropImport(FC_TxImport *import);                                        // Drops uncompleted import

    std::string GetSubKey(void *hash,FC_TxDefRow *txdef,int *errOut);         
    
    int GetEntityListCount();
    FC_TxEntityStat *GetEntity(int row);

    std::string Summary();                                                      // Wallet summary
    
// Internal functions
    
    void Zero();    
    int Destroy();
    
    int AddToUnconfirmedSends(int block,const CWalletTx& tx);
    int FlushUnconfirmedSends(int block);
    int SaveTxFlag(                                                             // Changes tx flag setting (if tx is found))
                   const unsigned char *hash,                                   // Tx ID
                   uint32_t flag,                                               // Flag to set/unset
                   int set_flag);                                               // 1 if set, 0 if unset
    
    int RollBackSubKeys(FC_TxImport *import,int block,FC_TxEntityStat *parent_entity,FC_Buffer *lpSubKeyEntRowBuffer); // Rollback subkeys to specific block

    int LoadUnconfirmedSends(int block,int file_block); 
    
    std::map<uint256,CWalletTx> GetUnconfirmedSends(int block,std::vector<uint256>& unconfirmedSendsHashes);                 // Internal. Retrieves list of unconfirmed txs sent by this wallet for specific block
    void GetSingleInputEntity(const CWalletTx& tx,FC_TxEntity *input_entity);
    int RemoveUnconfirmedSends(int block);              
    int SaveUTXOMap(int import_id,int block);
    int LoadUTXOMap(int import_id,int block);
    int RemoveUTXOMap(int import_id,int block);
    int GetBlock();
    
    void Lock();
    void UnLock();
    
    
    
} FC_WalletTxs;



#endif	/* FLOATCHAIN_WALLETTXS_H */

