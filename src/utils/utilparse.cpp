// Copyright (c) 2014-2017 Coin Sciences Ltd
// FloatChain code distributed under the GPLv3 license, see COPYING file.

#include "utils/utilparse.h"
#include "util.h"

using namespace std;

const unsigned char* GetAddressIDPtr(const CTxDestination& address) 
{
    const CKeyID *lpKeyID=boost::get<CKeyID> (&address);
    const CScriptID *lpScriptID=boost::get<CScriptID> (&address);
    unsigned char *aptr;
    aptr=NULL;
    if(lpKeyID)
    {
        aptr=(unsigned char*)(lpKeyID);
    }
    else
    {
        if(lpScriptID)
        {
            aptr=(unsigned char*)(lpScriptID);
        }                    
    }
    
    return aptr;
}

/* 
 * Parses txout script into asset-quantity buffer
 * Use it only with unspent or not yet created outputs
 */


bool ParseFloatchainTxOutToBuffer(uint256 hash,                                 // IN, tx hash, if !=0 genesis asset reference is retrieved from asset DB
                                  const CTxOut& txout,                          // IN, tx to be parsed
                                  FC_Buffer *amounts,                           // OUT, output amount buffer
                                  FC_Script *lpScript,                          // TMP, temporary script object
                                  int *allowed,                                 // IN/OUT/NULL returns permissions of output address ANDed with input value 
                                  int *required,                                // IN/OUT/NULL returns permission required by this output, adds special rows to output buffer according to input value   
                                  map<uint32_t, uint256>* mapSpecialEntity,
                                  string& strFailReason)                        // OUT error
{
    unsigned char buf[FC_AST_ASSET_FULLREF_BUF_SIZE];
    int err,row,disallow_if_assets_found;
    int64_t quantity,total,last;
    int expected_allowed,expected_required;
    uint32_t type,from,to,timestamp,type_ored;
    bool issue_found;
    uint32_t new_entity_type;
    
    memset(buf,0,FC_AST_ASSET_FULLREF_BUF_SIZE);
    
    expected_allowed=0;
    expected_required=0;
    
    strFailReason="";
    
    if(allowed)
    {
        expected_allowed=*allowed;
        *allowed=FC_PTP_ALL;
    }
    if(required)
    {
        expected_required=*required;
        *required=0;
    }
    if(FC_gState->m_NetworkParams->IsProtocolFloatchain())
    {
        disallow_if_assets_found=0;
        const CScript& script1 = txout.scriptPubKey;        
        CScript::const_iterator pc1 = script1.begin();

        lpScript->Clear();
        lpScript->SetScript((unsigned char*)(&pc1[0]),(size_t)(script1.end()-pc1),FC_SCR_TYPE_SCRIPTPUBKEY);
        if(allowed)                                                             // Checking permissions this output address have
        {
            *allowed=0;
            CTxDestination addressRet;        
            if(ExtractDestinationScriptValid(script1, addressRet))
            {
                const unsigned char *aptr;
                
                aptr=GetAddressIDPtr(addressRet);
                if(aptr)
                {
                    if(expected_allowed & FC_PTP_SEND)
                    {
                        if(FC_gState->m_Permissions->CanSend(NULL,aptr))
                        {
                            if(FC_gState->m_Permissions->CanReceive(NULL,aptr)) 
                            {
                                *allowed |= FC_PTP_SEND;
                            }
                            else
                            {
                                if(FC_gState->m_Features->AnyoneCanReceiveEmpty())                                
                                {
                                    if(txout.nValue == 0)
                                    {
                                        disallow_if_assets_found=1;
                                        *allowed |= FC_PTP_SEND;
                                    }
                                }
                            }
                        }                         
                    }
                    if(expected_allowed & FC_PTP_WRITE)
                    {
                        unsigned char *lpEntity=NULL;
                        if(mapSpecialEntity)
                        {
                            std::map<uint32_t,uint256>::const_iterator it = mapSpecialEntity->find(FC_PTP_WRITE);
                            if (it != mapSpecialEntity->end())
                            {
                                lpEntity=(unsigned char*)(&(it->second));
                            }
                        }

                        if(FC_gState->m_Permissions->CanWrite(lpEntity,aptr))
                        {
                            *allowed |= FC_PTP_WRITE;
                        }                                                 
                    }
                    if(expected_allowed & FC_PTP_ISSUE)
                    {
                        unsigned char *lpEntity=NULL;

                        if(mapSpecialEntity)
                        {
                            std::map<uint32_t,uint256>::const_iterator it = mapSpecialEntity->find(FC_PTP_ISSUE);
                            if (it != mapSpecialEntity->end())
                            {
                                lpEntity=(unsigned char*)(&(it->second));
                            }
                        }

                        if(FC_gState->m_Permissions->CanIssue(lpEntity,aptr))
                        {
                            *allowed |= FC_PTP_ISSUE;
                        }                                                 
                    }
                    if(expected_allowed & FC_PTP_CREATE)
                    {
                        if(FC_gState->m_Permissions->CanCreate(NULL,aptr))
                        {
                            *allowed |= FC_PTP_CREATE;
                        }                         
                    }
                    if(expected_allowed & FC_PTP_ADMIN)
                    {
                        unsigned char *lpEntity=NULL;

                        if(mapSpecialEntity)
                        {
                            std::map<uint32_t,uint256>::const_iterator it = mapSpecialEntity->find(FC_PTP_ADMIN);
                            if (it != mapSpecialEntity->end())
                            {
                                lpEntity=(unsigned char*)(&(it->second));
                            }
                        }

                        if(FC_gState->m_Permissions->CanAdmin(lpEntity,aptr))
                        {
                            *allowed |= FC_PTP_ADMIN;
                        }                                                 
                    }
                    if(expected_allowed & FC_PTP_ACTIVATE)
                    {
                        unsigned char *lpEntity=NULL;

                        if(mapSpecialEntity)
                        {
                            std::map<uint32_t,uint256>::const_iterator it = mapSpecialEntity->find(FC_PTP_ACTIVATE);
                            if (it != mapSpecialEntity->end())
                            {
                                lpEntity=(unsigned char*)(&(it->second));
                            }
                        }
                        
                        if(FC_gState->m_Permissions->CanActivate(lpEntity,aptr))
                        {
                            *allowed |= FC_PTP_ACTIVATE;
                        }                                                 
                    }
                }
            }
        }    
    
        issue_found=false;
        total=0;
        for (int e = 0; e < lpScript->GetNumElements(); e++)
        {
            lpScript->SetElement(e);
            err=lpScript->GetAssetGenesis(&quantity);
            if(err == 0)
            {
                issue_found=true;
                if(quantity+total<0)
                {
                    strFailReason="Invalid asset genesis script";
                    return false;                                        
                }

                total+=quantity;                    
            }            
            else
            {
                if(err != FC_ERR_WRONG_SCRIPT)
                {
                    strFailReason="Invalid asset genesis script";
                    return false;                                    
                }
            }        
        }
                
        if(issue_found)                                                         
        {
            if(hash != 0)
            {
                FC_EntityDetails entity;
                if(FC_gState->m_Assets->FindEntityByTxID(&entity,(unsigned char*)&hash))
                {
                    if(disallow_if_assets_found)
                    {
                        disallow_if_assets_found=0;
                        *allowed -= FC_PTP_SEND;                        
                    }
                    
                    if((FC_gState->m_Features->ShortTxIDInTx() == 0) && (entity.IsUnconfirmedGenesis() != 0) )
                    {
                        if(required)                                            // Unconfirmed genesis in protocol < 10007, cannot be spent
                        {
                            memset(buf,0,FC_AST_ASSET_FULLREF_SIZE);
                            FC_SetABRefType(buf,FC_AST_ASSET_REF_TYPE_GENESIS);
                            FC_SetABQuantity(buf,total);
                            amounts->Add(buf);        
                            *required |= FC_PTP_ISSUE;
                        }
                    }
                    else            
                    {
                        meFCpy(buf,entity.GetFullRef(),FC_AST_ASSET_FULLREF_SIZE);
                        row=amounts->Seek(buf);
                        last=0;
                        if(row >= 0)
                        {
                            last=FC_GetABQuantity(amounts->GetRow(row));
                            total+=last;
                            FC_SetABQuantity(amounts->GetRow(row),total);
                        }
                        else
                        {
                            FC_SetABQuantity(buf,total);
                            amounts->Add(buf);                        
                        }
                        
                        if(required)
                        {
                            if(expected_required == 0)                          
                            {
                                *required |= FC_PTP_ISSUE;
                            }                            
                        }
                    }
                }                
                else                                                            // Asset not found, no error but the caller should check required field
                {
                    if(required)                                                
                    {
                        *required |= FC_PTP_ISSUE;
                    }
                }
            }
            else                                                                // New unconfirmed genesis                                            
            {
                memset(buf,0,FC_AST_ASSET_FULLREF_SIZE);
                FC_SetABRefType(buf,FC_AST_ASSET_REF_TYPE_GENESIS);
                FC_SetABQuantity(buf,total);
                amounts->Add(buf);
                if(required)
                {
                    *required |= FC_PTP_ISSUE;
                }
            }
        }
        
        if(lpScript->IsOpReturnScript() == 0)
        {
            for (int e = 0; e < lpScript->GetNumElements(); e++)                // Parsing asset quantities
            {
                lpScript->SetElement(e);
                err=lpScript->GetAssetQuantities(amounts,FC_SCR_ASSET_SCRIPT_TYPE_TRANSFER);       // Buffer is updated, new rows are added if needed 
                if((err != FC_ERR_NOERROR) && (err != FC_ERR_WRONG_SCRIPT))
                {
                    strFailReason="Invalid asset transfer script";
                    return false;                                
                }
                
                if(disallow_if_assets_found)                                    // Cannot use in coin selection as this is non-empty output without receive permission
                {
                    if(err != FC_ERR_WRONG_SCRIPT)
                    {
                        disallow_if_assets_found=0;
                        *allowed -= FC_PTP_SEND;                        
                    }
                }
                                
                if(hash != 0)                                                   // Follow-ons
                {
                    err=lpScript->GetAssetQuantities(amounts,FC_SCR_ASSET_SCRIPT_TYPE_FOLLOWON);                  
                    if((err != FC_ERR_NOERROR) && (err != FC_ERR_WRONG_SCRIPT))
                    {
                        strFailReason="Invalid asset followon script";
                        return false;                                
                    }
                    if(disallow_if_assets_found)
                    {
                        if(err != FC_ERR_WRONG_SCRIPT)
                        {
                            disallow_if_assets_found=0;
                            *allowed -= FC_PTP_SEND;                        
                        }
                    }
                }
                else
                {
                    uint32_t script_type=0;
                    unsigned char ref[FC_AST_ASSET_FULLREF_SIZE];
                    err=lpScript->GetFullRef(ref,&script_type);

                    switch(script_type)
                    {
                        case FC_SCR_ASSET_SCRIPT_TYPE_FOLLOWON:
                            FC_EntityDetails entity;
                            if(FC_gState->m_Assets->FindEntityByFullRef(&entity,ref))
                            {
                                if(required)
                                {
                                    *required |= FC_PTP_ISSUE;                    
                                }
                                if(mapSpecialEntity)
                                {
                                    std::map<uint32_t,uint256>::const_iterator it = mapSpecialEntity->find(FC_PTP_ISSUE);
                                    if (it == mapSpecialEntity->end())
                                    {
                                        mapSpecialEntity->insert(make_pair(FC_PTP_ISSUE,*(uint256*)(entity.GetTxID())));
                                    }
                                    else
                                    {
                                        if(it->second != *(uint256*)(entity.GetTxID()))
                                        {
                                            strFailReason="Invalid asset follow-on script, multiple assets";
                                            return false;                                                                                                            
                                        }
                                    }
                                }
                            }
                            else
                            {
                                strFailReason="Invalid asset follow-on script, asset not found";
                                return false;                                                                    
                            }

                            break;
                    }
                }

            }    
        }
        else                                                                    // OP_RETURN outputs
        {
            if(required)
            {
                if(hash == 0)
                {
                    if(lpScript->GetNumElements() == 2)                         // Create entity
                    {
                        lpScript->SetElement(0);
                        if(lpScript->GetNewEntityType(&new_entity_type) == 0)    
                        {
                            if(new_entity_type == FC_ENT_TYPE_STREAM)
                            {
                                *required |= FC_PTP_CREATE;                    
                            }
                        }
                    }
                    if(lpScript->GetNumElements() == 3)                         // Publish
                    {
                        unsigned char short_txid[FC_AST_SHORT_TXID_SIZE];
                        lpScript->SetElement(0);
                        if(lpScript->GetEntity(short_txid) == 0)    
                        {
                            FC_EntityDetails entity;
                            if(FC_gState->m_Assets->FindEntityByShortTxID(&entity,short_txid))
                            {
                                if(entity.GetEntityType() == FC_ENT_TYPE_STREAM)
                                {
                                    if(entity.AnyoneCanWrite() == 0)
                                    {
                                        if(mapSpecialEntity)
                                        {
                                            if(required)
                                            {
                                                *required |= FC_PTP_WRITE;                    
                                            }
                                            std::map<uint32_t,uint256>::const_iterator it = mapSpecialEntity->find(FC_PTP_WRITE);
                                            if (it == mapSpecialEntity->end())
                                            {
                                                mapSpecialEntity->insert(make_pair(FC_PTP_WRITE,*(uint256*)(entity.GetTxID())));
                                            }
                                            else
                                            {
                                                if(it->second != *(uint256*)(entity.GetTxID()))
                                                {
                                                    strFailReason="Invalid publish script, multiple streams";
                                                    return false;                                                                                                            
                                                }
                                            }
                                        }                                    
                                    }
                                }
                                else
                                {
                                    if(FC_gState->m_Features->OpDropDetailsScripts() == 0)// May be Follow-on details from v10007
                                    {
                                        strFailReason="Invalid publish script, not stream";
                                        return false;                                                                    
                                    }
                                }                            
                            }                        
                            else
                            {
                                strFailReason="Invalid publish script, stream not found";
                                return false;                                                                    
                            }
                        }
                        else
                        {
                            strFailReason="Invalid publish script";
                            return false;                                                                    
                        }
                    }
                }
            }        
        }
        
        if(required)
        {
            *required |= FC_PTP_SEND;
            
            if(lpScript->IsOpReturnScript() == 0)
            {
                type_ored=0;
                FC_EntityDetails entity;
                uint32_t admin_type;
                entity.Zero();
                for (int e = 0; e < lpScript->GetNumElements(); e++)            // Parsing permissions
                {
                    unsigned char short_txid[FC_AST_SHORT_TXID_SIZE];
                    lpScript->SetElement(e);                    
                    if(lpScript->GetEntity(short_txid) == 0)                    // Entity element
                    {
                        if(FC_gState->m_Assets->FindEntityByShortTxID(&entity,short_txid) == 0)
                        {
                            strFailReason="Entity not found";
                            return false;                                                                    
                        }                        
                    }
                    
                    if(lpScript->GetPermission(&type,&from,&to,&timestamp) == 0)// Permission script found, admin permission needed
                    {
                        if(FC_gState->m_Permissions->IsActivateEnough(type))
                        {
                            admin_type=FC_PTP_ACTIVATE;
                        }
                        else
                        {
                            admin_type=FC_PTP_ADMIN;
                        }                        
                        *required |= admin_type;
                        if( type & (FC_PTP_ADMIN | FC_PTP_MINE) )
                        {
                            if(FC_gState->m_Features->CachedInputScript())
                            {
                                if(FC_gState->m_NetworkParams->GetInt64Param("supportminerprecheck"))                                
                                {
                                    *required |= FC_PTP_CACHED_SCRIPT_REQUIRED;
                                }        
                            }
                        }
                        
                        if(hash == 0)
                        {
                            if(entity.GetEntityType())
                            {
                                if(mapSpecialEntity)
                                {
                                    std::map<uint32_t,uint256>::const_iterator it = mapSpecialEntity->find(admin_type);
                                    if (it == mapSpecialEntity->end())
                                    {
                                        mapSpecialEntity->insert(make_pair(admin_type,*(uint256*)(entity.GetTxID())));
                                    }
                                    else
                                    {
                                        if(it->second != *(uint256*)(entity.GetTxID()))
                                        {
                                            strFailReason="Invalid permission script, multiple entities";
                                            return false;                                                                                                            
                                        }
                                    }
                                }                                    
                            }
                        }
                        type_ored |= type;
                        entity.Zero();
                    }                        
                }

                if(expected_required & FC_PTP_RECEIVE)                          // Checking for dust
                {
                    if( (type_ored == 0) && (lpScript->IsOpReturnScript() == 0) )
                    {
                        if (txout.IsDust(::minRelayTxFee))
                        {
                            strFailReason="Transaction output value too small";
                            return false;                                
                        }            
                    }
                }
            }
            
            if( ( (expected_allowed & FC_PTP_ISSUE) && (*allowed & FC_PTP_ISSUE) ) ||
                ( (expected_required & FC_PTP_ISSUE) && (*required & FC_PTP_ISSUE) && (hash == 0) ) )    
            {
                    memset(buf,0,FC_AST_ASSET_FULLREF_BUF_SIZE);
                    type=FC_PTP_ISSUE | FC_PTP_SEND;
                    quantity=1;
                    FC_PutLE(buf+4,&type,4);
                    FC_SetABRefType(buf,FC_AST_ASSET_REF_TYPE_SPECIAL);
                    FC_SetABQuantity(buf,quantity);
                    if(amounts->Seek(buf) < 0)
                    {
                        amounts->Add(buf);                
                    }
            }

            if( ( (expected_allowed & FC_PTP_CREATE) && (*allowed & FC_PTP_CREATE) ) ||
                ( (expected_required & FC_PTP_CREATE) && (*required & FC_PTP_CREATE) && (hash == 0) ) )    
            {
                    memset(buf,0,FC_AST_ASSET_FULLREF_BUF_SIZE);
                    type=FC_PTP_CREATE | FC_PTP_SEND;
                    quantity=1;
                    FC_PutLE(buf+4,&type,4);
                    FC_SetABRefType(buf,FC_AST_ASSET_REF_TYPE_SPECIAL);
                    FC_SetABQuantity(buf,quantity);
                    if(amounts->Seek(buf) < 0)
                    {
                        amounts->Add(buf);                
                    }
            }

            if( ( (expected_allowed & FC_PTP_ADMIN) && (*allowed & FC_PTP_ADMIN) ) ||
                ( (expected_required & FC_PTP_ADMIN) && (*required & FC_PTP_ADMIN) && (hash == 0) ) )    
            {
                    memset(buf,0,FC_AST_ASSET_FULLREF_BUF_SIZE);
                    type=FC_PTP_ADMIN | FC_PTP_SEND;
                    quantity=1;
                    FC_PutLE(buf+4,&type,4);
                    FC_SetABRefType(buf,FC_AST_ASSET_REF_TYPE_SPECIAL);
                    FC_SetABQuantity(buf,quantity);
                    if(amounts->Seek(buf) < 0)
                    {
                        amounts->Add(buf);                
                    }
            }
            
            if( ( (expected_allowed & FC_PTP_ACTIVATE) && (*allowed & FC_PTP_ACTIVATE) ) ||
                ( (expected_required & FC_PTP_ACTIVATE) && (*required & FC_PTP_ACTIVATE) && (hash == 0) ) )    
            {
                    memset(buf,0,FC_AST_ASSET_FULLREF_BUF_SIZE);
                    type=FC_PTP_ACTIVATE | FC_PTP_SEND;
                    quantity=1;
                    FC_PutLE(buf+4,&type,4);
                    FC_SetABRefType(buf,FC_AST_ASSET_REF_TYPE_SPECIAL);
                    FC_SetABQuantity(buf,quantity);
                    if(amounts->Seek(buf) < 0)
                    {
                        amounts->Add(buf);                
                    }
            }

/*                                                                              // Not required, publish addresses are passed explicitly                
            if( ( (expected_allowed & FC_PTP_WRITE) && (*allowed & FC_PTP_WRITE) ) ||
                ( (expected_required & FC_PTP_WRITE) && (*required & FC_PTP_WRITE) && (hash == 0) ) )    
            {
                    memset(buf,0,FC_AST_ASSET_FULLREF_BUF_SIZE);
                    type=FC_PTP_WRITE | FC_PTP_SEND;
                    quantity=1;
                    FC_PutLE(buf+4,&type,4);
                    FC_PutLE(buf+FC_AST_ASSET_QUANTITY_OFFSET,&quantity,FC_AST_ASSET_QUANTITY_SIZE);
                    if(amounts->Seek(buf) < 0)
                    {
                        amounts->Add(buf,buf+FC_AST_ASSET_QUANTITY_OFFSET);                
                    }
            }
*/
            memset(buf,0,FC_AST_ASSET_FULLREF_BUF_SIZE);
            type=FC_PTP_SEND;
            FC_PutLE(buf+4,&type,4);
            FC_SetABRefType(buf,FC_AST_ASSET_REF_TYPE_SPECIAL);
            quantity=txout.nValue;
            row=amounts->Seek(buf);
            if(row >= 0)
            {
                quantity+=FC_GetABQuantity(amounts->GetRow(row));
                FC_SetABQuantity(amounts->GetRow(row),quantity);
            }
            else
            {
                FC_SetABQuantity(buf,quantity);
                amounts->Add(buf);                            
            }            
        }
        
    }    
    else                                                                        // Protocol != floatchain
    {
        if(allowed)
        {
            *allowed=FC_PTP_SEND;
        }    
        if(required)
        {
            *required |= FC_PTP_SEND;
            if(expected_required & FC_PTP_RECEIVE)                              // Checking for dust
            {
                if (txout.IsDust(::minRelayTxFee))
                {
                    strFailReason="Transaction output value too small";
                    return false;                                
                }            
            }
            memset(buf,0,FC_AST_ASSET_FULLREF_BUF_SIZE);
            type=FC_PTP_SEND;
            FC_PutLE(buf+4,&type,4);
            FC_SetABRefType(buf,FC_AST_ASSET_REF_TYPE_SPECIAL);
            quantity=txout.nValue;
            row=amounts->Seek(buf);
            if(row >= 0)
            {
                quantity+=FC_GetABQuantity(amounts->GetRow(row));
                FC_SetABQuantity(amounts->GetRow(row),quantity);
            }
            else
            {
                FC_SetABQuantity(buf,quantity);
                amounts->Add(buf);                            
            }            
        }        
    }

    return true;
}

bool ParseFloatchainTxOutToBuffer(uint256 hash,                                 // IN, tx hash, if !=0 genesis asset reference is retrieved from asset DB
                                  const CTxOut& txout,                          // IN, tx to be parsed
                                  FC_Buffer *amounts,                           // OUT, output amount buffer
                                  FC_Script *lpScript,                          // TMP, temporary script object
                                  int *allowed,                                 // IN/OUT/NULL returns permissions of output address ANDed with input value 
                                  int *required,                                // IN/OUT/NULL returns permission required by this output, adds special rows to output buffer according to input value   
                                  string& strFailReason)                        // OUT error
{
    return ParseFloatchainTxOutToBuffer(hash,txout,amounts,lpScript,allowed,required,NULL,strFailReason);
}

bool CreateAssetBalanceList(const CTxOut& txout,FC_Buffer *amounts,FC_Script *lpScript,int *required)
{
    string strFailReason;
    
    unsigned char buf[FC_AST_ASSET_FULLREF_BUF_SIZE];
    int err;
    int64_t quantity,total;
    bool issue_found=false;
    uint32_t type,from,to,timestamp;
    
    memset(buf,0,FC_AST_ASSET_FULLREF_BUF_SIZE);
    
    if(required)
    {
        *required=0;
    }
    
    if(FC_gState->m_NetworkParams->IsProtocolFloatchain())
    {
        const CScript& script1 = txout.scriptPubKey;        
        CScript::const_iterator pc1 = script1.begin();

        lpScript->Clear();
        lpScript->SetScript((unsigned char*)(&pc1[0]),(size_t)(script1.end()-pc1),FC_SCR_TYPE_SCRIPTPUBKEY);
            
        total=0;
        for (int e = 0; e < lpScript->GetNumElements(); e++)
        {
            lpScript->SetElement(e);
            err=lpScript->GetAssetGenesis(&quantity);
            if(err == 0)
            {
                issue_found=true;
                if(quantity+total<0)
                {
                    return false;                                        
                }

                total+=quantity;                    
            }            
            else
            {
                if(err != FC_ERR_WRONG_SCRIPT)
                {
                    return false;                                    
                }
            }        
            
            if(required)
            {
                if(lpScript->GetPermission(&type,&from,&to,&timestamp) == 0)    
                {
                    if(FC_gState->m_Permissions->IsActivateEnough(type))
                    {
                        *required |= FC_PTP_ACTIVATE;
                    }
                    else
                    {
                        *required |= FC_PTP_ADMIN;
                    }
                }                        
            }
        }
        
        if(issue_found)                                                         // Checking that genesis was confirmed at least once
        {
            if(required)
            {
                *required |= FC_PTP_ISSUE;
            }
            memset(buf,0,FC_AST_ASSET_FULLREF_SIZE);
            FC_SetABRefType(buf,FC_AST_ASSET_REF_TYPE_GENESIS);
            FC_SetABQuantity(buf,total);
            amounts->Add(buf);
        }

        for (int e = 0; e < lpScript->GetNumElements(); e++)                    // Parsing asset quantities
        {
            lpScript->SetElement(e);
            err=lpScript->GetAssetQuantities(amounts,FC_SCR_ASSET_SCRIPT_TYPE_TRANSFER); // Buffer is updated, new rows are added if needed 
            
            if((err != FC_ERR_NOERROR) && (err != FC_ERR_WRONG_SCRIPT))
            {
                return false;                                
            }

            err=lpScript->GetAssetQuantities(amounts,FC_SCR_ASSET_SCRIPT_TYPE_FOLLOWON);   
            
            if(err == 0)
            {
                if(required)
                {
                    *required |= FC_PTP_ISSUE;
                }                
            }
            if((err != FC_ERR_NOERROR) && (err != FC_ERR_WRONG_SCRIPT))
            {
                return false;                                
            }

        }            
    }    

    return true;

    
}

bool CreateAssetBalanceList(const CTxOut& txout,FC_Buffer *amounts,FC_Script *lpScript)
{
    return CreateAssetBalanceList(txout,amounts,lpScript,NULL);
}

bool FindFollowOnsInScript(const CScript& script1,FC_Buffer *amounts,FC_Script *lpScript)
{
    int err;
    if(FC_gState->m_NetworkParams->IsProtocolFloatchain())
    {
        CScript::const_iterator pc1 = script1.begin();

        lpScript->Clear();
        lpScript->SetScript((unsigned char*)(&pc1[0]),(size_t)(script1.end()-pc1),FC_SCR_TYPE_SCRIPTPUBKEY);
    }    
    
    for (int e = 0; e < lpScript->GetNumElements(); e++)                        // Parsing asset quantities
    {
        lpScript->SetElement(e);
        err=lpScript->GetAssetQuantities(amounts,FC_SCR_ASSET_SCRIPT_TYPE_FOLLOWON);   
        if((err != FC_ERR_NOERROR) && (err != FC_ERR_WRONG_SCRIPT))
        {
            return false;                                
        }
    }            
    return true;
}

void LogAssetTxOut(string message,uint256 hash,int index,unsigned char* assetrefbin,int64_t quantity)
{
    string txid=hash.GetHex();
    
    string assetref="";
    if(assetrefbin)
    {
        if(FC_GetABRefType(assetrefbin) == FC_AST_ASSET_REF_TYPE_SHORT_TXID)
        {
            for(int i=0;i<8;i++)
            {
                assetref += strprintf("%02x",assetrefbin[FC_AST_SHORT_TXID_OFFSET+FC_AST_SHORT_TXID_SIZE-i-1]);
            }
        }
        else
        {
            assetref += itostr((int)FC_GetLE(assetrefbin,4));
            assetref += "-";
            assetref += itostr((int)FC_GetLE(assetrefbin+4,4));
            assetref += "-";
            assetref += itostr((int)FC_GetLE(assetrefbin+8,2));        
        }
    }
    else
    {
        assetref += "0-0-2";
    }
    if(fDebug)LogPrint("FCatxo", "FCatxo: %s: %s-%d %s %ld\n",message.c_str(),txid.c_str(),index,assetref.c_str(),quantity);
}

bool AddressCanReceive(CTxDestination address)
{
    if(FC_gState->m_NetworkParams->IsProtocolFloatchain() == 0)
    {
        return true;
    }
    CKeyID *lpKeyID=boost::get<CKeyID> (&address);
    CScriptID *lpScriptID=boost::get<CScriptID> (&address);
    if((lpKeyID == NULL) && (lpScriptID == NULL))
    {
        LogPrintf("FChn: Invalid address");
        return false;                
    }
    unsigned char* ptr=NULL;
    CBitcoinAddress addressPrint;
    if(lpKeyID != NULL)
    {
        addressPrint=CBitcoinAddress(*lpKeyID);
        ptr=(unsigned char*)(lpKeyID);
    }
    else
    {
        addressPrint=CBitcoinAddress(*lpScriptID);
        ptr=(unsigned char*)(lpScriptID);
    }
    
    if(FC_gState->m_Permissions->CanReceive(NULL,ptr) == 0)
    {
        LogPrintf("FChn: Destination address doesn't have receive permission: %s\n",
                addressPrint.ToString().c_str());
        return false;
    }
    
    return true;
}

