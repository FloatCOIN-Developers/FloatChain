// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin developers
// Original code was distributed under the MIT software license.
// Copyright (c) 2014-2017 Coin Sciences Ltd
// FloatChain code distributed under the GPLv3 license, see COPYING file.

#include "core/main.h"
#include "utils/util.h"
#include "floatchain/floatchain.h"
#include "structs/base58.h"

using namespace std;

string EncodeHexTx(const CTransaction& tx);

uint160 FC_GenesisAdmin(const CTransaction& tx)
{
    uint32_t type,from,to,timestamp;    
    for (unsigned int j = 0; j < tx.vout.size(); j++)
    {
        FC_gState->m_TmpScript->Clear();
        const CScript& script1 = tx.vout[j].scriptPubKey;        
        CScript::const_iterator pc1 = script1.begin();
        CTxDestination addressRet;

        FC_gState->m_TmpScript->SetScript((unsigned char*)(&pc1[0]),(size_t)(script1.end()-pc1),FC_SCR_TYPE_SCRIPTPUBKEY);
        for (int e = 0; e < FC_gState->m_TmpScript->GetNumElements(); e++)
        {
            FC_gState->m_TmpScript->SetElement(e);
            if(FC_gState->m_TmpScript->GetPermission(&type,&from,&to,&timestamp) == 0)
            {
                if(type == FC_PTP_GLOBAL_ALL)
                {
                    CTxDestination addressRet;
                    if(ExtractDestination(script1, addressRet))
                    {
                        CKeyID *lpKeyID=boost::get<CKeyID> (&addressRet);
                        if(lpKeyID)
                        {
                            return *(uint160*)lpKeyID;                                                   
                        }
                    }                                        
                }
            }
        }
    }
    return 0;
}

bool FC_ExtractInputAssetQuantities(const CScript& script1, uint256 hash, string& reason)
{
    int err;
    int64_t quantity;
    CScript::const_iterator pc1 = script1.begin();

    FC_gState->m_TmpScript->Clear();
    FC_gState->m_TmpScript->SetScript((unsigned char*)(&pc1[0]),(size_t)(script1.end()-pc1),FC_SCR_TYPE_SCRIPTPUBKEY);
        
    for (int e = 0; e < FC_gState->m_TmpScript->GetNumElements(); e++)
    {
        FC_gState->m_TmpScript->SetElement(e);
        err=FC_gState->m_TmpScript->GetAssetQuantities(FC_gState->m_TmpAssetsIn,FC_SCR_ASSET_SCRIPT_TYPE_TRANSFER | FC_SCR_ASSET_SCRIPT_TYPE_FOLLOWON);
        if((err != FC_ERR_NOERROR) && (err != FC_ERR_WRONG_SCRIPT))
        {
            reason="Asset transfer script rejected - error in script";
            return false;                                
        }
        err=FC_gState->m_TmpScript->GetAssetGenesis(&quantity);
        if(err == 0)
        {
            FC_EntityDetails entity;
            unsigned char buf_amounts[FC_AST_ASSET_FULLREF_BUF_SIZE];
            memset(buf_amounts,0,FC_AST_ASSET_FULLREF_BUF_SIZE);
            if(FC_gState->m_Assets->FindEntityByTxID(&entity,(unsigned char*)&hash))
            {
                if(FC_gState->m_Features->ShortTxIDInTx() == 0)
                {
                    if(entity.IsUnconfirmedGenesis())             // Not confirmed genesis has -1 in offset field
                    {
                        reason="Asset transfer script rejected - using unconfirmed issue";
                        return false;                                
                    }
                }
                meFCpy(buf_amounts,entity.GetFullRef(),FC_AST_ASSET_FULLREF_SIZE);
                int row=FC_gState->m_TmpAssetsIn->Seek(buf_amounts);
                if(row>=0)
                {
                    int64_t last=FC_GetABQuantity(FC_gState->m_TmpAssetsIn->GetRow(row));
                    quantity+=last;
                    FC_SetABQuantity(FC_gState->m_TmpAssetsIn->GetRow(row),quantity);                        
                }
                else
                {
                    FC_SetABQuantity(buf_amounts,quantity);
                    FC_gState->m_TmpAssetsIn->Add(buf_amounts);
                }
            }                
            else
            {
                reason="Asset transfer script rejected - issue tx not found";
                return false;                                
            }                
        }            
        else
        {
            if(err != FC_ERR_WRONG_SCRIPT)
            {
                reason="Asset transfer script rejected - error in input issue script";
                return false;                                    
            }
        }
    }

    return true;
}

bool FC_ExtractOutputAssetQuantities(string& reason)
{
    int err;
    for (int e = 0; e < FC_gState->m_TmpScript->GetNumElements(); e++)
    {
        FC_gState->m_TmpScript->SetElement(e);
        err=FC_gState->m_TmpScript->GetAssetQuantities(FC_gState->m_TmpAssetsOut,FC_SCR_ASSET_SCRIPT_TYPE_TRANSFER);
        if((err != FC_ERR_NOERROR) && (err != FC_ERR_WRONG_SCRIPT))
        {
            reason="Asset transfer script rejected - error in output transfer script";
            return false;                                
        }
    }

    return true;
}

bool FC_CompareAssetQuantities(string& reason)
{
    unsigned char *ptrIn;
    unsigned char *ptrOut;
    int64_t quantity;
    FC_EntityDetails entity;

    for(int i=0;i<FC_gState->m_TmpAssetsIn->GetCount();i++)
    {
        ptrIn=FC_gState->m_TmpAssetsIn->GetRow(i);
        int row=FC_gState->m_TmpAssetsOut->Seek(ptrIn);
        quantity=FC_GetABQuantity(ptrIn);
        if(quantity>0)
        {
            if(row>=0)
            {
                ptrOut=FC_gState->m_TmpAssetsOut->GetRow(row);       
                if(meFCmp(ptrIn,ptrOut,FC_AST_ASSET_QUANTITY_OFFSET+FC_AST_ASSET_QUANTITY_SIZE))
                {
                    reason="Asset transfer script rejected - mismatch in input/output quantities";
                    return false;                                                                    
                }
            }
            else
            {
                reason="Asset transfer script rejected - mismatch in input/output quantities";
                return false;                                                    
            }
        }
    }
    
    for(int i=0;i<FC_gState->m_TmpAssetsOut->GetCount();i++)
    {
        ptrOut=FC_gState->m_TmpAssetsOut->GetRow(i);
        int row=FC_gState->m_TmpAssetsIn->Seek(ptrOut);
        quantity=FC_GetABQuantity(ptrOut);

        if(FC_gState->m_Features->ShortTxIDInTx())
        {
            if(FC_gState->m_Assets->FindEntityByFullRef(&entity,ptrOut) == 0)
            {
                reason="Asset transfer script rejected - asset not found";
                return false;                                                    
            }                           
        }
        
        if(quantity>0)
        {
            if(row>=0)
            {
                ptrIn=FC_gState->m_TmpAssetsIn->GetRow(row);       
                if(meFCmp(ptrIn,ptrOut,FC_AST_ASSET_QUANTITY_OFFSET+FC_AST_ASSET_QUANTITY_SIZE))
                {
                    reason="Asset transfer script rejected - mismatch in input/output quantities";
                    return false;                                                                    
                }
            }
            else
            {
                reason="Asset transfer script rejected - mismatch in input/output quantities";
                return false;                                                    
            }
        }
    }

    return true;
}

bool AcceptAssetGenesisFromPredefinedIssuers(const CTransaction &tx,
                                            int offset,bool accept,
                                            vector <uint160> vInputDestinations,
                                            vector <int> vInputHashTypes,
                                            vector <txnouttype> vInputScriptTypes,
                                            bool fReturnFalseIfFound,
                                            string& reason,
                                            bool *fFullReplayCheckRequired)
{    
    int update_mempool=0;
    FC_EntityDetails entity;
    FC_EntityDetails this_entity;
    unsigned char details_script[FC_ENT_MAX_SCRIPT_SIZE];
    char asset_name[FC_ENT_MAX_NAME_SIZE+1];
    int multiple;
    int details_script_size;
    int err;
    int64_t quantity,total;
    uint256 txid;
    bool new_issue,follow_on;
    int details_script_type;
    unsigned char *ptrOut;
    vector <uint160> issuers;
    vector <uint32_t> issuer_flags;
    uint32_t flags;
    uint32_t value_offset;
    size_t value_size;
    unsigned char short_txid[FC_AST_SHORT_TXID_SIZE];
    
        
    if(FC_gState->m_NetworkParams->IsProtocolFloatchain() == 0)
    {
        return true;
    }    
    if(tx.IsCoinBase())
    {
        return true;
    }
    
    if(accept)
    {
        update_mempool=1;
    }
    
    total=0;
    
    asset_name[0]=0;
    multiple=1;
    new_issue=false;
    follow_on=false;
    details_script_type=-1;
    FC_gState->m_TmpAssetsOut->Clear();
    
    details_script_size=0;
    for (unsigned int j = 0; j < tx.vout.size(); j++)
    {
        FC_gState->m_TmpScript->Clear();
        
        const CScript& script1 = tx.vout[j].scriptPubKey;        
        CScript::const_iterator pc1 = script1.begin();
        uint32_t new_entity_type;
        int entity_update;
        
        CTxDestination addressRet;
        
        FC_gState->m_TmpScript->SetScript((unsigned char*)(&pc1[0]),(size_t)(script1.end()-pc1),FC_SCR_TYPE_SCRIPTPUBKEY);
        
        if(FC_gState->m_TmpScript->IsOpReturnScript())
        {
            if(FC_gState->m_Features->OpDropDetailsScripts())
            {
                if(FC_gState->m_TmpScript->GetNumElements() == 2 )              // One OP_DROP + OP_RETRUN - new entity
                {
                    FC_gState->m_TmpScript->SetElement(0);
                    err=FC_gState->m_TmpScript->GetNewEntityType(&new_entity_type,&entity_update,details_script,&details_script_size);
                    if(err == 0)                                                // New entity element
                    {
                        if(new_entity_type == FC_ENT_TYPE_ASSET)
                        {
                            if(details_script_type >= 0)
                            {
                                reason="Metadata script rejected - too many new entities";
                                return false;                                    
                            }
                            if(entity_update == 0)
                            {
                                details_script_type=entity_update;
                                *asset_name=0x00;
                                multiple=1;
                                value_offset=FC_FindSpecialParamInDetailsScript(details_script,details_script_size,FC_ENT_SPRM_NAME,&value_size);
                                if(value_offset<(uint32_t)details_script_size)
                                {
                                    meFCpy(asset_name,details_script+value_offset,value_size);
                                    asset_name[value_size]=0x00;
                                }
                                value_offset=FC_FindSpecialParamInDetailsScript(details_script,details_script_size,FC_ENT_SPRM_ASSET_MULTIPLE,&value_size);
                                if(value_offset<(uint32_t)details_script_size)
                                {
                                    multiple=FC_GetLE(details_script+value_offset,value_size);
                                }                            
                            }
                        }
                    }
                    else
                    {
                        if(err != FC_ERR_WRONG_SCRIPT)
                        {
                            reason="Asset details script rejected - error in script";
                            return false;                                    
                        }
                    }
                        
                }
                if(FC_gState->m_TmpScript->GetNumElements())                    // 2 OP_DROPs + OP_RETURN - entity update
                {
                    FC_gState->m_TmpScript->SetElement(1);
                    err=FC_gState->m_TmpScript->GetNewEntityType(&new_entity_type,&entity_update,details_script,&details_script_size);
                    if(err == 0)    // New entity element
                    {
                        if(entity_update == 0)
                        {
                            reason="Metadata script rejected - wrong element, should be entity update";
                            return false;                                    
                        }
                        if(details_script_type >= 0)
                        {
                            reason="Metadata script rejected - too many new new entities/entity updates";
                            return false;                                    
                        }
                        if(new_entity_type == FC_ENT_TYPE_ASSET)
                        {
                            details_script_type=entity_update;
                            FC_gState->m_TmpScript->SetElement(0);
                                                                                // Should be spke
                            if(FC_gState->m_TmpScript->GetEntity(short_txid))   // Entity element
                            {
                                reason="Metadata script rejected - wrong element, should be entityref";
                                return false;                                    
                            }
                            if(FC_gState->m_Assets->FindEntityByShortTxID(&entity,short_txid) == 0)
                            {
                                reason="Metadata script rejected - entity not found";
                                return false;                                    
                            }                                           
                        }
                        else
                        {
                            reason="Metadata script rejected - wrong entity type, should be asset";
                            return false;                                                                
                        }
                    }                              
                }                
            }
            else
            {
                if(details_script_size == 0)
                {
                    int e=FC_gState->m_TmpScript->GetNumElements()-1;
                    if(e == 0)
                    {
                        FC_gState->m_TmpScript->SetElement(e);
                        err=FC_gState->m_TmpScript->GetAssetDetails(asset_name,&multiple,details_script,&details_script_size);
                        if(err)
                        {
                            if(err != FC_ERR_WRONG_SCRIPT)
                            {
                                reason="Asset details script rejected - error in script";
                                return false;                                    
                            }
                            details_script_size=0;
                        }
                    }
                }
            }
        }
        else
        {
            for (int e = 0; e < FC_gState->m_TmpScript->GetNumElements(); e++)
            {
                FC_gState->m_TmpScript->SetElement(e);
                err=FC_gState->m_TmpScript->GetAssetGenesis(&quantity);
                if(err == 0)
                {
                    new_issue=true;
                    if(quantity+total<0)
                    {
                        reason="Asset issue script rejected - overflow";
                        return false;                                        
                    }
                                        
                    total+=quantity;
                    
                    if(!ExtractDestination(script1, addressRet))
                    {
                        reason="Asset issue script rejected - wrong destination type";
                        return false;                
                    }
                    
                    CKeyID *lpKeyID=boost::get<CKeyID> (&addressRet);
                    CScriptID *lpScriptID=boost::get<CScriptID> (&addressRet);
                    if((lpKeyID == NULL) && (lpScriptID == NULL))
                    {
                        reason="Asset issue script rejected - wrong destination type";
                        return false;                
                    }
                    CBitcoinAddress address;
                    if(lpKeyID != NULL)
                    {
                        address=CBitcoinAddress(*lpKeyID);
                    }
                    else
                    {
                        address=CBitcoinAddress(*lpScriptID);
                    }
                    if(update_mempool)
                    {
                        if(fDebug)LogPrint("FChn","Found asset issue script in tx %s for %s - (%ld)\n",
                                tx.GetHash().GetHex().c_str(),
                                address.ToString().c_str(),quantity);                    
                    }
                }            
                else
                {
                    if(err != FC_ERR_WRONG_SCRIPT)
                    {
                        reason="Asset issue script rejected - error in script";
                        return false;                                    
                    }
                    err=FC_gState->m_TmpScript->GetAssetQuantities(FC_gState->m_TmpAssetsOut,FC_SCR_ASSET_SCRIPT_TYPE_FOLLOWON);
                    if((err != FC_ERR_NOERROR) && (err != FC_ERR_WRONG_SCRIPT))
                    {
                        reason="Asset follow-on script rejected - error in follow-on script";
                        return false;                                
                    }
                    if(err == FC_ERR_NOERROR)
                    {
                        if(update_mempool)
                        {
                            if(fDebug)LogPrint("FChn","Found asset follow-on script in tx %s\n",
                                    tx.GetHash().GetHex().c_str());                    
                        }
                    }
                }                
            }        
        }
    }    

    if(details_script_type >= 0)
    {
        if(details_script_type)
        {
            follow_on=true;            
        }
        else
        {
            new_issue=true;
        }
    }
    
    if(FC_gState->m_TmpAssetsOut->GetCount())
    {
        follow_on=true;
    }   
    
    if(follow_on)
    {
        total=0;
        if(FC_gState->m_TmpAssetsOut->GetCount() > 1)
        {
            reason="Asset follow-on script rejected - follow-on for several assets";
            return false;                                                
        }
        if(new_issue)
        {
            reason="Asset follow-on script rejected - follow-on and issue in one transaction";
            return false;                                                            
        }
        ptrOut=NULL;
        if(FC_gState->m_TmpAssetsOut->GetCount() == 0)
        {
            if(FC_gState->m_Assets->FindEntityByShortTxID(&entity,short_txid) == 0)
            {
                reason="Details script rejected - entity not found";
                return false;                                    
            }                                           
        }
        else
        {
            ptrOut=FC_gState->m_TmpAssetsOut->GetRow(0);       
            if(FC_gState->m_Assets->FindEntityByFullRef(&entity,ptrOut) == 0)
            {
                reason="Asset follow-on script rejected - asset not found";
                return false;                                                                        
            }            
        }
        if(FC_gState->m_Features->ShortTxIDInTx() == 0)
        {
            if(entity.IsUnconfirmedGenesis())
            {
                reason="Asset follow-on script rejected - unconfirmed asset";
                return false;                                                                                        
            }
        }
        if(entity.AllowedFollowOns() == 0)
        {
            reason="Asset follow-on script rejected - follow-ons not allowed for this asset";
            return false;                                                                                    
        }
        if(details_script_type >= 0)
        {
            if(meFCmp(entity.GetTxID()+FC_AST_SHORT_TXID_OFFSET,short_txid,FC_AST_SHORT_TXID_SIZE))
            {
                reason="Asset follow-on script rejected - mismatch in follow-on quantity asset and details script";
                return false;                                                                                                    
            }
        }
        if(ptrOut)
        {
            total=FC_GetABQuantity(ptrOut);
            if(total+FC_gState->m_Assets->GetTotalQuantity(&entity) < 0)
            {
                reason="Asset follow-on script rejected - exceeds maximal value for asset";
                return false;                                                                                                                
            }
        }
    }
    
    if(!new_issue && !follow_on)
    {
        return true;
    }
    
    if(fReturnFalseIfFound)
    {
        {
            reason="Asset issue script rejected - not allowed in this transaction, conflicts with other entities";
            return false;                                                                                            
        }
    }
    
    *fFullReplayCheckRequired=true;
    
    issuers.clear();
    if(vInputDestinations.size())
    {
        for (unsigned int i = 0; i < tx.vin.size(); i++)
        {
            if(vInputHashTypes[i] == SIGHASH_ALL)
            {
                if(vInputDestinations[i] != 0)
                {
                    bool can_issue=false;
                    if(new_issue)
                    {
                        if(FC_gState->m_Permissions->CanIssue(NULL,(unsigned char*)&vInputDestinations[i]))
                        {                            
                            can_issue=true;
                        }                            
                    }
                    else
                    {
                        if(FC_gState->m_Permissions->CanIssue(entity.GetTxID(),(unsigned char*)&vInputDestinations[i]))
                        {
                            can_issue=true;
                        }                                                        
                    }                
                    if(can_issue)
                    {
                        issuers.push_back(vInputDestinations[i]);
                        flags=FC_PFL_NONE;
                        if(vInputScriptTypes[i] == TX_SCRIPTHASH)
                        {
                            flags |= FC_PFL_IS_SCRIPTHASH;
                        }
                        issuer_flags.push_back(flags);                        
                    }
                }
            }
        }        
    }
    else
    {        
        for (unsigned int i = 0; i < tx.vin.size(); i++)
        {
            const CScript& script2 = tx.vin[i].scriptSig;        
            CScript::const_iterator pc2 = script2.begin();

            FC_gState->m_TmpScript->Clear();
            FC_gState->m_TmpScript->SetScript((unsigned char*)(&pc2[0]),(size_t)(script2.end()-pc2),FC_SCR_TYPE_SCRIPTSIG);

            if(FC_gState->m_TmpScript->GetNumElements() == 2)
            {
                size_t elem_size;
                const unsigned char *elem;

                elem = FC_gState->m_TmpScript->GetData(0,&elem_size);

                if(elem_size > 1)                                               // If this is multisig with one signature it should be OP_0
                {
                    unsigned char hash_type=elem[elem_size-1] & 0x1f;

                    if(hash_type == SIGHASH_ALL)
                    {
                        elem = FC_gState->m_TmpScript->GetData(1,&elem_size);
                        const unsigned char *pubkey_hash=(unsigned char *)Hash160(elem,elem+elem_size).begin();

                        if(new_issue)
                        {
                            if(FC_gState->m_Permissions->CanIssue(NULL,pubkey_hash))
                            {                            
                                issuers.push_back(Hash160(elem,elem+elem_size));
                            }                            
                        }
                        else
                        {
                            if(FC_gState->m_Permissions->CanIssue(entity.GetTxID(),pubkey_hash))
                            {
                                issuers.push_back(Hash160(elem,elem+elem_size));
                            }                                                        
                        }
                    }
                }
            }
        }            
    }
    
    if(issuers.size() == 0)
    {
        reason="Inputs don't belong to valid issuer";
        return false;
    }                

    err=FC_ERR_NOERROR;
    
    FC_gState->m_TmpScript->Clear();
    FC_gState->m_TmpScript->AddElement();
    unsigned char issuer_buf[24];
    memset(issuer_buf,0,sizeof(issuer_buf));
    flags=FC_PFL_NONE;        
    uint32_t timestamp=0;
    set <uint160> stored_issuers;

    if(new_issue)
    {
        FC_gState->m_Permissions->SetCheckPoint();
        err=FC_ERR_NOERROR;

        txid=tx.GetHash();
        err=FC_gState->m_Permissions->SetPermission(&txid,issuer_buf,FC_PTP_CONNECT,
                (unsigned char*)issuers[0].begin(),0,(uint32_t)(-1),timestamp,flags | FC_PFL_ENTITY_GENESIS ,update_mempool,offset);
    }

    for (unsigned int i = 0; i < issuers.size(); i++)
    {
        if(err == FC_ERR_NOERROR)
        {
            if(stored_issuers.count(issuers[i]) == 0)
            {
                meFCpy(issuer_buf,issuers[i].begin(),sizeof(uint160));
                FC_PutLE(issuer_buf+sizeof(uint160),&issuer_flags[i],4);
                if((int)i < FC_gState->m_Assets->MaxStoredIssuers())
                {
                    FC_gState->m_TmpScript->SetSpecialParamValue(FC_ENT_SPRM_ISSUER,issuer_buf,sizeof(issuer_buf));            
                }
                if(new_issue)
                {
                    err=FC_gState->m_Permissions->SetPermission(&txid,issuer_buf,FC_PTP_ADMIN | FC_PTP_ISSUE,
                            (unsigned char*)issuers[0].begin(),0,(uint32_t)(-1),timestamp,flags | FC_PFL_ENTITY_GENESIS ,update_mempool,offset);
                }
                stored_issuers.insert(issuers[i]);
            }
        }
    }        

    memset(issuer_buf,0,sizeof(issuer_buf));
    FC_gState->m_TmpScript->SetSpecialParamValue(FC_ENT_SPRM_ISSUER,issuer_buf,1);                    
    if(new_issue)
    {
        if((err != FC_ERR_NOERROR) || !accept)
        {
            FC_gState->m_Permissions->RollBackToCheckPoint();            
        }
    }
    if(err)
    {
        reason="Cannot update permission database for issued asset";
        return false;            
    }
    
    const unsigned char *special_script;
    size_t special_script_size=0;
    special_script=FC_gState->m_TmpScript->GetData(0,&special_script_size);
    txid=tx.GetHash();
    if(new_issue)
    {        
        err=FC_gState->m_Assets->InsertAsset(&txid,offset,total,asset_name,multiple,details_script,details_script_size,special_script,special_script_size,update_mempool);                      
    }
    else
    {
        err=FC_gState->m_Assets->InsertAssetFollowOn(&txid,offset,total,details_script,details_script_size,special_script,special_script_size,entity.GetTxID(),update_mempool);
    }
            
    if(err)           
    {
        reason="Asset issue script rejected - could not insert new asset to database";
        if(err == FC_ERR_FOUND)
        {
            reason="Asset issue script rejected - entity with this name/asset-ref/txid already exists";                        
            if(FC_gState->m_Assets->FindEntityByTxID(&entity,(unsigned char*)&txid) == 0)
            {
                if(strlen(asset_name) == 0)
                {
                    FC_gState->m_Assets->FindEntityByName(&entity,asset_name);
                }
            }
            
            if(fDebug)LogPrint("FChn","Asset already exists. TxID: %s, AssetRef: %d-%d-%d, Name: %s\n",
                    tx.GetHash().GetHex().c_str(),
                    FC_gState->m_Assets->m_Block+1,offset,(int)(*((unsigned char*)&txid+31))+256*(int)(*((unsigned char*)&txid+30)),
                    entity.GetName());                                        
        }
        return false;                                            
    }
        
    
    if(update_mempool)
    {
        if(offset>=0)
        {
            if(FC_gState->m_Assets->FindEntityByTxID(&this_entity,(unsigned char*)&txid))
            {
                if(new_issue)
                {
                    if(fDebug)LogPrint("FChn","New asset. TxID: %s, AssetRef: %d-%d-%d, Name: %s\n",
                            tx.GetHash().GetHex().c_str(),
                            FC_gState->m_Assets->m_Block+1,offset,(int)(*((unsigned char*)&txid+0))+256*(int)(*((unsigned char*)&txid+1)),
                            this_entity.GetName());                                        
                }
                else
                {
                    uint256 otxid;
                    meFCpy(&otxid,entity.GetTxID(),32);
                    if(fDebug)LogPrint("FChn","Follow-on issue. TxID: %s,  Original issue txid: %s\n",
                            tx.GetHash().GetHex().c_str(),otxid.GetHex().c_str());
                }
            }
            else
            {
                reason="Asset issue script rejected - could not insert new asset to database";
                return false;                                            
            }
        }
    }
    
    return true;
}


bool AcceptFloatChainTransaction(const CTransaction& tx, 
                                 const CCoinsViewCache &inputs,
                                 int offset,
                                 bool accept,
                                 string& reason,
                                 uint32_t *replay)
{
    bool fScriptHashAllFound;
    bool fSeedNodeInvolved;
    bool fReject;
    bool fShouldHaveDestination;
    bool fRejectIfOpDropOpReturn;
    bool fCheckCachedScript;
    bool fFullReplayCheckRequired;
    bool fAdminMinerGrant;
    int nNewEntityOutput;
    unsigned char details_script[FC_ENT_MAX_SCRIPT_SIZE];
    int details_script_size;
    uint32_t new_entity_type;
    int err;
    
    vector <txnouttype> vInputScriptTypes;
    vector <uint160> vInputDestinations;
    vector <int> vInputHashTypes;
    vector <bool> vInputCanGrantAdminMine;
    vector <bool> vInputHadAdminPermissionBeforeThisTx;
    set <string> vAllowedAdmins;
    set <string> vAllowedActivators;
    
    if(FC_gState->m_NetworkParams->IsProtocolFloatchain() == 0)
    {
        return true;
    }    

    details_script_size=0;
    new_entity_type=FC_ENT_TYPE_NONE;
    
    fCheckCachedScript=false;
    if(FC_gState->m_Features->CachedInputScript())
    {
        if(FC_gState->m_NetworkParams->GetInt64Param("supportminerprecheck"))                                
        {
            fCheckCachedScript=true;
        }        
    }
    
    fFullReplayCheckRequired=false;
    fAdminMinerGrant=false;
    fScriptHashAllFound=false;     
    fRejectIfOpDropOpReturn=false;
    FC_gState->m_TmpAssetsIn->Clear();
    for (unsigned int i = 0; i < tx.vin.size(); i++)                            // Processing input scripts
    {                                                                                                                                                                
        if(tx.IsCoinBase())                                                     
        {                                                                                                                                           
            if(i == 0)                                                          
            {
                if(FC_gState->m_Permissions->m_Block == -1)                     
                {
                    vInputScriptTypes.push_back(TX_PUBKEYHASH);
                    vInputDestinations.push_back(FC_GenesisAdmin(tx));          // Genesis admin is considered to be admin/opener of everything in genesis block        
                    fCheckCachedScript=false;
                }
                else
                {
                    vInputScriptTypes.push_back(TX_NONSTANDARD);
                    vInputDestinations.push_back(0);                                                    
                }
                fScriptHashAllFound=true;                                       // Allows to transfer OP_RETURN metadata in this tx. 
                vInputHashTypes.push_back(SIGHASH_ALL);                         
            }
            else                                                                
            {
                vInputScriptTypes.push_back(TX_NONSTANDARD);
                vInputDestinations.push_back(0);                                
                vInputHashTypes.push_back(SIGHASH_NONE);                                        
            }                
        }
        else
        {
            const CScript& script2 = tx.vin[i].scriptSig;        
            CScript::const_iterator pc2 = script2.begin();
            if(FC_gState->m_Features->FixedIn10007())
            {
                if (!script2.IsPushOnly())
                {
                    reason="sigScript should be push-only";
                    return false;
                }
            }
            
            const COutPoint &prevout = tx.vin[i].prevout;
            const CCoins *coins = inputs.AccessCoins(prevout.hash);
            assert(coins);

            const CScript& script1 = coins->vout[prevout.n].scriptPubKey;        
            CScript::const_iterator pc1 = script1.begin();

            txnouttype typeRet;
            int nRequiredRet;
            vector<CTxDestination> addressRets;
            int op_addr_offset,op_addr_size,is_redeem_script,sighash_type,check_last;

            sighash_type=SIGHASH_NONE;
            if(ExtractDestinations(script1,typeRet,addressRets,nRequiredRet))   // Is Standard transaction
            {
                if (typeRet != TX_NULL_DATA)                                    // Single-address destinations
                {
                    CKeyID *lpKeyID=boost::get<CKeyID> (&addressRets[0]);
                    CScriptID *lpScriptID=boost::get<CScriptID> (&addressRets[0]);
                    if( (lpKeyID == NULL) && (lpScriptID == NULL) )
                    {
                        reason="Internal error: cannot extract address from input scriptPubKey";
                        return false;
                    }
                    else
                    {
                        if(typeRet != TX_MULTISIG)
                        {
                            if(lpKeyID)
                            {
                                vInputDestinations.push_back(*(uint160*)lpKeyID);                               
                            }
                            if(lpScriptID)
                            {
                                vInputDestinations.push_back(*(uint160*)lpScriptID);                               
                            }
                        }
                        else
                        {
                            vInputDestinations.push_back(0);                                   
                        }
                    }
                    
                    check_last=0;
                    if( (typeRet == TX_PUBKEY) || (typeRet == TX_MULTISIG) )
                    {
                        check_last=1;
                    }
                    
                                                                                // Find sighash_type
                    FC_ExtractAddressFromInputScript((unsigned char*)(&pc2[0]),(int)(script2.end()-pc2),&op_addr_offset,&op_addr_size,&is_redeem_script,&sighash_type,check_last);        
                    if(sighash_type == SIGHASH_ALL)
                    {
                        fScriptHashAllFound=true;
                    }
                    if(FC_gState->m_Features->FixedIn10008())
                    {
                        if(sighash_type == SIGHASH_SINGLE)
                        {
                            if(i >= tx.vout.size())
                            {
                                reason="SIGHASH_SINGLE input without matching output";
                                return false;                                
                            }
                        }                    
                    }
                    if(check_last)
                    {
                        fRejectIfOpDropOpReturn=true;                           // pay-to-pubkey and bare multisig  script cannot be considered "publisher" for the stream, because we cannot
                                                                                // extract it from the input script. Though we can still accept this transaction it is rejected
                                                                                // for consistency with principle "each input which signs the stream item must contain a permitted writer"
                    }
                }
                else
                {
                    fRejectIfOpDropOpReturn=true;                               // Null data scripts cannot be used in txs with OP_DROP+OP_RETURN
                                                                                // We should not be there at all as null data scripts cannot be signed generally speaking
                    vInputDestinations.push_back(0);       
                }            
                vInputScriptTypes.push_back(typeRet);
            }
            else
            {
                fRejectIfOpDropOpReturn=true;                                   // Non-standard inputs cannot be used in txs with OP_DROP+OP_RETURN
                                                                                // Otherwise we cannot be sure where are the signatures in input script
                vInputScriptTypes.push_back(TX_NONSTANDARD);
                vInputDestinations.push_back(0);                                
            }            

            vInputHashTypes.push_back(sighash_type);        
            
            if(!FC_ExtractInputAssetQuantities(script1,prevout.hash,reason))    // Filling input asset quantity list
            {
                return false;
            }
        }    
        
        vInputCanGrantAdminMine.push_back(!fCheckCachedScript);            
    }    
        
    fReject=false;
    nNewEntityOutput=-1;
    fSeedNodeInvolved=false;
    fShouldHaveDestination=false;
    fShouldHaveDestination |= (FCP_ANYONE_CAN_RECEIVE == 0);
    fShouldHaveDestination |= (FCP_ALLOW_MULTISIG_OUTPUTS == 0);
    fShouldHaveDestination |= (FCP_ALLOW_P2SH_OUTPUTS == 0);
    
    FC_gState->m_Permissions->SetCheckPoint();
    FC_gState->m_TmpAssetsOut->Clear();
    
                                                                                // Processing output scripts
                                                                                // pass 0 - all permissions, except mine, admin and activate + script type checks
                                                                                // pass 1 - mine permissions (to retrieve cached scripts in pass=0)
                                                                                // pass 2 - admin and activate permissions (to retrive list of admins in pass=0)
                                                                                // pass 3 - everything except permissions - balance, assets, streams
    for(int pass=0;pass<4;pass++)
    {
        for (unsigned int j = 0; j < tx.vout.size(); j++)
        {
            bool fNoDestinationInOutput;
            bool fIsPurePermission;
            bool fAdminRequired;
            bool fCheckAdminList;
            bool fAllValidPublishers;
            bool fScriptParsed;
            unsigned char short_txid[FC_AST_SHORT_TXID_SIZE];
            FC_EntityDetails entity;
            int receive_required;
            uint32_t type,from,to,timestamp,flags,approval;
            unsigned char item_key[FC_ENT_MAX_ITEM_KEY_SIZE];
            int item_key_size,entity_update;
            int cs_offset,cs_new_offset,cs_size,cs_vin;
            unsigned char *cs_script;
            
            const CScript& script1 = tx.vout[j].scriptPubKey;        
            CScript::const_iterator pc1 = script1.begin();


            FC_gState->m_TmpScript->Clear();
            FC_gState->m_TmpScript->SetScript((unsigned char*)(&pc1[0]),(size_t)(script1.end()-pc1),FC_SCR_TYPE_SCRIPTPUBKEY);
                       
            if(FC_gState->m_TmpScript->IsOpReturnScript())                      // OP_RETURN
            {
                if( (pass == 0) && !fScriptHashAllFound)             
                {
                    if(FC_gState->m_Features->FixedIn10007())
                    {
                        if( (vInputHashTypes.size() <= j) || (vInputHashTypes[j] != SIGHASH_SINGLE) )
                        {
                            reason="Output with metadata should be properly signed";
                            fReject=true;
                            goto exitlbl;                                                                    
                        }                        
                    }
                    else
                    {
                        reason="Tx with metadata should have at least one SIGHASH_ALL output";
                        fReject=true;
                        goto exitlbl;                                                                
                    }
                }
                
                if( (pass == 0) && (FC_gState->m_TmpScript->GetNumElements() > 1) ) // We have at least one OP_DROP element
                {
                    if(fRejectIfOpDropOpReturn)                                 // We cannot extract sighash_type properly from the input script 
                                                                                // as we don't know where are the signatures
                    {
                        reason="Non-standard, P2PK or bare multisig inputs cannot be used in this tx";
                        fReject=true;
                        goto exitlbl;                                                                
                    }
                    
                    if(FC_gState->m_TmpScript->IsDirtyOpReturnScript())
                    {
                        reason="Non-standard, Only OP_DROP elements are allowed in metadata outputs with OP_DROP";
                        fReject=true;
                        goto exitlbl;                                                                                        
                    }
                }
                
                if( (pass == 0) && (FC_gState->m_TmpScript->GetNumElements() == 2) ) // One OP_DROP + OP_RETURN - new entity or cached script
                {
                    FC_gState->m_TmpScript->SetElement(0);
                    fScriptParsed=false;
                                                  
                    if(FC_gState->m_Features->CachedInputScript())
                    {
                        cs_offset=0;
                        while( (err=FC_gState->m_TmpScript->GetCachedScript(cs_offset,&cs_new_offset,&cs_vin,&cs_script,&cs_size)) != FC_ERR_WRONG_SCRIPT )
                        {
                            fScriptParsed=true;
                            if(err != FC_ERR_NOERROR)
                            {
                                reason="Metadata script rejected - error in cached script";
                                fReject=true;
                                goto exitlbl;                                                                                                                            
                            }
                            if(cs_offset)
                            {
                                if( cs_vin >= (int)tx.vin.size() )
                                {
                                    reason="Metadata script rejected - invalid input in cached script";
                                    fReject=true;
                                    goto exitlbl;                                                                                                                                                                
                                }
                                const COutPoint &prevout = tx.vin[cs_vin].prevout;
                                const CCoins *coins = inputs.AccessCoins(prevout.hash);
                                
                                const CScript& script3 = coins->vout[prevout.n].scriptPubKey;        
                                CScript::const_iterator pc3 = script3.begin();

                                if(cs_size != (int)script3.size())
                                {
                                    reason="Metadata script rejected - cached script mismatch";
                                    fReject=true;
                                    goto exitlbl;                                                                               
                                }
                                if(meFCmp(cs_script,(unsigned char*)&pc3[0],cs_size))
                                {
                                    reason="Metadata script rejected - cached script mismatch";
                                    fReject=true;
                                    goto exitlbl;                                                                                                                   
                                }
                                if(fCheckCachedScript)
                                {
                                    if(vInputHashTypes[cs_vin] == SIGHASH_ALL)
                                    {
                                        vInputCanGrantAdminMine[cs_vin]=true;
                                    }
                                }
                            }
                            cs_offset=cs_new_offset;
                        }
                    }

                    if(FC_gState->m_Features->OpDropDetailsScripts())
                    {
                        err=FC_gState->m_TmpScript->GetNewEntityType(&new_entity_type,&entity_update,details_script,&details_script_size);
                        if(err == 0)    // New entity element
                        {
                            fScriptParsed=true;
                            if(nNewEntityOutput >= 0)
                            {
                                reason="Metadata script rejected - too many new entities";
                                fReject=true;
                                goto exitlbl;                                                                                            
                            }
                            if(entity_update)
                            {
                                reason="Metadata script rejected - entity update script should be preceded by entityref";
                                fReject=true;
                                goto exitlbl;                                                                                                                            
                            }
                            if(new_entity_type <= FC_gState->m_Assets->MaxEntityType())
                            {
                                if(new_entity_type != FC_ENT_TYPE_ASSET)
                                {
                                    nNewEntityOutput=j;
                                }
                            }
                            else
                            {
                                reason="Metadata script rejected - unsupported new entity type";
                                fReject=true;
                                goto exitlbl;                                                                                                                                                                                            
                            }
                        }   
                        else
                        {
                            if(err != FC_ERR_WRONG_SCRIPT)
                            {
                                reason="Entity details script rejected - error in script";
                                fReject=true;
                                goto exitlbl;                                                                                                                                                        
                            }
                        }
                    }
                    else
                    {
                        if(FC_gState->m_TmpScript->GetNewEntityType(&new_entity_type) == 0)    // New entity element
                        {
                            fScriptParsed=true;
                            if(nNewEntityOutput >= 0)
                            {
                                reason="Metadata script rejected - too many new entities";
                                fReject=true;
                                goto exitlbl;                                                                                            
                            }
                            if(new_entity_type != FC_ENT_TYPE_STREAM)
                            {
                                reason="Metadata script rejected - unsupported new entity type";
                                fReject=true;
                                goto exitlbl;                                                                                                                        
                            }
                            nNewEntityOutput=j;
                                                                                    // Should be SPKc
                            FC_gState->m_TmpScript->SetElement(1);
                            err=FC_gState->m_TmpScript->GetGeneralDetails(details_script,&details_script_size);
                            if(err)
                            {
                                if(err != FC_ERR_WRONG_SCRIPT)
                                {
                                    reason="Entity details script rejected - error in script";
                                    fReject=true;
                                    goto exitlbl;                                                                                                                                                        
                                }
                                details_script_size=0;
                            }
                        }
                    }
                    if(!fScriptParsed)
                    {
                        reason="Metadata script rejected - Unrecognized script, should be new entity";
                        if(FC_gState->m_Features->CachedInputScript())
                        {
                            reason+=" or input script cache";
                        }
                        fReject=true;
                        goto exitlbl;                                                                                                                                        
                    }
                }
                
                if( (pass == 0) && (FC_gState->m_TmpScript->GetNumElements() > 3) ) // More than 2 OP_DROPs
                {
                    reason="Metadata script rejected - too many elements";
                    fReject=true;
                    goto exitlbl;                                                                                                                                        
                }

                if( (pass == 0) && (FC_gState->m_TmpScript->GetNumElements() == 3) ) // 2 OP_DROPs + OP_RETURN - possible upgrade approval
                {
                    if(FC_gState->m_Features->Upgrades())
                    {
                        FC_gState->m_TmpScript->SetElement(1);

                        if(FC_gState->m_TmpScript->GetApproval(&approval,&timestamp) == 0)
                        {
                            if(vInputHadAdminPermissionBeforeThisTx.size() == 0)
                            {
                                for (unsigned int i = 0; i < tx.vin.size(); i++)
                                {
                                    vInputHadAdminPermissionBeforeThisTx.push_back(FC_gState->m_Permissions->CanAdmin(NULL,(unsigned char*)&vInputDestinations[i]) != 0);
                                }                                
                            }
                        }                        
                    }
                }
                
                if( (pass == 3) && (FC_gState->m_TmpScript->GetNumElements() == 3) ) // 2 OP_DROPs + OP_RETURN - item key or entity update or upgrade approval
                {
                    FC_gState->m_TmpScript->SetElement(0);
                                                                                // Should be spke
                    if(FC_gState->m_TmpScript->GetEntity(short_txid))           // Entity element
                    {
                        reason="Metadata script rejected - wrong element, should be entityref";
                        fReject=true;
                        goto exitlbl;                                                                                                                                        
                    }
                    if(FC_gState->m_Assets->FindEntityByShortTxID(&entity,short_txid) == 0)
                    {
                        reason="Metadata script rejected - entity not found";
                        fReject=true;
                        goto exitlbl;                                                                            
                    }               
                    
                    if(FC_gState->m_Features->OpDropDetailsScripts())
                    {
                        if(entity.GetEntityType() == FC_ENT_TYPE_ASSET)
                        {
                            FC_gState->m_TmpScript->SetElement(1);
                            err=FC_gState->m_TmpScript->GetNewEntityType(&new_entity_type,&entity_update,details_script,&details_script_size);
                            if(err == 0)    // New entity element
                            {
                                if(entity_update == 0)
                                {
                                    reason="Metadata script rejected - wrong element, should be entity update";
                                    fReject=true;
                                    goto exitlbl;                                                                                                                                                                                                    
                                }
                                if(new_entity_type != FC_ENT_TYPE_ASSET)
                                {
                                    reason="Metadata script rejected - entity type mismatch in update script";
                                    fReject=true;
                                    goto exitlbl;                                                                                                                                                                                                                                        
                                }                                
                            }          
                            else
                            {
                                reason="Metadata script rejected - wrong element, should be entity update";
                                fReject=true;
                                goto exitlbl;                                                                                                                                                                    
                            }
                        }
                        else                                                    // (pseudo)stream or upgrade
                        {
                            if((FC_gState->m_Features->Upgrades() == 0) || (entity.GetEntityType() != FC_ENT_TYPE_UPGRADE)) // (pseudo)stream
                            {
                                FC_gState->m_TmpScript->SetElement(1);
                                                                                    // Should be spkk
                                if(FC_gState->m_TmpScript->GetItemKey(item_key,&item_key_size))   // Item key
                                {
                                    reason="Metadata script rejected - wrong element, should be item key";
                                    fReject=true;
                                    goto exitlbl;                                                                                                                                        
                                }                                            
                            }
                            else                        
                            {
                                FC_gState->m_TmpScript->SetElement(1);          // Upgrade approval
                                
                                if(FC_gState->m_TmpScript->GetApproval(&approval,&timestamp))
                                {
                                    reason="Metadata script rejected - wrong element, should be upgrade approval";
                                    fReject=true;
                                    goto exitlbl;                                                                                                                                                                            
                                }
                                uint256 upgrade_hash=*(uint256*)(entity.GetTxID());
                                if(approval)
                                {
                                    LogPrintf("Found approval script in tx %s for %s\n",
                                            tx.GetHash().GetHex().c_str(),
                                            upgrade_hash.ToString().c_str());
                                }
                                else
                                {
                                    LogPrintf("Found disapproval script in tx %s for %s\n",
                                            tx.GetHash().GetHex().c_str(),
                                            upgrade_hash.ToString().c_str());                                    
                                }

                                bool fAdminFound=false;
                                for (unsigned int i = 0; i < tx.vin.size(); i++)
                                {
                                    if(vInputHadAdminPermissionBeforeThisTx[i])
                                    {
                                        if( (vInputHashTypes[i] == SIGHASH_ALL) || ( (vInputHashTypes[i] == SIGHASH_SINGLE) && (i == j) ) )
                                        {
                                            if(FC_gState->m_Permissions->SetApproval(entity.GetTxID()+FC_AST_SHORT_TXID_OFFSET,approval,
                                                                                     (unsigned char*)&vInputDestinations[i],entity.UpgradeStartBlock(),timestamp,FC_PFL_NONE,1,offset) == 0)
                                            {
                                                fAdminFound=true;
                                            }                                                                                    
                                        }                                        
                                    }
                                }   
                                if(!fAdminFound)
                                {
                                    reason="Inputs don't belong to valid admin for approval script";
                                    fReject=true;
                                    goto exitlbl;                                                            
                                }                                
                            }
                        }
                    }
                    else
                    {
                        if(entity.GetEntityType() != FC_ENT_TYPE_STREAM)
                        {
                            reason="Metadata script rejected - not stream entity";                            
                            fReject=true;
                            goto exitlbl;                                                                                                                                        
                        }                        
                        FC_gState->m_TmpScript->SetElement(1);
                                                                                // Should be spkk
                        if(FC_gState->m_TmpScript->GetItemKey(item_key,&item_key_size))   // Item key
                        {
                            reason="Metadata script rejected - wrong element, should be item key";
                            fReject=true;
                            goto exitlbl;                                                                                                                                        
                        }                                            
                    }
                    
                    if( (entity.GetEntityType() != FC_ENT_TYPE_ASSET) && (entity.GetEntityType() != FC_ENT_TYPE_UPGRADE) )
                    {
                        fAllValidPublishers=true;
                        if(entity.AnyoneCanWrite() == 0)
                        {
                            if(FC_gState->m_Features->FixedIn10007())
                            {
                                for (unsigned int i = 0; i < tx.vin.size(); i++)
                                {
                                    if( (vInputHashTypes[i] == SIGHASH_ALL) || ( (vInputHashTypes[i] == SIGHASH_SINGLE) && (i == j) ) )
                                    {
                                        if(fAllValidPublishers)
                                        {
                                            if(FC_gState->m_Permissions->CanWrite(entity.GetTxID(),(unsigned char*)&vInputDestinations[i]) == 0)
                                            {
                                                fAllValidPublishers=false;
                                            }
                                        }
                                    }
                                }                                
                            }
                            else
                            {
                                for (unsigned int i = 0; i < tx.vin.size(); i++)
                                {
                                    if(fAllValidPublishers)
                                    {
                                        fAllValidPublishers=false;
                                        if( (vInputHashTypes[i] == SIGHASH_ALL) || ( (vInputHashTypes[i] == SIGHASH_SINGLE) && (i == j) ) )
                                        {
                                                                                // Only in these two cases we can extract address from the input script
                                                                                // When subscribing to the stream we don't have output scripts
                                            if( (vInputScriptTypes[i] == TX_PUBKEYHASH) || (vInputScriptTypes[i] == TX_SCRIPTHASH) )
                                            {
                                                if(FC_gState->m_Permissions->CanWrite(entity.GetTxID(),(unsigned char*)&vInputDestinations[i]))
                                                {
                                                    fAllValidPublishers=true;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        if(!fAllValidPublishers)
                        {
                            reason="Metadata script rejected - Inputs don't belong to valid publisher";
                            fReject=true;
                            goto exitlbl;                                                                                                                                                                
                        }                    
                    }
                }
            }
            else                                                                // Not OP_RETURN
            {
                txnouttype typeRet;
                int nRequiredRet;
                vector<CTxDestination> addressRets;

                fNoDestinationInOutput=false;
                
                if(!ExtractDestinations(script1,typeRet,addressRets,nRequiredRet))
                {
                    fNoDestinationInOutput=true;
                }            

                if( (pass == 0) && fShouldHaveDestination )                     // Some setting in the protocol require address can be extracted
                {
                    if(fNoDestinationInOutput && (FCP_ANYONE_CAN_RECEIVE == 0))
                    {
                        reason="Script rejected - destination required ";
                        fReject=true;
                        goto exitlbl;                    
                    }
                    
                    if((typeRet == TX_MULTISIG) && (FCP_ALLOW_MULTISIG_OUTPUTS == 0))
                    {
                        reason="Script rejected - multisig is not allowed";
                        fReject=true;
                        goto exitlbl;                    
                    }

                    if((typeRet == TX_SCRIPTHASH) && (FCP_ALLOW_P2SH_OUTPUTS == 0))
                    {
                        reason="Script rejected - P2SH is not allowed";
                        fReject=true;
                        goto exitlbl;                    
                    }
                }                
                
                receive_required=0;                                             // Number of required receive permissions
                if( (pass == 3) && fShouldHaveDestination )
                {
                    receive_required=addressRets.size();
                    if(typeRet == TX_MULTISIG)
                    {
                        receive_required-=(pc1[0]-0x50);
                        receive_required+=1;
                        if(receive_required>(int)addressRets.size())
                        {
                            receive_required=addressRets.size();
                        }
                    }
                }
                
                fIsPurePermission=false;
                if(FC_gState->m_TmpScript->GetNumElements())
                {
                    fIsPurePermission=true;
                }
                
                entity.Zero();                                                  // Permission processing    
                for (int e = 0; e < FC_gState->m_TmpScript->GetNumElements(); e++)
                {
                    FC_gState->m_TmpScript->SetElement(e);
                    if(FC_gState->m_TmpScript->GetEntity(short_txid) == 0)      // Entity element
                    {
                        if(entity.GetEntityType())
                        {
                            reason="Script rejected - duplicate entity script";
                            fReject=true;
                            goto exitlbl;                                                
                        }
                        if(FC_gState->m_Assets->FindEntityByShortTxID(&entity,short_txid) == 0)
                        {
                            reason="Script rejected - entity not found";
                            fReject=true;
                            goto exitlbl;                                                                            
                        }                        
                    }
                    else                                                        // Not entity element
                    {   
                        if(FC_gState->m_TmpScript->GetPermission(&type,&from,&to,&timestamp) == 0)// Permission script
                        {
                            if(fNoDestinationInOutput)
                            {
                                reason="Script rejected - wrong destination type in output with permission script";
                                fReject=true;
                                goto exitlbl;                                
                            }
                            fCheckAdminList=false;
                            if(FC_gState->m_Features->CachedInputScript())
                            {
                                fCheckAdminList=true;
                            }
                            else
                            {
                                if(type & ( FC_PTP_ACTIVATE | FC_PTP_ADMIN ))
                                {
                                    fCheckAdminList=true;
                                }
                            }
                            fAdminRequired=false;
                            switch(pass)
                            {
                                case 0:                                         // Not admin or activate
                                    fAdminRequired=fCheckAdminList;                                        
                                    if(FC_gState->m_Features->CachedInputScript())
                                    {
                                        type &= ( FC_PTP_CONNECT | FC_PTP_SEND | FC_PTP_RECEIVE | FC_PTP_WRITE);
                                    }
                                    else
                                    {
                                        type &= ~( FC_PTP_ACTIVATE | FC_PTP_ADMIN);                                        
                                    }
                                    break;
                                case 1:                                         // Admin or activate
                                    if(FC_gState->m_Features->CachedInputScript())
                                    {
                                        type &= ( FC_PTP_CREATE | FC_PTP_ISSUE | FC_PTP_ACTIVATE );
                                    }
                                    else
                                    {
                                        type = 0;                                        
                                    }
                                    break;
                                case 2:                                         // Admin or activate
                                    if(FC_gState->m_Features->CachedInputScript())
                                    {
                                        type &= ( FC_PTP_MINE | FC_PTP_ADMIN );                                        
                                        if(type)
                                        {
                                            fAdminMinerGrant=true;                                            
                                        }
                                    }
                                    else                                        
                                    {
                                        type &= ( FC_PTP_ACTIVATE | FC_PTP_ADMIN );
                                    }
                                    break;
                                case 3:                                         // This pass is not about permissions
                                    type=0;
                                    break;
                            }
                            if(fAdminRequired)                                  // this is done for pass=2, calculating list of valid admins before this tx has effect
                            {
                                for (unsigned int i = 0; i < tx.vin.size(); i++)
                                {
                                    if( (vInputHashTypes[i] == SIGHASH_ALL) || ( (vInputHashTypes[i] == SIGHASH_SINGLE) && (i == j) ) )
                                    {
                                        if(FC_gState->m_Permissions->CanAdmin(entity.GetTxID(),(unsigned char*)&vInputDestinations[i]))
                                        {
                                            vAllowedAdmins.insert(strprintf("%d-%d-%d",i,j,e));
                                        }                                        
                                        if(FC_gState->m_Permissions->CanActivate(entity.GetTxID(),(unsigned char*)&vInputDestinations[i]))
                                        {
                                            vAllowedActivators.insert(strprintf("%d-%d-%d",i,j,e));
                                        }
                                    }
                                }
                            }
                            
                            if(type)
                            {
                                CKeyID *lpKeyID=boost::get<CKeyID> (&addressRets[0]);
                                CScriptID *lpScriptID=boost::get<CScriptID> (&addressRets[0]);
                                                                                // Permissions cannot be granted to nonstandard outputs or bare multisigs
                                if(((lpKeyID == NULL) && (lpScriptID == NULL)) || (addressRets.size() > 1))
                                {
                                    reason="Permission script rejected - wrong destination type";
                                    fReject=true;
                                    goto exitlbl;
                                }
                                CBitcoinAddress address;
                                unsigned char* ptr=NULL;
                                flags=FC_PFL_NONE;
                                if(lpKeyID != NULL)
                                {
                                    address=CBitcoinAddress(*lpKeyID);
                                    ptr=(unsigned char*)(lpKeyID);
                                    if(type & FC_PTP_CONNECT)                   
                                    {
                                        if(FC_gState->m_pSeedNode)
                                        {
                                            CNode* seed_node;
                                            seed_node=(CNode*)(FC_gState->m_pSeedNode);

                                                                                // If connect permission of seed node was involved, we may want to disconnect from it
                                            if(meFCmp(ptr,seed_node->kAddrRemote.begin(),20) == 0)
                                            {
                                                fSeedNodeInvolved=true;
                                            }
                                        }
                                    }
                                }
                                else
                                {
                                    flags=FC_PFL_IS_SCRIPTHASH;
                                    address=CBitcoinAddress(*lpScriptID);
                                    ptr=(unsigned char*)(lpScriptID);
                                }
                                
                                if(fDebug)LogPrint("FChn","Found permission script in tx %s for %s - (%08x: %d - %d)\n",
                                        tx.GetHash().GetHex().c_str(),
                                        address.ToString().c_str(),
                                        type, from, to);

                                bool fAdminFound=false;
                                bool fAdminFoundWithoutCachedScript=false;
                                bool fActivateIsEnough=FC_gState->m_Permissions->IsActivateEnough(type);
                                for (unsigned int i = 0; i < tx.vin.size(); i++)
                                {
                                    if( ( fCheckAdminList && !fActivateIsEnough && (vAllowedAdmins.count(strprintf("%d-%d-%d",i,j,e)) > 0)) ||
                                        ( fCheckAdminList && fActivateIsEnough && (vAllowedActivators.count(strprintf("%d-%d-%d",i,j,e)) > 0)) ||
                                        (!fCheckAdminList && ( (vInputHashTypes[i] == SIGHASH_ALL) || ( (vInputHashTypes[i] == SIGHASH_SINGLE) && (i == j) ) )))    
                                    {                 
                                        if(vInputDestinations[i] != 0)
                                        {
                                            if( ( (type & (FC_PTP_ADMIN | FC_PTP_MINE)) == 0) || vInputCanGrantAdminMine[i] || (entity.GetEntityType() > 0) )
                                            {
                                                fFullReplayCheckRequired=true;
                                                if(FC_gState->m_Permissions->SetPermission(entity.GetTxID(),ptr,type,(unsigned char*)&vInputDestinations[i],from,to,timestamp,flags,1,offset) == 0)
                                                {
                                                    fAdminFound=true;
                                                }
                                            }
                                            else
                                            {
                                                fAdminFoundWithoutCachedScript=true;                                                
                                            }
                                        }
                                    }
                                }
                                if(!fAdminFound)
                                {
                                    reason="Inputs don't belong to valid admin";
                                    if(fAdminFoundWithoutCachedScript)
                                    {
                                        reason="Inputs require scriptPubKey cache to support miner precheck";
                                    }
                                    fReject=true;
                                    goto exitlbl;                                                            
                                }
                            }      
                            entity.Zero();                                      // Entity element in the non-op-return output should be followed by permission element
                                                                                // So only permission can nullify it
                        }
                        else                                                    // Not permission script
                        {
                            if(entity.GetEntityType())                              
                            {
                                reason="Script rejected - entity script should be followed by permission";
                                fReject=true;
                                goto exitlbl;                                                
                            }
                            fIsPurePermission=false;
                        }
                    }
                }                                                               // End of permission script processing
                
                if(entity.GetEntityType())
                {
                    reason="Script rejected - incomplete entity script";
                    fReject=true;
                    goto exitlbl;                                                
                }
                
                if(tx.vout[j].nValue > 0)
                {
                    fIsPurePermission=false;
                }

                if(pass == 3)
                {
                    if(!FC_ExtractOutputAssetQuantities(reason))                // Filling output asset quantity list
                    {
                        fReject=true;
                        goto exitlbl;                                                                        
                    }
                }

                if( (pass == 3) && !fIsPurePermission )                         // Check for dust and receive permission
                {
                    if((offset < 0) && Params().RequireStandard())              // If not in block - part of IsStandard check
                    {
                        if (tx.vout[j].IsDust(::minRelayTxFee))             
                        {
                            if(!tx.IsCoinBase())
                            {
                                reason="Transaction amount too small";
                                fReject=true;
                                goto exitlbl;                                                            
                            }                            
                        }
                    }
                    for(int a=0;a<(int)addressRets.size();a++)
                    {                            
                        CKeyID *lpKeyID=boost::get<CKeyID> (&addressRets[a]);
                        CScriptID *lpScriptID=boost::get<CScriptID> (&addressRets[a]);
                        if((lpKeyID == NULL) && (lpScriptID == NULL))
                        {
                            reason="Script rejected - wrong destination type";
                            fReject=true;
                            goto exitlbl;                                                            
                        }
                        unsigned char* ptr=NULL;
                        if(lpKeyID != NULL)
                        {
                            ptr=(unsigned char*)(lpKeyID);
                        }
                        else
                        {
                            ptr=(unsigned char*)(lpScriptID);
                        }

                        bool fCanReceive=FC_gState->m_Permissions->CanReceive(NULL,ptr);

                        if(tx.IsCoinBase())                                     // Miner can send funds to himself in coinbase even without receive permission
                        {
                            fCanReceive |= FC_gState->m_Permissions->CanMine(NULL,ptr);
                        }
                        if(fCanReceive)                        
                        {
                            receive_required--;
                        }                                    
                    }
                    if(receive_required>0)
                    {
                        if( (tx.vout[j].nValue > 0) || 
                            (FC_gState->m_TmpScript->GetNumElements() > 0) ||
                            (FC_gState->m_Features->AnyoneCanReceiveEmpty() == 0) )
                        {
                            reason="One of the outputs doesn't have receive permission";
                            fReject=true;
                            goto exitlbl;                                                                                
                        }
                    }
                }
            }                                                                   // End of non-OP_RETURN output
        }                                                                       // End of output processing            
    }                                                                           // End of pass loop
    
    if(!FC_CompareAssetQuantities(reason))                                      // Comparing input/output asset quantities
    {
        fReject=true;
        goto exitlbl;                                                                        
    }
        
                                                                                // Checking asset geneses and follow-ons
                                                                                // If new entity is found - asset genesis is forbidden, otherwise they will have the same reference
                                                                                // If we fail here only permission db was updated. it will be rolled back
    if(!AcceptAssetGenesisFromPredefinedIssuers(tx,offset,accept,vInputDestinations,vInputHashTypes,vInputScriptTypes,(nNewEntityOutput >= 0),reason,&fFullReplayCheckRequired))
    {
        fReject=true;
        goto exitlbl;                                                                                
    }
            
    if(nNewEntityOutput >= 0)                                                   // (pseudo)streams and upgrade                                               
    {
        vector <uint160> openers;
        vector <uint32_t> opener_flags;
        unsigned char opener_buf[24];
        
        string entity_type_str;
        uint32_t flags=FC_PFL_NONE;        
        uint32_t timestamp=0;
        set <uint160> stored_openers;
        int update_mempool;
        uint256 txid;
        FC_EntityDetails entity;
        
        fFullReplayCheckRequired=true;
        
        update_mempool=0;
        if(accept)
        {
            update_mempool=1;            
        }
        
        openers.clear();                                                        // List of openers
        opener_flags.clear();
        for (unsigned int i = 0; i < tx.vin.size(); i++)
        {
            if( (vInputHashTypes[i] == SIGHASH_ALL) || ( (vInputHashTypes[i] == SIGHASH_SINGLE) && ((int)i == nNewEntityOutput) ) )
            {
                if(FC_gState->m_Permissions->CanCreate(NULL,(unsigned char*)&vInputDestinations[i]))
                {                            
                    if( (new_entity_type != FC_ENT_TYPE_UPGRADE) || (FC_gState->m_Permissions->CanAdmin(NULL,(unsigned char*)&vInputDestinations[i]) != 0) )
                    {
                        openers.push_back(vInputDestinations[i]);
                        flags=FC_PFL_NONE;
                        if(vInputScriptTypes[i] == TX_SCRIPTHASH)
                        {
                            flags |= FC_PFL_IS_SCRIPTHASH;
                        }
                        opener_flags.push_back(flags);
                    }
                }                            
            }
        }                
        
        if(openers.size() == 0)
        {
            reason="Metadata script rejected - Inputs don't belong to valid creator";
            fReject=true;
            goto exitlbl;                                                                                                                                                                
        }
        
        err=FC_ERR_NOERROR;

        FC_gState->m_TmpScript->Clear();
        FC_gState->m_TmpScript->AddElement();
        txid=tx.GetHash();                                                      // Setting first record in the per-entity permissions list
        
        if(new_entity_type != FC_ENT_TYPE_UPGRADE)
        {
            memset(opener_buf,0,sizeof(opener_buf));
            err=FC_gState->m_Permissions->SetPermission(&txid,opener_buf,FC_PTP_CONNECT,
                    (unsigned char*)openers[0].begin(),0,(uint32_t)(-1),timestamp, FC_PFL_ENTITY_GENESIS ,update_mempool,offset);
        }
        for (unsigned int i = 0; i < openers.size(); i++)
        {
            if(err == FC_ERR_NOERROR)
            {
                if(stored_openers.count(openers[i]) == 0)
                {
                    meFCpy(opener_buf,openers[i].begin(),sizeof(uint160));
                    FC_PutLE(opener_buf+sizeof(uint160),&opener_flags[i],4);
                    if((int)i < FC_gState->m_Assets->MaxStoredIssuers())
                    {
                        FC_gState->m_TmpScript->SetSpecialParamValue(FC_ENT_SPRM_ISSUER,opener_buf,sizeof(opener_buf));            
                    }
                    if(new_entity_type != FC_ENT_TYPE_UPGRADE)
                    {
                                                                                // Granting default permissions to openers
                        err=FC_gState->m_Permissions->SetPermission(&txid,opener_buf,FC_PTP_ADMIN | FC_PTP_ACTIVATE | FC_PTP_WRITE,
                                (unsigned char*)openers[i].begin(),0,(uint32_t)(-1),timestamp,opener_flags[i] | FC_PFL_ENTITY_GENESIS ,update_mempool,offset);
                    }
                    stored_openers.insert(openers[i]);
                }
            }
        }        
        if(err)
        {
            reason=" Cannot update permission database for new entity";
            fReject=true;
            goto exitlbl;                                                                                                                                                                
        }
        
        memset(opener_buf,0,sizeof(opener_buf));                                // Storing opener list in entity metadata
        FC_gState->m_TmpScript->SetSpecialParamValue(FC_ENT_SPRM_ISSUER,opener_buf,1);                    
        
        const unsigned char *special_script;
        size_t special_script_size=0;
        special_script=FC_gState->m_TmpScript->GetData(0,&special_script_size);
        err=FC_gState->m_Assets->InsertEntity(&txid,offset,new_entity_type,details_script,details_script_size,special_script,special_script_size,update_mempool);
        if(err)           
        {
            reason="New entity script rejected - could not insert new entity to database";
            if(err == FC_ERR_ERROR_IN_SCRIPT)
            {
                reason="New entity script rejected - error in script";                        
            }
            if(err == FC_ERR_FOUND)
            {
                reason="New entity script rejected - entity with this name already exists";                        
            }
            fReject=true;
            goto exitlbl;                                                                                                                                                                
        }

        if(update_mempool)
        {
            if(FC_gState->m_Assets->FindEntityByTxID(&entity,(unsigned char*)&txid))
            {
                entity_type_str="stream";
                if(new_entity_type == FC_ENT_TYPE_UPGRADE)
                {
                    entity_type_str="upgrade";
                }
                if(offset>=0)
                {
                    LogPrintf("New %s. TxID: %s, StreamRef: %d-%d-%d, Name: %s\n",
                            entity_type_str.c_str(),tx.GetHash().GetHex().c_str(),
                            FC_gState->m_Assets->m_Block+1,offset,(int)(*((unsigned char*)&txid+0))+256*(int)(*((unsigned char*)&txid+1)),
                            entity.GetName());                                        
                }
                else
                {
                    LogPrintf("New %s. TxID: %s, unconfirmed, Name: %s\n",
                            entity_type_str.c_str(),tx.GetHash().GetHex().c_str(),entity.GetName());                                                            
                }
            }
            else
            {
                reason="New entity script rejected - could not insert new entity to database";
                fReject=true;
                goto exitlbl;                                                                                                                                                                
            }
        }
    }
    
    
exitlbl:
                                    
    if(accept)
    {
        if(fSeedNodeInvolved)                                                   // Checking if we should disconnect from seed node
        {
            CNode* seed_node;
            seed_node=(CNode*)(FC_gState->m_pSeedNode);

            if(!FC_gState->m_Permissions->CanConnect(NULL,seed_node->kAddrRemote.begin()))
            {
                LogPrintf("FChn: Seed node lost connect permission \n");
                FC_gState->m_pSeedNode=NULL;
            }
        }
    }

    if(!accept || fReject)                                                      // Rolling back permission database if we were just checking or error occurred    
    {
        FC_gState->m_Permissions->RollBackToCheckPoint();
    }

    if(replay)
    {
        *replay=0;
        if(fFullReplayCheckRequired)
        {
            *replay |= FC_PPL_REPLAY;
        }
        if(fAdminMinerGrant)
        {
            *replay |= FC_PPL_ADMINMINERGRANT;
        }
    }

    if(fReject)
    {
        if(fDebug)LogPrint("FChn","FChn: Tx rejected: %s\n",EncodeHexTx(tx));
    }

    return !fReject;
}

bool AcceptAdminMinerPermissions(const CTransaction& tx,
                                 int offset,
                                 bool verify_signatures,
                                 string& reason,
                                 uint32_t *result)
{
    vector <txnouttype> vInputScriptTypes;
    vector <uint160> vInputDestinations;
    vector <int> vInputHashTypes;
    vector <bool> vInputCanGrantAdminMine;
    vector <bool> vInputHadAdminPermissionBeforeThisTx;
    vector <CScript> vInputPrevOutputScripts;
    bool fReject;    
    bool fAdminFound;
    int err;

    if(result)
    {
        *result=0;
    }
    
    if(tx.IsCoinBase())
    {
        return true;
    }
    
    for (unsigned int i = 0; i < tx.vin.size(); i++)                            
    {                                                                                                                                                                
        vInputCanGrantAdminMine.push_back(false);
        vInputPrevOutputScripts.push_back(CScript());
        vInputDestinations.push_back(0);
    }
    
    fReject=false;
    fAdminFound=false;
    for (unsigned int j = 0; j < tx.vout.size(); j++)
    {
        int cs_offset,cs_new_offset,cs_size,cs_vin;
        unsigned char *cs_script;
            
        const CScript& script1 = tx.vout[j].scriptPubKey;        
        CScript::const_iterator pc1 = script1.begin();


        FC_gState->m_TmpScript->Clear();
        FC_gState->m_TmpScript->SetScript((unsigned char*)(&pc1[0]),(size_t)(script1.end()-pc1),FC_SCR_TYPE_SCRIPTPUBKEY);
                       
        if(FC_gState->m_TmpScript->IsOpReturnScript())                      
        {                
            if( FC_gState->m_TmpScript->GetNumElements() == 2 ) 
            {
                FC_gState->m_TmpScript->SetElement(0);
                                                  
                if(FC_gState->m_Features->CachedInputScript())
                {
                    cs_offset=0;
                    while( (err=FC_gState->m_TmpScript->GetCachedScript(cs_offset,&cs_new_offset,&cs_vin,&cs_script,&cs_size)) != FC_ERR_WRONG_SCRIPT )
                    {
                        if(err != FC_ERR_NOERROR)
                        {
                            reason="Metadata script rejected - error in cached script";
                            fReject=true;
                            goto exitlbl;                                                                                                                            
                        }
                        if(cs_offset)
                        {
                            if( cs_vin >= (int)tx.vin.size() )
                            {
                                reason="Metadata script rejected - invalid input in cached script";
                                fReject=true;
                                goto exitlbl;                                                                                                                                                                
                            }
                            vInputPrevOutputScripts[cs_vin]=CScript(cs_script,cs_script+cs_size);                            
                            vInputCanGrantAdminMine[cs_vin]=true;
//                            fAdminFound=true;
                        }
                        cs_offset=cs_new_offset;
                    }
                }
            }
        }
    }
/*
    if(!fAdminFound)
    {
        goto exitlbl;                                                                                                                                                                        
    }
    
    fAdminFound=false;
*/    
    for (unsigned int i = 0; i < tx.vin.size(); i++)        
    {                                                                                                                                                                
        if(vInputCanGrantAdminMine[i])
        {
            vInputCanGrantAdminMine[i]=false;
            const CScript& script2 = tx.vin[i].scriptSig;        
            CScript::const_iterator pc2 = script2.begin();
            if(FC_gState->m_Features->FixedIn10007())
            {
                if (!script2.IsPushOnly())
                {
                    reason="sigScript should be push-only";
                    fReject=true;
                    goto exitlbl;                                                                                                                                                                
                }
            }

            const CScript& script1 = vInputPrevOutputScripts[i];        
            CScript::const_iterator pc1 = script1.begin();

            txnouttype typeRet;
            int nRequiredRet;
            vector<CTxDestination> addressRets;
            int op_addr_offset,op_addr_size,is_redeem_script,sighash_type,check_last;

            sighash_type=SIGHASH_NONE;
            if(ExtractDestinations(script1,typeRet,addressRets,nRequiredRet)) 
            {
                if ( (typeRet != TX_NULL_DATA) && (typeRet != TX_MULTISIG) )                                  
                {
                    CKeyID *lpKeyID=boost::get<CKeyID> (&addressRets[0]);
                    CScriptID *lpScriptID=boost::get<CScriptID> (&addressRets[0]);
                    if( (lpKeyID == NULL) && (lpScriptID == NULL) )
                    {
                        fReject=true;
                        goto exitlbl;                                                                                                                                                                
                    }
                    if(lpKeyID)
                    {
                        vInputDestinations[i]=*(uint160*)lpKeyID;                               
                    }
                    if(lpScriptID)
                    {
                        vInputDestinations[i]=*(uint160*)lpScriptID;                               
                    }
                    
                    check_last=0;
                    if( typeRet == TX_PUBKEY )
                    {
                        check_last=1;
                    }

                                                                                // Find sighash_type
                    FC_ExtractAddressFromInputScript((unsigned char*)(&pc2[0]),(int)(script2.end()-pc2),&op_addr_offset,&op_addr_size,&is_redeem_script,&sighash_type,check_last);        
                    if(sighash_type == SIGHASH_ALL)
                    {
                        if(FC_gState->m_Permissions->CanAdmin(NULL,(unsigned char*)&vInputDestinations[i]))
                        {
                            vInputCanGrantAdminMine[i]=true;
                            if(verify_signatures)
                            {
                                vInputCanGrantAdminMine[i]=false;
                                if(VerifyScript(script2, script1, STANDARD_SCRIPT_VERIFY_FLAGS, CachingTransactionSignatureChecker(&tx, i, false)))
                                {
                                    vInputCanGrantAdminMine[i]=true;
                                }
                                else
                                {
                                    reason="Signature verification error";
                                    fReject=true;
                                    goto exitlbl;                                                                                                                                                                                                    
                                }
                            }
/*                            
                            if(vInputCanGrantAdminMine[i])
                            {
                                fAdminFound=true;
                            }
 */ 
                        } 
                    }
                }
            }    
        }        
    }    
/*    
    if(!fAdminFound)
    {
        goto exitlbl;                                                                                                                                                                        
    }
*/    
    for (unsigned int j = 0; j < tx.vout.size(); j++)
    {
        unsigned char short_txid[FC_AST_SHORT_TXID_SIZE];
        FC_EntityDetails entity;
        uint32_t type,from,to,timestamp,flags;
            
        const CScript& script1 = tx.vout[j].scriptPubKey;        
        CScript::const_iterator pc1 = script1.begin();


        FC_gState->m_TmpScript->Clear();
        FC_gState->m_TmpScript->SetScript((unsigned char*)(&pc1[0]),(size_t)(script1.end()-pc1),FC_SCR_TYPE_SCRIPTPUBKEY);
        
        CTxDestination addressRet;

        if(ExtractDestination(script1,addressRet))
        {            
            entity.Zero();                                                  
            for (int e = 0; e < FC_gState->m_TmpScript->GetNumElements(); e++)
            {
                FC_gState->m_TmpScript->SetElement(e);
                if(FC_gState->m_TmpScript->GetEntity(short_txid) == 0)      
                {
                    if(entity.GetEntityType())
                    {
                        reason="Script rejected - duplicate entity script";
                        fReject=true;
                        goto exitlbl;                                                
                    }
                }
                else                                                        
                {   
                    if(FC_gState->m_TmpScript->GetPermission(&type,&from,&to,&timestamp) == 0)
                    {                        
                        type &= ( FC_PTP_MINE | FC_PTP_ADMIN );                                        
                        if(entity.GetEntityType())
                        {
                            type=0;
                        }
                        if(type)
                        {
                            CKeyID *lpKeyID=boost::get<CKeyID> (&addressRet);
                            CScriptID *lpScriptID=boost::get<CScriptID> (&addressRet);
                            if((lpKeyID == NULL) && (lpScriptID == NULL))
                            {
                                reason="Permission script rejected - wrong destination type";
                                fReject=true;
                                goto exitlbl;
                            }
                            unsigned char* ptr=NULL;
                            flags=FC_PFL_NONE;
                            if(lpKeyID != NULL)
                            {
                                ptr=(unsigned char*)(lpKeyID);
                            }
                            else
                            {
                                flags=FC_PFL_IS_SCRIPTHASH;
                                ptr=(unsigned char*)(lpScriptID);
                            }
                            fAdminFound=false;
                            for (unsigned int i = 0; i < tx.vin.size(); i++)
                            {
                                if(vInputCanGrantAdminMine[i])
                                {
                                    if(FC_gState->m_Permissions->SetPermission(entity.GetTxID(),ptr,type,(unsigned char*)&vInputDestinations[i],from,to,timestamp,flags,1,offset) == 0)
                                    {
                                        fAdminFound=true;
                                    }                                
                                }
                            }    
                            if(!fAdminFound)
                            {
                                reason="Inputs don't belong to valid admin";
                                fReject=true;
                                goto exitlbl;                                                            
                            }                                
                            else
                            {
                                if(result)
                                {
                                    *result |= FC_PPL_ADMINMINERGRANT;
                                }                                
                            }
                        }
                        entity.Zero();                                                                                          
                    }
                    else                                                   
                    {
                        if(entity.GetEntityType())                              
                        {
                            reason="Script rejected - entity script should be followed by permission";
                            fReject=true;
                            goto exitlbl;                                                
                        }
                    }
                }
            }                                                                              
        }
    }    
    
exitlbl:        
    
    if(fReject)
    {
        if(fDebug)LogPrint("FChn","FChn: AcceptAdminMinerPermissions: Tx rejected: %s\n",EncodeHexTx(tx));
    }

    return !fReject;
}


/**
 * Used only for protocols < 10006
 */

bool AcceptAssetTransfers(const CTransaction& tx, const CCoinsViewCache &inputs, string& reason)
{
    unsigned char *ptrIn;
    unsigned char *ptrOut;
    int err;
    int64_t quantity;
    
    if(FC_gState->m_NetworkParams->IsProtocolFloatchain() == 0)
    {
        return true;
    }    
    if(tx.IsCoinBase())
    {
        return true;
    }
    
    FC_gState->m_TmpAssetsIn->Clear();
    for (unsigned int i = 0; i < tx.vin.size(); i++)
    {
        const COutPoint &prevout = tx.vin[i].prevout;
        const CCoins *coins = inputs.AccessCoins(prevout.hash);
        assert(coins);

        FC_gState->m_TmpScript->Clear();
        
        const CScript& script1 = coins->vout[prevout.n].scriptPubKey;        
        CScript::const_iterator pc1 = script1.begin();
        
        FC_gState->m_TmpScript->SetScript((unsigned char*)(&pc1[0]),(size_t)(script1.end()-pc1),FC_SCR_TYPE_SCRIPTPUBKEY);
        
        for (int e = 0; e < FC_gState->m_TmpScript->GetNumElements(); e++)
        {
            FC_gState->m_TmpScript->SetElement(e);
            err=FC_gState->m_TmpScript->GetAssetQuantities(FC_gState->m_TmpAssetsIn,FC_SCR_ASSET_SCRIPT_TYPE_TRANSFER | FC_SCR_ASSET_SCRIPT_TYPE_FOLLOWON);
            if((err != FC_ERR_NOERROR) && (err != FC_ERR_WRONG_SCRIPT))
            {
                reason="Asset transfer script rejected - error in script";
                return false;                                
            }
            err=FC_gState->m_TmpScript->GetAssetGenesis(&quantity);
            if(err == 0)
            {
                uint256 hash=prevout.hash;
                FC_EntityDetails entity;
                unsigned char buf_amounts[FC_AST_ASSET_FULLREF_BUF_SIZE];
                memset(buf_amounts,0,FC_AST_ASSET_FULLREF_BUF_SIZE);
                if(FC_gState->m_Assets->FindEntityByTxID(&entity,(unsigned char*)&hash))
                {
                    if(entity.IsUnconfirmedGenesis())         // Not confirmed genesis has -1 in offset field
                    {
                        reason="Asset transfer script rejected - using unconfirmed issue";
                        return false;                                
                    }
                    meFCpy(buf_amounts,entity.GetFullRef(),FC_AST_ASSET_FULLREF_SIZE);
                    int row=FC_gState->m_TmpAssetsIn->Seek(buf_amounts);
                    if(row>=0)
                    {
                        int64_t last=FC_GetABQuantity(FC_gState->m_TmpAssetsIn->GetRow(row));
                        quantity+=last;
                        FC_SetABQuantity(FC_gState->m_TmpAssetsIn->GetRow(row),quantity);                        
                    }
                    else
                    {
                        FC_SetABQuantity(buf_amounts,quantity);
                        FC_gState->m_TmpAssetsIn->Add(buf_amounts);
                    }
                }                
                else
                {
                    reason="Asset transfer script rejected - issue tx not found";
                    return false;                                
                }                
            }            
            else
            {
                if(err != FC_ERR_WRONG_SCRIPT)
                {
                    reason="Asset transfer script rejected - error in input issue script";
                    return false;                                    
                }
            }
        }
    }

    FC_gState->m_TmpAssetsOut->Clear();
    for (unsigned int j = 0; j < tx.vout.size(); j++)
    {
        FC_gState->m_TmpScript->Clear();
        
        const CScript& script1 = tx.vout[j].scriptPubKey;        

        CScript::const_iterator pc1 = script1.begin();
        
        FC_gState->m_TmpScript->SetScript((unsigned char*)(&pc1[0]),(size_t)(script1.end()-pc1),FC_SCR_TYPE_SCRIPTPUBKEY);
        for (int e = 0; e < FC_gState->m_TmpScript->GetNumElements(); e++)
        {
            FC_gState->m_TmpScript->SetElement(e);
            err=FC_gState->m_TmpScript->GetAssetQuantities(FC_gState->m_TmpAssetsOut,FC_SCR_ASSET_SCRIPT_TYPE_TRANSFER);
            if((err != FC_ERR_NOERROR) && (err != FC_ERR_WRONG_SCRIPT))
            {
                reason="Asset transfer script rejected - error in output transfer script";
                return false;                                
            }
            if(err == FC_ERR_NOERROR)
            {
            }
        }
    }
    
    if(fDebug)LogPrint("FChnminor","Found asset transfer script in tx %s, %d assets\n",
            tx.GetHash().GetHex().c_str(),FC_gState->m_TmpAssetsOut->GetCount());                    
    for(int i=0;i<FC_gState->m_TmpAssetsIn->GetCount();i++)
    {
        ptrIn=FC_gState->m_TmpAssetsIn->GetRow(i);
        int row=FC_gState->m_TmpAssetsOut->Seek(ptrIn);
        quantity=FC_GetABQuantity(ptrIn);
        if(quantity>0)
        {
            if(row>=0)
            {
                ptrOut=FC_gState->m_TmpAssetsOut->GetRow(row);       
                if(meFCmp(ptrIn,ptrOut,FC_AST_ASSET_QUANTITY_OFFSET+FC_AST_ASSET_QUANTITY_SIZE))
                {
                    reason="Asset transfer script rejected - mismatch in input/output quantities";
                    return false;                                                                    
                }
            }
            else
            {
                reason="Asset transfer script rejected - mismatch in input/output quantities";
                return false;                                                    
            }
        }
    }
    
    for(int i=0;i<FC_gState->m_TmpAssetsOut->GetCount();i++)
    {
        ptrOut=FC_gState->m_TmpAssetsOut->GetRow(i);
        int row=FC_gState->m_TmpAssetsIn->Seek(ptrOut);
        quantity=FC_GetABQuantity(ptrOut);
        if(quantity>0)
        {
            if(row>=0)
            {
                ptrIn=FC_gState->m_TmpAssetsIn->GetRow(row);       
                if(meFCmp(ptrIn,ptrOut,FC_AST_ASSET_QUANTITY_OFFSET+FC_AST_ASSET_QUANTITY_SIZE))
                {
                    reason="Asset transfer script rejected - mismatch in input/output quantities";
                    return false;                                                                    
                }
            }
            else
            {
                reason="Asset transfer script rejected - mismatch in input/output quantities";
                return false;                                                    
            }
        }
    }
    
    return true;
}

/**
 * Used only for protocols < 10006
 */


bool AcceptAssetGenesis(const CTransaction &tx,int offset,bool accept,string& reason)
{    
    int update_mempool=0;
    FC_EntityDetails entity;
    FC_EntityDetails this_entity;
    unsigned char details_script[FC_ENT_MAX_SCRIPT_SIZE];
    char asset_name[FC_ENT_MAX_NAME_SIZE+1];
    int multiple;
    int details_script_size;
    int err;
    int64_t quantity,total;
    uint256 txid;
    bool new_issue;
    unsigned char *ptrOut;
    vector <uint160> issuers;
    
    
    if(FC_gState->m_NetworkParams->IsProtocolFloatchain() == 0)
    {
        return true;
    }    
    if(tx.IsCoinBase())
    {
        return true;
    }
    
    if(accept)
    {
        update_mempool=1;
    }
    
    total=0;
    
    asset_name[0]=0;
    multiple=1;
    new_issue=false;
    FC_gState->m_TmpAssetsOut->Clear();
    
    details_script_size=0;
    for (unsigned int j = 0; j < tx.vout.size(); j++)
    {
        FC_gState->m_TmpScript->Clear();
        
        const CScript& script1 = tx.vout[j].scriptPubKey;        
        CScript::const_iterator pc1 = script1.begin();
        
        CTxDestination addressRet;
        
        FC_gState->m_TmpScript->SetScript((unsigned char*)(&pc1[0]),(size_t)(script1.end()-pc1),FC_SCR_TYPE_SCRIPTPUBKEY);
        
        if(FC_gState->m_TmpScript->IsOpReturnScript())
        {
            int e=FC_gState->m_TmpScript->GetNumElements()-1;
            if(e != 0)
            {
                reason="Asset details script rejected - error in script, too many script elements";
                return false;                                                    
            }
            {
                FC_gState->m_TmpScript->SetElement(e);
                err=FC_gState->m_TmpScript->GetAssetDetails(asset_name,&multiple,details_script,&details_script_size);
                if(err)
                {
                    if(err != FC_ERR_WRONG_SCRIPT)
                    {
                        reason="Asset details script rejected - error in script";
                        return false;                                    
                    }
                    details_script_size=0;
                }
                
            }
        }
        else
        {
            for (int e = 0; e < FC_gState->m_TmpScript->GetNumElements(); e++)
            {
                FC_gState->m_TmpScript->SetElement(e);
                err=FC_gState->m_TmpScript->GetAssetGenesis(&quantity);
                if(err == 0)
                {
                    new_issue=true;
                    if(quantity+total<0)
                    {
                        reason="Asset issue script rejected - negative total amount";
//                        printf("%s\n",reason.c_str());
                        return false;                                        
                    }
                                        
                    total+=quantity;
                    
                    if(!ExtractDestination(script1, addressRet))
                    {
                        reason="Asset issue script rejected - wrong destination type";
                        return false;                
                    }
                    
                    CKeyID *lpKeyID=boost::get<CKeyID> (&addressRet);
                    CScriptID *lpScriptID=boost::get<CScriptID> (&addressRet);
                    if((lpKeyID == NULL) && (lpScriptID == NULL))
                    {
                        reason="Asset issue script rejected - wrong destination type";
                        return false;                
                    }
                    CBitcoinAddress address;
                    if(lpKeyID != NULL)
                    {
                        address=CBitcoinAddress(*lpKeyID);
                    }
                    else
                    {
                        address=CBitcoinAddress(*lpScriptID);
                    }
                    if(update_mempool)
                    {
                        if(fDebug)LogPrint("FChn","Found asset issue script in tx %s for %s - (%ld)\n",
                                tx.GetHash().GetHex().c_str(),
                                address.ToString().c_str(),quantity);                    
                    }
                }            
                else
                {
                    if(err != FC_ERR_WRONG_SCRIPT)
                    {
                        reason="Asset issue script rejected - error in script";
                        return false;                                    
                    }
                    err=FC_gState->m_TmpScript->GetAssetQuantities(FC_gState->m_TmpAssetsOut,FC_SCR_ASSET_SCRIPT_TYPE_FOLLOWON);
                    if((err != FC_ERR_NOERROR) && (err != FC_ERR_WRONG_SCRIPT))
                    {
                        reason="Asset follow-on script rejected - error in follow-on script";
                        return false;                                
                    }
                    if(err == FC_ERR_NOERROR)
                    {
                        if(update_mempool)
                        {
                            if(fDebug)LogPrint("FChn","Found asset follow-on script in tx %s\n",
                                    tx.GetHash().GetHex().c_str());                    
                        }
                    }
                }                
            }
        
        }
    }    

    if(FC_gState->m_TmpAssetsOut->GetCount())
    {
        if(FC_gState->m_TmpAssetsOut->GetCount() > 1)
        {
            reason="Asset follow-on script rejected - follow-on for several assets";
            return false;                                                
        }
        if(new_issue)
        {
            reason="Asset follow-on script rejected - follow-on and issue in one transaction";
            return false;                                                            
        }
        ptrOut=FC_gState->m_TmpAssetsOut->GetRow(0);       
        if(FC_gState->m_Assets->FindEntityByRef(&entity,ptrOut) == 0)
        {
            reason="Asset follow-on script rejected - asset not found";
            return false;                                                                        
        }
        if(entity.AllowedFollowOns() == 0)
        {
            reason="Asset follow-on script rejected - follow-ons not allowed for this asset";
            return false;                                                                                    
        }
        total=FC_GetABQuantity(ptrOut);
    }
    
    if(!new_issue && (FC_gState->m_TmpAssetsOut->GetCount() == 0))
    {
        return true;
    }
    
    issuers.clear();
    for (unsigned int i = 0; i < tx.vin.size(); i++)
    {
        const CScript& script2 = tx.vin[i].scriptSig;        
        CScript::const_iterator pc2 = script2.begin();

        FC_gState->m_TmpScript->Clear();
        FC_gState->m_TmpScript->SetScript((unsigned char*)(&pc2[0]),(size_t)(script2.end()-pc2),FC_SCR_TYPE_SCRIPTSIG);

        if(FC_gState->m_TmpScript->GetNumElements() == 2)
        {
            size_t elem_size;
            const unsigned char *elem;

            elem = FC_gState->m_TmpScript->GetData(0,&elem_size);

            if(elem_size > 1)                                                   // If this is multisig with one signature it should be OP_0
            {
                unsigned char hash_type=elem[elem_size-1] & 0x1f;

                if(hash_type == SIGHASH_ALL)
                {
                    elem = FC_gState->m_TmpScript->GetData(1,&elem_size);
                    const unsigned char *pubkey_hash=(unsigned char *)Hash160(elem,elem+elem_size).begin();

                    if(new_issue)
                    {
                        if(FC_gState->m_Permissions->CanIssue(NULL,pubkey_hash))
                        {                            
                            issuers.push_back(Hash160(elem,elem+elem_size));
                        }                            
                    }
                    else
                    {
                        if(FC_gState->m_Permissions->CanIssue(entity.GetTxID(),pubkey_hash))
                        {
                            issuers.push_back(Hash160(elem,elem+elem_size));
                        }                                                        
                    }
                }
            }
        }
    }            
    
    if(issuers.size() == 0)
    {
        reason="Inputs don't belong to valid issuer";
        return false;
    }                

    err=FC_ERR_NOERROR;
    
    FC_gState->m_TmpScript->Clear();
    FC_gState->m_TmpScript->AddElement();
    unsigned char issuer_buf[20];
    memset(issuer_buf,0,sizeof(issuer_buf));
    uint32_t flags=FC_PFL_NONE;        
    uint32_t timestamp=0;
    set <uint160> stored_issuers;

    if(new_issue)
    {
        FC_gState->m_Permissions->SetCheckPoint();
        err=FC_ERR_NOERROR;

        txid=tx.GetHash();
        err=FC_gState->m_Permissions->SetPermission(&txid,issuer_buf,FC_PTP_CONNECT,
                (unsigned char*)issuers[0].begin(),0,(uint32_t)(-1),timestamp,flags | FC_PFL_ENTITY_GENESIS ,update_mempool,offset);
    }

    for (unsigned int i = 0; i < issuers.size(); i++)
    {
        if(err == FC_ERR_NOERROR)
        {
            if(stored_issuers.count(issuers[i]) == 0)
            {
                meFCpy(issuer_buf,issuers[i].begin(),sizeof(uint160));
                if((int)i < FC_gState->m_Assets->MaxStoredIssuers())
                {
                    FC_gState->m_TmpScript->SetSpecialParamValue(FC_ENT_SPRM_ISSUER,issuer_buf,sizeof(issuer_buf));            
                }
                if(new_issue)
                {
                    err=FC_gState->m_Permissions->SetPermission(&txid,issuer_buf,FC_PTP_ADMIN | FC_PTP_ISSUE,
                            (unsigned char*)issuers[0].begin(),0,(uint32_t)(-1),timestamp,flags | FC_PFL_ENTITY_GENESIS ,update_mempool,offset);
                }
                stored_issuers.insert(issuers[i]);
            }
        }
    }        

    memset(issuer_buf,0,sizeof(issuer_buf));
    FC_gState->m_TmpScript->SetSpecialParamValue(FC_ENT_SPRM_ISSUER,issuer_buf,1);                    
    if(new_issue)
    {
        if((err != FC_ERR_NOERROR) || !accept)
        {
            FC_gState->m_Permissions->RollBackToCheckPoint();            
        }
    }
    if(err)
    {
        reason="Cannot update permission database for issued asset";
        return false;            
    }
    
    const unsigned char *special_script;
    size_t special_script_size=0;
    special_script=FC_gState->m_TmpScript->GetData(0,&special_script_size);
    txid=tx.GetHash();
    if(new_issue)
    {        
        err=FC_gState->m_Assets->InsertAsset(&txid,offset,total,asset_name,multiple,details_script,details_script_size,special_script,special_script_size,update_mempool);                      
    }
    else
    {
        err=FC_gState->m_Assets->InsertAssetFollowOn(&txid,offset,total,details_script,details_script_size,special_script,special_script_size,entity.GetTxID(),update_mempool);
    }
            
    if(err)           
    {
        reason="Asset issue script rejected - could not insert new asset to database";
        if(err == FC_ERR_FOUND)
        {
            reason="Asset issue script rejected - asset with such name/asset-ref/txid already exists";                        
            if(FC_gState->m_Assets->FindEntityByTxID(&entity,(unsigned char*)&txid) == 0)
            {
                if(strlen(asset_name) == 0)
                {
                    FC_gState->m_Assets->FindEntityByName(&entity,asset_name);
                }
            }
            
            if(fDebug)LogPrint("FChn","Asset already exists. TxID: %s, AssetRef: %d-%d-%d, Name: %s\n",
                    tx.GetHash().GetHex().c_str(),
                    FC_gState->m_Assets->m_Block+1,offset,(int)(*((unsigned char*)&txid+31))+256*(int)(*((unsigned char*)&txid+30)),
                    entity.GetName());                                        
        }
        return false;                                            
    }
        
    
    if(update_mempool)
    {
        if(offset>=0)
        {
            if(FC_gState->m_Assets->FindEntityByTxID(&this_entity,(unsigned char*)&txid))
            {
                if(new_issue)
                {
                    if(fDebug)LogPrint("FChn","New asset. TxID: %s, AssetRef: %d-%d-%d, Name: %s\n",
                            tx.GetHash().GetHex().c_str(),
                            FC_gState->m_Assets->m_Block+1,offset,(int)(*((unsigned char*)&txid+0))+256*(int)(*((unsigned char*)&txid+1)),
                            this_entity.GetName());                                        
                }
                else
                {
                    uint256 otxid;
                    meFCpy(&otxid,entity.GetTxID(),32);
                    if(fDebug)LogPrint("FChn","Follow-on issue. TxID: %s,  Original issue txid: %s\n",
                            tx.GetHash().GetHex().c_str(),otxid.GetHex().c_str());
                }
            }
            else
            {
                reason="Asset issue script rejected - could not insert new asset to database";
                return false;                                            
            }
        }
    }
    
    return true;
}

/**
 * Used only for protocols < 10006
 */

bool AcceptPermissionsAndCheckForDust(const CTransaction &tx,bool accept,string& reason)
{    
    uint32_t type,from,to,timestamp,flags;
    int receive_required;
    bool is_op_return;
    bool is_pure_permission;
    bool seed_node_involved;
    bool reject;
    bool sighashall_found;
    int pass;
    FC_Script *lpInputScript=NULL;    
    
    if(FC_gState->m_NetworkParams->IsProtocolFloatchain() == 0)
    {
        return true;
    }    
    
    sighashall_found=false;
    
    std::vector <int> admin_outputs;
    admin_outputs.resize(tx.vin.size());
    for (unsigned int i = 0; i < tx.vin.size(); i++)
    {
        const CScript& script2 = tx.vin[i].scriptSig;        
        CScript::const_iterator pc2 = script2.begin();

        FC_gState->m_TmpScript->Clear();
        FC_gState->m_TmpScript->SetScript((unsigned char*)(&pc2[0]),(size_t)(script2.end()-pc2),FC_SCR_TYPE_SCRIPTSIG);

        admin_outputs[i]=tx.vout.size();
        if(tx.IsCoinBase())
        {
            admin_outputs[i]=-1;
        }
        else
        {
            bool is_pay_to_pubkeyhash=false;
            if(FC_gState->m_TmpScript->GetNumElements() == 2)
            {
                size_t elem_size;
                const unsigned char *elem;
                unsigned char hash_type;

                elem = FC_gState->m_TmpScript->GetData(0,&elem_size);
                
                if(elem_size > 1)
                {
                    is_pay_to_pubkeyhash=true;
                    hash_type=elem[elem_size-1] & 0x1f;

                    if(hash_type == SIGHASH_ALL)
                    {
                        admin_outputs[i]=-1;
                        sighashall_found=true;
                    }
                    else
                    {
                        if(hash_type == SIGHASH_SINGLE)
                        {
                            admin_outputs[i]=i;
                        }                    
                        else
                        {                        
                            reason="Permission script rejected - invalid signature hash type";
    //                        printf("%s\n",reason.c_str());
                            return false;
                        }
                    }                
                }
            }
            if(!is_pay_to_pubkeyhash)
            {
                int first_sig=1;
                int this_sig;
                if(FC_gState->m_TmpScript->GetNumElements() == 1)              // pay to pubkey
                {
                    first_sig=0;
                }
                this_sig=first_sig;
                while(this_sig<FC_gState->m_TmpScript->GetNumElements())
                {
                    size_t elem_size;
                    const unsigned char *elem;
                    unsigned char hash_type;

                    elem = FC_gState->m_TmpScript->GetData(this_sig,&elem_size);
                    if(elem_size > 1)
                    {
                        hash_type=elem[elem_size-1] & 0x1f;

                        if(hash_type == SIGHASH_ALL)
                        {
                            sighashall_found=true;
                        }                    
                        this_sig++;
                    }
                    else                                                        // First element of redeemScript
                    {
                        this_sig=FC_gState->m_TmpScript->GetNumElements();
                    }
                }
            }
        }
    }            
    
    reject=false;
    seed_node_involved=false;
    FC_gState->m_Permissions->SetCheckPoint();
    
    lpInputScript=new FC_Script;
    
    for(pass=0;pass<3;pass++)
    {
        for (unsigned int j = 0; j < tx.vout.size(); j++)
        {
            FC_gState->m_TmpScript->Clear();

            const CScript& script1 = tx.vout[j].scriptPubKey;        
            CScript::const_iterator pc1 = script1.begin();

            CTxDestination addressRet;

            FC_gState->m_TmpScript->SetScript((unsigned char*)(&pc1[0]),(size_t)(script1.end()-pc1),FC_SCR_TYPE_SCRIPTPUBKEY);

            is_op_return=false;
            if(FC_gState->m_TmpScript->IsOpReturnScript())
            {
                is_op_return=true;
                if(!sighashall_found)
                {
                    if(!tx.IsCoinBase())
                    {
                        reason="Tx with metadata should have at least one SIGHASH_ALL output";
                        reject=true;
                        goto exitlbl;                    
                    }
                }
            }
            else
            {
                txnouttype typeRet;
                int nRequiredRet;
                vector<CTxDestination> addressRets;

                if(!ExtractDestinations(script1,typeRet,addressRets,nRequiredRet))
                {
                    reason="Script rejected - wrong destination type";
                    reject=true;
                    goto exitlbl;
                }            

                if((typeRet == TX_MULTISIG) && (FCP_ALLOW_MULTISIG_OUTPUTS == 0))
                {
                    reason="Script rejected - multisig is not allowed";
                    reject=true;
                    goto exitlbl;                    
                }
                
                if((typeRet == TX_SCRIPTHASH) && (FCP_ALLOW_P2SH_OUTPUTS == 0))
                {
                    reason="Script rejected - P2SH is not allowed";
                    reject=true;
                    goto exitlbl;                    
                }
                
                receive_required=addressRets.size();
                if(typeRet == TX_MULTISIG)
                {
                    receive_required-=(pc1[0]-0x50);
                    receive_required+=1;
                    if(receive_required>(int)addressRets.size())
                    {
                        receive_required=addressRets.size();
                    }
                }

                is_pure_permission=false;
                if(FC_gState->m_TmpScript->GetNumElements())
                {
                    is_pure_permission=true;
                }
                for (int e = 0; e < FC_gState->m_TmpScript->GetNumElements(); e++)
                {
                    FC_gState->m_TmpScript->SetElement(e);
                    if(FC_gState->m_TmpScript->GetPermission(&type,&from,&to,&timestamp) == 0)
                    {
                        switch(pass)
                        {
                            case 0:
                                type &= ~( FC_PTP_ACTIVATE | FC_PTP_ADMIN );
                                break;
                            case 1:
                                type &= ( FC_PTP_ACTIVATE | FC_PTP_ADMIN );
                                break;
                            case 2:
                                type=0;
                                break;
                        }
                        if(type)
                        {
                            CKeyID *lpKeyID=boost::get<CKeyID> (&addressRets[0]);
                            CScriptID *lpScriptID=boost::get<CScriptID> (&addressRets[0]);
                            if(((lpKeyID == NULL) && (lpScriptID == NULL)) || (addressRets.size() > 1))
                            {
                                reason="Permission script rejected - wrong destination type";
                                reject=true;
                                goto exitlbl;
                            }
                            if(lpScriptID)
                            {
                                if(type != FC_PTP_RECEIVE)
                                {
                                    reason="Permission script rejected - only receive permission can be set for P2SH";
                                    reject=true;
                                    goto exitlbl;                            
                                }
                            }

                            CBitcoinAddress address;
                            unsigned char* ptr=NULL;
                            flags=FC_PFL_NONE;
                            if(lpKeyID != NULL)
                            {
                                address=CBitcoinAddress(*lpKeyID);
                                ptr=(unsigned char*)(lpKeyID);
                                if(type & FC_PTP_CONNECT)
                                {
                                    if(FC_gState->m_pSeedNode)
                                    {
                                        CNode* seed_node;
                                        seed_node=(CNode*)(FC_gState->m_pSeedNode);
                                        
                                        if(meFCmp(ptr,seed_node->kAddrRemote.begin(),20) == 0)
                                        {
                                            seed_node_involved=true;
                                        }
                                    }
                                }
                            }
                            else
                            {
                                flags=FC_PFL_IS_SCRIPTHASH;
                                address=CBitcoinAddress(*lpScriptID);
                                ptr=(unsigned char*)(lpScriptID);
                            }
                            if(fDebug)LogPrint("FChn","Found permission script in tx %s for %s - (%08x: %d - %d)\n",
                                    tx.GetHash().GetHex().c_str(),
                                    address.ToString().c_str(),
                                    type, from, to);

                            bool admin_found=false;
                            for (unsigned int i = 0; i < tx.vin.size(); i++)
                            {
                                if((admin_outputs[i] < 0) || (admin_outputs[i] == (int)j))
                                {
                                    const CScript& script2 = tx.vin[i].scriptSig;        
                                    CScript::const_iterator pc2 = script2.begin();

                                    lpInputScript->Clear();
                                    lpInputScript->SetScript((unsigned char*)(&pc2[0]),(size_t)(script2.end()-pc2),FC_SCR_TYPE_SCRIPTSIG);

                                    if(lpInputScript->GetNumElements() > 1)
                                    {
                                        size_t elem_size;
                                        const unsigned char *elem;

                                        elem = lpInputScript->GetData(0,&elem_size);
                                        if(elem_size > 1)                       // it is not multisig with one signature
                                        {
                                            elem = lpInputScript->GetData(1,&elem_size);
                                            const unsigned char *pubkey_hash=(unsigned char *)Hash160(elem,elem+elem_size).begin();
                                            if(FC_gState->m_Permissions->SetPermission(NULL,ptr,type,pubkey_hash,from,to,timestamp,flags,1,-1) == 0)
                                            {
                                                admin_found=true;
                                            }
                                        }
                                    }                                    
                                }
                            }            
                            if(!admin_found)
                            {
                                reason="Inputs don't belong to valid admin";
                                reject=true;
                                goto exitlbl;                                                            
                            }
                        }
                    }            
                    else
                    {
                        is_pure_permission=false;
                    }
                }

                if(tx.vout[j].nValue > 0)
                {
                    is_pure_permission=false;
                }

                
                if( (pass == 2) && !is_op_return && !is_pure_permission )
                {
                    if (tx.vout[j].IsDust(::minRelayTxFee))             
                    {
                        if(!tx.IsCoinBase())
                        {
                            reason="Transaction amount too small";
                            reject=true;
                            goto exitlbl;                                                            
                        }                            
                    }

                    for(int a=0;a<(int)addressRets.size();a++)
                    {                            
                        CKeyID *lpKeyID=boost::get<CKeyID> (&addressRets[a]);
                        CScriptID *lpScriptID=boost::get<CScriptID> (&addressRets[a]);
                        if((lpKeyID == NULL) && (lpScriptID == NULL))
                        {
                            reason="Script rejected - wrong destination type";
//                            printf("%s\n",reason.c_str());
                            reject=true;
                            goto exitlbl;                                                            
                        }
                        unsigned char* ptr=NULL;
                        if(lpKeyID != NULL)
                        {
                            ptr=(unsigned char*)(lpKeyID);
                        }
                        else
                        {
                            ptr=(unsigned char*)(lpScriptID);
                        }

                        bool can_receive=FC_gState->m_Permissions->CanReceive(NULL,ptr);

                        if(tx.IsCoinBase())
                        {
                            can_receive |= FC_gState->m_Permissions->CanMine(NULL,ptr);
                        }

                        if(can_receive)                        
                        {
                            receive_required--;
                        }                                    
                    }

                    if(receive_required>0)
                    {
                        reason="One of the outputs doesn't have receive permission";
                        reject=true;
                        goto exitlbl;                                                                                
                    }
                }            
            }
        }
    }    

exitlbl:    

    if(accept)
    {
        if(seed_node_involved)
        {
            CNode* seed_node;
            seed_node=(CNode*)(FC_gState->m_pSeedNode);

            if(!FC_gState->m_Permissions->CanConnect(NULL,seed_node->kAddrRemote.begin()))
            {
                LogPrintf("FChn: Seed node lost connect permission \n");
                FC_gState->m_pSeedNode=NULL;
            }
        }
    }

    if(!accept || reject)
    {
        FC_gState->m_Permissions->RollBackToCheckPoint();
    }

    if(lpInputScript)
    {
        delete lpInputScript;
    }

    return !reject;
}


