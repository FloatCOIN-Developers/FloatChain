// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin developers
// Original code was distributed under the MIT software license.
// Copyright (c) 2014-2017 Coin Sciences Ltd
// FloatChain code distributed under the GPLv3 license, see COPYING file.

#include "core/init.h"
#include "core/main.h"
#include "utils/util.h"
#include "wallet/wallet.h"
#include "floatchain/floatchain.h"
#include "keys/pubkey.h"
#include "keys/key.h"
#include "structs/base58.h"
#include "net/net.h"

using namespace std;

bool FloatchainNode_CanConnect(CNode *pnode)
{    
    bool ret=true;
    
    if(!FC_gState->m_Permissions->CanConnect(NULL,pnode->kAddrRemote.begin()))
    {
        if(pnode->fInbound)
        {
            ret=false;
        }
        if(FC_gState->m_pSeedNode != (void*)pnode)
        {
            ret=false;
        }
    }
    
    pnode->fCanConnectRemote=ret;        
    
    return ret;
}

bool FloatchainNode_DisconnectRemote(CNode *pnode)
{
    if(pnode->fDisconnect)
    {
        return true;
    }
    
    if((FC_gState->m_NetworkParams->m_Status == FC_PRM_STATUS_VALID) && pnode->fSuccessfullyConnected && pnode->fParameterSetVerified)
    {
        return !FloatchainNode_CanConnect(pnode);
    }
    
    return false;
}

bool FloatchainNode_DisconnectLocal(CNode *pnode)
{
    if(pnode->fDisconnect)
    {
        return true;
    }
        
    bool ret=true;

    if((FC_gState->m_NetworkParams->m_Status == FC_PRM_STATUS_VALID) && pnode->fSuccessfullyConnected && pnode->fParameterSetVerified)
    {
        if(!FC_gState->m_Permissions->CanConnect(NULL,pnode->kAddrLocal.begin()))
        {
            if(pnode->fInbound)
            {
                ret=false;
            }
            if(FC_gState->m_pSeedNode != (void*)pnode)
            {
                ret=false;
            }            
        }
    }    
    
    pnode->fCanConnectLocal=ret;
    
    return !ret;
}
    

bool FloatchainNode_RespondToGetData(CNode *pnode)
{
    return !(pnode->fDisconnect) & FC_gState->m_Permissions->CanConnect(NULL,pnode->kAddrRemote.begin());    
}

bool FloatchainNode_SendInv(CNode *pnode)
{
    return !(pnode->fDisconnect) & FC_gState->m_Permissions->CanConnect(NULL,pnode->kAddrRemote.begin());    
}

bool FloatchainNode_AcceptData(CNode *pnode)
{
    if(FC_gState->m_pSeedNode == (void*)pnode)
    {
        return true;
    }
    
    return !FloatchainNode_DisconnectRemote(pnode);
}

bool FloatchainNode_IgnoreIncoming(CNode *pnode)
{
    if(FC_gState->m_NodePausedState & FC_NPS_INCOMING)
    {
        return true;
    }
    return false;
}

bool FloatchainNode_IsLocal(CNode *pnode)
{
    return (IsLocal(pnode->addr) || pnode->addr.IsRFC1918()) && (pnode->addr.GetPort() == GetListenPort());
}


bool VerifyFloatchainVerackHash(string sParameterSetHash,uint64_t nNonce)
{
    unsigned char* stored_hash=(unsigned char*)FC_gState->m_NetworkParams->GetParam("chainparamshash",NULL);
    CHashWriter ss(SER_GETHASH, 0);
    ss << vector<unsigned char>(stored_hash, stored_hash+32);
    ss << nNonce;
    uint256 expected_hash=ss.GetHash();
    if(meFCmp(&expected_hash,sParameterSetHash.c_str(),32))
    {
        return false;                    
    }
    return true;
}

bool ProcessFloatchainVerack(CNode* pfrom, CDataStream& vRecv,bool fIsVerackack,bool *disconnect_flag)
{   
    string sParameterSet="";
    string sParameterSetHash="";
    string sSigScript="";
    bool only_check_signature; 
    
    uint64_t nNonce;
    
    only_check_signature=false;
    if(!fIsVerackack)
    {
        if(pfrom->nVersionNonceSent == 1)
        {
            LogPrintf("FChn: We don't expect verack from peer=%d\n", pfrom->id);
            return false;
        }
        vRecv >> pfrom->nVerackNonceReceived >> sParameterSet;        
        nNonce=pfrom->nVersionNonceSent;
    }
    else
    {
        if(pfrom->nVerackNonceSent == 1)
        {
            LogPrintf("FChn: We don't expect verackack from peer=%d\n", pfrom->id);
            return false;
        }        
        nNonce=pfrom->nVerackNonceSent;
        pfrom->fVerackackReceived=true;
    }
    
    vRecv >> sParameterSetHash >> sSigScript;
    
    if(sParameterSetHash.size() != 32)
    {
            LogPrintf("FChn: Wrong parameter set hash size (%d) from peer=%d\n", sParameterSetHash.size(), pfrom->id);
            return false;        
    }
    
    if(FC_gState->m_NetworkParams->m_Status == FC_PRM_STATUS_VALID)
    {
        if(!VerifyFloatchainVerackHash(sParameterSetHash,nNonce))
        {
            if(!fIsVerackack)
            {
                LogPrintf("FChn: Parameter set hash mismatch from peer=%d\n",  pfrom->id);
                return false;                    
            }
        }
        else
        {
            if(fIsVerackack)
            {
                only_check_signature=true;
//                return false;
            }
        }
    }
    
    if(FCP_ANYONE_CAN_CONNECT == 0)
    {
        CScript scriptSig((unsigned char*)sSigScript.c_str(),(unsigned char*)sSigScript.c_str()+sSigScript.size());
        
        vector<unsigned char> vchSigOut;
        vector<unsigned char> vchPubKey;
    
        opcodetype opcode;

        CScript::const_iterator pc = scriptSig.begin();
    
        if (!scriptSig.GetOp(pc, opcode, vchSigOut))
        {
            LogPrintf("FChn: Cannot extract signature from sigScript from peer=%d\n",  pfrom->id);
            return false;                                
        }
    
        vchSigOut.resize(vchSigOut.size()-1);
        if (!scriptSig.GetOp(pc, opcode, vchPubKey))
        {
            LogPrintf("FChn: Cannot extract pubkey from sigScript from peer=%d\n", pfrom->id);
            return false;                                
        }
    
        CPubKey pubKeyOut(vchPubKey);
        if (!pubKeyOut.IsValid())
        {
            LogPrintf("FChn: Invalid pubkey received from peer=%d\n", pfrom->id);
            return false;                                
        }

        
        CKeyID pubKeyHash=pubKeyOut.GetID();
        pfrom->kAddrRemote=pubKeyHash;
        if(fIsVerackack)
        {
            LogPrintf("FChn: Connection from %s received on peer=%d in verackack (%s)\n",CBitcoinAddress(pubKeyHash).ToString().c_str(), pfrom->id,pfrom->addr.ToString());
            if(!FloatchainNode_CanConnect(pfrom))
            {
                LogPrintf("FChn: Permission denied for address %s received from peer=%d\n",CBitcoinAddress(pubKeyHash).ToString().c_str(), pfrom->id);
                pfrom->fVerackackReceived=false;                                // Will resend minimal parameter set
                if(only_check_signature)
                {
                    return false;                                                
                }
//                return false;                                                
            }
/*            
            {
                LOCK(cs_vNodes);
                BOOST_FOREACH(CNode* pnode, vNodes)
                {
                    if(pnode->id != pfrom->id)
                    {
                        if(pnode->kAddrRemote == pubKeyHash)
                        {
                            LogPrintf("FChn: Already connected to address %s received from peer=%d\n",CBitcoinAddress(pubKeyHash).ToString().c_str(), pfrom->id);
                            pfrom->fVerackackReceived=false;                                // Will resend minimal parameter set
                            if(only_check_signature)
                            {
                                return false;                                                
                            }                        
                        }
                    }
                }
            }            
 */ 
        }
        else
        {
            LogPrintf("FChn: Connection from %s received on peer=%d in verack\n",CBitcoinAddress(pubKeyHash).ToString().c_str(), pfrom->id);
        }
        
        CHashWriter ss(SER_GETHASH, 0);
        ss << vector<unsigned char>((unsigned char*)sParameterSetHash.c_str(), (unsigned char*)sParameterSetHash.c_str()+32);
        ss << vector<unsigned char>((unsigned char*)&nNonce, (unsigned char*)&nNonce+sizeof(nNonce));
        uint256 signed_hash=ss.GetHash();

        if(!pubKeyOut.Verify(signed_hash,vchSigOut))
        {
            LogPrintf("FChn: Wrong signature received from peer=%d\n", pfrom->id);
            return false;                                
        }        
        if(only_check_signature)
        {
            *disconnect_flag=false;
            return false;
        }
    }
    else
    {
        if(only_check_signature)
        {
            *disconnect_flag=false;
            return false;
        }
    }
       
    int current_status=FC_gState->m_NetworkParams->m_Status;
    
    if((current_status == FC_PRM_STATUS_EMPTY) ||
       ((current_status == FC_PRM_STATUS_MINIMAL) && pfrom->fVerackackSent))
    {
        pfrom->fDisconnect=true;        
                
        if(FC_gState->m_NetworkParams->Set(FC_gState->m_Params->NetworkName(),sParameterSet.c_str(),sParameterSet.size()) == FC_ERR_NOERROR)
        {
            FC_gState->m_NetworkParams->Validate();        

            switch(current_status)
            {
                case FC_PRM_STATUS_EMPTY:
                    if((FC_gState->m_NetworkParams->m_Status != FC_PRM_STATUS_MINIMAL) &&
                       (FC_gState->m_NetworkParams->m_Status != FC_PRM_STATUS_VALID))
                    {
                        LogPrintf("FChn: Invalid parameter set received from %s\n", pfrom->addr.ToString());
                        return false;                       
                    }
                    break;
                case FC_PRM_STATUS_MINIMAL:
                    if((FC_gState->m_NetworkParams->m_Status != FC_PRM_STATUS_MINIMAL) &&
                       (FC_gState->m_NetworkParams->m_Status != FC_PRM_STATUS_VALID))
                    {
                        LogPrintf("FChn: Invalid parameter set received from %s\n", pfrom->addr.ToString());
                        return false;                       
                    }
                    break;
                default:
                    LogPrintf("FChn: Invalid current parameter set status\n", pfrom->addr.ToString());
                    return false;                       
            }
            if(strcmp(FC_gState->m_NetworkParams->Name(),FC_gState->m_Params->NetworkName()))
            {
                LogPrintf("FChn: Parameter set received from %s has different name\n", pfrom->addr.ToString());
                FC_gState->m_NetworkParams->Read(FC_gState->m_Params->NetworkName());
                FC_gState->m_NetworkParams->Validate();
                return false;                       
            }
            
            if(FC_gState->m_NetworkParams->m_Status == FC_PRM_STATUS_VALID)
            {
                if(!VerifyFloatchainVerackHash(sParameterSetHash,nNonce))
                {
                    LogPrintf("FChn: Parameter set received from peer %d doesn't match received hash\n", pfrom->id);
                    return false;                    
                }
            }

            if( (pwalletMain != NULL) && !pwalletMain->vchDefaultKey.IsValid() && (FC_gState->m_NetworkParams->GetParam("privatekeyversion",NULL) == NULL) )
            {
                LogPrintf("FChn: Parameter set received from %s doesn't contain privatekeyversion fields, not stored\n", pfrom->addr.ToString());                
            }
            else
            {
                if(FC_gState->m_NetworkParams->Write(1))
                {
                    LogPrintf("FChn: Cannot store parameter set received from %s\n", pfrom->addr.ToString());
                    FC_gState->m_NetworkParams->m_Status=FC_PRM_STATUS_ERROR;
                    return false;                       
                }
                else
                {
                    LogPrintf("FChn: Successfully stored parameter set received from %s\n", pfrom->addr.ToString());
                }
            }
        }
        else
        {
            LogPrintf("FChn: Cannot parse parameter set received from %s\n", pfrom->addr.ToString());
            return false;                       
        }
        
    }
    
    return true;
}

bool PushFloatChainVerack(CNode* pfrom, bool fIsVerackack)
{
    vector<unsigned char>vParameterSet;
    vector<unsigned char>vParameterSetHash;
    vector<unsigned char>vSigScript;
    uint64_t nNonce;
    
    if(!fIsVerackack)
    {
        if(FC_gState->m_NetworkParams->m_Status != FC_PRM_STATUS_VALID)         // Ignoring version message from seed node in initial handshake
        {
            return false;
        }
    }
    
    if(!fIsVerackack)
    {
        if((pfrom->fDefaultMessageStart && (FCP_ANYONE_CAN_CONNECT!=0) ) || 
            pfrom->fVerackackReceived)
        {
            vParameterSet=vector<unsigned char>(FC_gState->m_NetworkParams->m_lpData,FC_gState->m_NetworkParams->m_lpData+FC_gState->m_NetworkParams->m_Size);        
            LogPrintf("FChn: Sending full parameter set to %s\n", pfrom->addr.ToString());
        }
        else
        {
            string fields_to_send [] ={"protocolversion","chainname","defaultrpcport","networkmessagestart","addresspubkeyhashversion","addresschecksumvalue","privatekeyversion"};
            int NumFieldsToSend=1;
            unsigned char *ptr;
            int size;
//            if(pfrom->fDefaultMessageStart)
            {
                NumFieldsToSend=7;                
            }
            LogPrintf("FChn: Sending minimal parameter set to %s\n", pfrom->addr.ToString());
            for(int f=0;f<NumFieldsToSend;f++)
            {
                vParameterSet.insert(vParameterSet.end(),(unsigned char*)(fields_to_send[f].c_str()),(unsigned char*)(fields_to_send[f].c_str())+fields_to_send[f].size()+1);
                ptr=(unsigned char*)FC_gState->m_NetworkParams->GetParam(fields_to_send[f].c_str(),&size);
                if(ptr == NULL)
                {
                    LogPrintf("FChn: Internal error: Invalid parameter set\n");
                    return false;
                }
                vParameterSet.insert(vParameterSet.end(),ptr-FC_PRM_PARAM_SIZE_BYTES,ptr+size);                
            }            
        }
        nNonce=pfrom->nVersionNonceReceived;
    }
    else
    {
        if(pfrom->fVerackackSent)                      
        {
            return true;
        }        
        nNonce=pfrom->nVerackNonceReceived;
        pfrom->fVerackackSent=true;
    }

    unsigned char* stored_hash=(unsigned char*)FC_gState->m_NetworkParams->GetParam("chainparamshash",NULL);
    CHashWriter ssHash(SER_GETHASH, 0);
    if(stored_hash)
    {
        ssHash << vector<unsigned char>(stored_hash, stored_hash+32);
    }
    ssHash << nNonce;
    uint256 hash_to_send=ssHash.GetHash();
        
    vParameterSetHash=vector<unsigned char>((unsigned char*)&hash_to_send, (unsigned char*)&hash_to_send+32);
    
    CHashWriter ssSig(SER_GETHASH, 0);
    ssSig << vParameterSetHash;
    ssSig << vector<unsigned char>((unsigned char*)&nNonce, (unsigned char*)&nNonce+sizeof(nNonce));
    uint256 signed_hash=ssSig.GetHash();
            
    CKey key;
    CKeyID keyID;
    CPubKey pkey;            
    bool key_found=false;
    
    
    if(mapArgs.count("-handshakelocal"))
    {
        CBitcoinAddress address(mapArgs["-handshakelocal"]);
        LogPrintf("FChn: Using handshake address %d\n",mapArgs["-handshakelocal"].c_str());
        if (address.IsValid())    
        {
            CTxDestination dst=address.Get();
            CKeyID *lpKeyID=boost::get<CKeyID> (&dst);
            if(lpKeyID)
            {
                if(pwalletMain->GetKey(*lpKeyID, key))
                {
                    keyID=*lpKeyID;
                    key_found=true;
                }
                else
                {
                    LogPrintf("FChn: handshakelocal address %s doesn't belong to this wallet, using default address for connection\n",mapArgs["-handshakelocal"].c_str());
                    printf("\nWarning: handshakelocal address %s doesn't belong to this wallet, using default address for connection\n\n",mapArgs["-handshakelocal"].c_str());                
                }
            }
            else
            {
                LogPrintf("FChn: handshakelocal address %s is invalid, using default address for connection\n",mapArgs["-handshakelocal"].c_str());
                printf("\nWarning: handshakelocal address %s is invalid, using default address for connection\n\n",mapArgs["-handshakelocal"].c_str());
            }
        }        
        else
        {
            LogPrintf("FChn: handshakelocal address %s is invalid, using default address for connection\n",mapArgs["-handshakelocal"].c_str());
            printf("\nWarning: handshakelocal address %s is invalid, using default address for connection\n\n",mapArgs["-handshakelocal"].c_str());
        }
    }

    if(!key_found)
    {
        if(!pwalletMain->GetKeyFromAddressBook(pkey,FC_PTP_CONNECT))
        {
            LogPrintf("FChn: Cannot find address having connect permission, trying default key\n");
            pkey=pwalletMain->vchDefaultKey;
        }
        keyID=pkey.GetID();
    }
    
    
    pfrom->kAddrLocal=keyID;
    
    if(!pwalletMain->GetKey(keyID, key))
    {
        LogPrintf("FChn: Internal error: Connection address not found in the wallet\n");
        return false;        
    }
    
    if(key_found)
    {
        pkey=key.GetPubKey();
    }
    
    CScript scriptSig;
    vector<unsigned char> vchSig;
    if (!key.Sign(signed_hash, vchSig))
    {
        LogPrintf("FChn: Internal error: Cannot sign parameter set hash\n");
        return false;        
    }
    
    vchSig.push_back(0x00);
    scriptSig << vchSig;
    scriptSig << ToByteVector(pkey);
    
    vSigScript=scriptSig;
    
    if(!fIsVerackack)
    {
        GetRandBytes((unsigned char*)&(pfrom->nVerackNonceSent), sizeof(pfrom->nVerackNonceSent));
        pfrom->PushMessage("verack", pfrom->nVerackNonceSent, vParameterSet, vParameterSetHash, vSigScript);        
    }
    else
    {
        pfrom->PushMessage("verackack", vParameterSetHash, vSigScript);                
    }
    
    return true;
    
}


