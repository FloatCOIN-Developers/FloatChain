// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2014-2016 The Bitcoin Core developers
// Original code was distributed under the MIT software license.
// Copyright (c) 2014-2017 Coin Sciences Ltd
// FloatChain code distributed under the GPLv3 license, see COPYING file.


#include "rpc/rpcwallet.h"

void mergeGenesisWithAssets(FC_Buffer *genesis_amounts, FC_Buffer *asset_amounts)
{
    if(FC_gState->m_Features->ShortTxIDInTx() == 0)
    {
        return; 
    }
    
    unsigned char buf[FC_AST_ASSET_FULLREF_BUF_SIZE];
    int64_t quantity;
    memset(buf,0,FC_AST_ASSET_FULLREF_BUF_SIZE);
    
    for(int a=0;a<genesis_amounts->GetCount();a++)
    {
        meFCpy(buf+FC_AST_SHORT_TXID_OFFSET,(unsigned char*)genesis_amounts->GetRow(a)+FC_AST_SHORT_TXID_OFFSET,FC_AST_SHORT_TXID_SIZE);
        FC_SetABRefType(buf,FC_AST_ASSET_REF_TYPE_SHORT_TXID);
        quantity=FC_GetABQuantity(genesis_amounts->GetRow(a));
        int row=asset_amounts->Seek(buf);
        if(row >= 0)
        {
            int64_t last=FC_GetABQuantity(asset_amounts->GetRow(row));
            quantity+=last;
            FC_SetABQuantity(asset_amounts->GetRow(row),quantity);
        }
        else
        {
            FC_SetABQuantity(buf,quantity);
            asset_amounts->Add(buf);                        
        }        
    }    
    
    genesis_amounts->Clear();
}

Value issuefroFCmd(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 4)
        throw runtime_error("Help message not found\n");

    CBitcoinAddress address(params[1].get_str());
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");

    // Amount
    CAmount nAmount = FCP_MINIMUM_PER_OUTPUT;
    if (params.size() > 5 && params[5].type() != null_type)
    {
        nAmount = AmountFromValue(params[5]);
        if(nAmount < 0)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid amount");
    }
    // Wallet comments
    CWalletTx wtx;
    
    FC_Script *lpScript;
    lpScript=new FC_Script;

    int64_t quantity;
    int multiple;
    double dQuantity;
    double dUnit;
    
    dQuantity=0.;
    dUnit=1.;
    if (params.size() > 3 && params[3].type() != null_type)
    {
        dQuantity=params[3].get_real();
    }
    
    if (params.size() > 4 && params[4].type() != null_type)
    {
        dUnit=params[4].get_real();
    }
    
    if(dQuantity<0.)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid quantity. Should be positive.");
    if(dUnit<=0.)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid smallest unit. Valid Range [0.00000001 - 1].");
    if(dUnit>1.)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid smallest unit. Valid Range [0.00000001 - 1].");
    
    multiple=(int)((1 + 0.1*dUnit)/dUnit);
    if(fabs((double)multiple*dUnit-1)>0.0001)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid smallest unit. 1 should be divisible by this number.");
    
    quantity=(int64_t)(dQuantity*multiple+0.1);
    if(quantity<0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid quantity or smallest unit. ");
    if(multiple<=0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid quantity or smallest unit.");

    int64_t quantity_to_check=(int64_t)((dQuantity+0.1*dUnit)/dUnit);
    double dDelta;

    dDelta=fabs(dQuantity/dUnit-quantity);
    quantity_to_check=quantity;
    while(quantity_to_check > 1)
    {
        quantity_to_check /= 2;
        dDelta /= 2.;
    }
    
    if(dDelta>1.e-14)
    {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Quantity should be divisible by smallest unit.");        
        
    }

    

    if(!AddressCanReceive(address.Get()))
    {
        throw JSONRPCError(RPC_INSUFFICIENT_PERMISSIONS, "Destination address doesn't have receive permission");        
    }
    
    lpScript->SetAssetGenesis(quantity);
    
    FC_Script *lpDetailsScript;
    lpDetailsScript=NULL;
    
    FC_Script *lpDetails;
    lpDetails=new FC_Script;
    lpDetails->AddElement();
    
    int ret,type;
    string asset_name="";
    bool is_open=false;
    bool name_is_found=false;
    
    if (params.size() > 2 && params[2].type() != null_type)// && !params[2].get_str().empty())
    {
        if(params[2].type() == obj_type)
        {
            Object objSpecialParams = params[2].get_obj();
            BOOST_FOREACH(const Pair& s, objSpecialParams) 
            {  
                if(s.name_ == "name")
                {
                    if(!name_is_found)
                    {
                        asset_name=s.value_.get_str().c_str();
                        name_is_found=true;
                    }
                }
                if(s.name_ == "open")
                {
                    if(s.value_.type() == bool_type)
                    {
                        is_open=s.value_.get_bool();
                    }
                    else
                    {
                        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid value for 'open' field, should be boolean");                                                                
                    }
                }
            }
        }
        else
        {
            if(params[2].get_str().size())
            {
                asset_name=params[2].get_str();
            }
        }
    }
    
    if(FC_gState->m_Features->Streams())
    {
        if(asset_name == "*")
        {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid asset name: *");                                                                                            
        }
    }
    
    unsigned char buf_a[FC_AST_ASSET_REF_SIZE];    
    if(AssetRefDecode(buf_a,asset_name.c_str(),asset_name.size()))
    {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid asset name, looks like an asset reference");                                                                                                    
    }
    
    if(asset_name.size())
    {
        ret=ParseAssetKey(asset_name.c_str(),NULL,NULL,NULL,NULL,&type,FC_ENT_TYPE_ANY);
        if(ret != FC_ASSET_KEY_INVALID_NAME)
        {
            if(type == FC_ENT_KEYTYPE_NAME)
            {
                throw JSONRPCError(RPC_DUPLICATE_NAME, "Asset or stream with this name already exists");                                    
            }
            else
            {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid asset name");                                    
            }
        }        
    }

    if(FC_gState->m_Features->OpDropDetailsScripts())
    {
        if(asset_name.size())
        {
            lpDetails->SetSpecialParamValue(FC_ENT_SPRM_NAME,(const unsigned char*)(asset_name.c_str()),asset_name.size());//+1);
        }        
        lpDetails->SetSpecialParamValue(FC_ENT_SPRM_ASSET_MULTIPLE,(unsigned char*)&multiple,4);
    }
    
    if(is_open)
    {
        unsigned char b=1;        
        lpDetails->SetSpecialParamValue(FC_ENT_SPRM_FOLLOW_ONS,&b,1);
    }
    
    if (params.size() > 6)
    {
        if(params[6].type() == obj_type)
        {
            Object objParams = params[6].get_obj();
            BOOST_FOREACH(const Pair& s, objParams) 
            {  
                lpDetails->SetParamValue(s.name_.c_str(),s.name_.size(),(unsigned char*)s.value_.get_str().c_str(),s.value_.get_str().size());                
            }
        }
        else
        {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid custom fields, expecting object");                                        
        }
    }
    int err;
    size_t bytes;
    const unsigned char *script;
    size_t elem_size;
    const unsigned char *elem;
    CScript scriptOpReturn=CScript();
    
    script=lpDetails->GetData(0,&bytes);
//    if(bytes > 0)
    {
        lpDetailsScript=new FC_Script;
        
        if(FC_gState->m_Features->OpDropDetailsScripts())
        {
            err=lpDetailsScript->SetNewEntityType(FC_ENT_TYPE_ASSET,0,script,bytes);
            if(err)
            {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid custom fields or asset name, too long");                                                        
            }
            
            elem = lpDetailsScript->GetData(0,&elem_size);
            scriptOpReturn << vector<unsigned char>(elem, elem + elem_size) << OP_DROP << OP_RETURN;                    
        }
        else
        {
            err=lpDetailsScript->SetAssetDetails(asset_name.c_str(),multiple,script,bytes);
            if(err)
            {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid custom fields or asset name, too long");                                                    
            }

            elem = lpDetailsScript->GetData(0,&elem_size);
            scriptOpReturn << OP_RETURN << vector<unsigned char>(elem, elem + elem_size);
        }
    }
        
    

    vector<CTxDestination> addresses;    
    addresses.push_back(address.Get());
    
    vector<CTxDestination> fromaddresses;        
    
    if(params[0].get_str() != "*")
    {
        fromaddresses=ParseAddresses(params[0].get_str(),false,false);

        if(fromaddresses.size() != 1)
        {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Single from-address should be specified");                        
        }

        if( (IsMine(*pwalletMain, fromaddresses[0]) & ISMINE_SPENDABLE) != ISMINE_SPENDABLE )
        {
            throw JSONRPCError(RPC_WALLET_ADDRESS_NOT_FOUND, "Private key for from-address is not found in this wallet");                        
        }
        
        set<CTxDestination> thisFromAddresses;

        BOOST_FOREACH(const CTxDestination& fromaddress, fromaddresses)
        {
            thisFromAddresses.insert(fromaddress);
        }

        CPubKey pkey;
        if(!pwalletMain->GetKeyFromAddressBook(pkey,FC_PTP_ISSUE,&thisFromAddresses))
        {
            throw JSONRPCError(RPC_INSUFFICIENT_PERMISSIONS, "from-address doesn't have issue permission");                
        }   
    }
    else
    {
        CPubKey pkey;
        if(!pwalletMain->GetKeyFromAddressBook(pkey,FC_PTP_ISSUE))
        {
            throw JSONRPCError(RPC_INSUFFICIENT_PERMISSIONS, "This wallet doesn't have keys with issue permission");                
        }        
    }
    
    EnsureWalletIsUnlocked();
    LOCK (pwalletMain->cs_wallet_send);
    
    SendMoneyToSeveralAddresses(addresses, nAmount, wtx, lpScript, scriptOpReturn,fromaddresses);

    if(lpDetailsScript)
    {
        delete lpDetailsScript;
    }
    delete lpDetails;
    delete lpScript;
  
    return wtx.GetHash().GetHex();    
}
 


Value issuecmd(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 3)
        throw runtime_error("Help message not found\n");

    Array ext_params;
    ext_params.push_back("*");
    BOOST_FOREACH(const Value& value, params)
    {
        ext_params.push_back(value);
    }
    
    return issuefroFCmd(ext_params,fHelp);    
}

Value issuemorefroFCmd(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 4 || params.size() > 6)
        throw runtime_error("Help message not found\n");

    CBitcoinAddress address(params[1].get_str());
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
       
    // Amount
    CAmount nAmount = FCP_MINIMUM_PER_OUTPUT;
    if (params.size() > 4 && params[4].type() != null_type)
    {
        nAmount = AmountFromValue(params[4]);
        if(nAmount < 0)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid amount");
    }
    
    FC_Script *lpScript;
    lpScript=new FC_Script;
    unsigned char buf[FC_AST_ASSET_FULLREF_BUF_SIZE];
    memset(buf,0,FC_AST_ASSET_FULLREF_BUF_SIZE);
    int multiple=1;
    FC_EntityDetails entity;
    
    if (params.size() > 2 && params[2].type() != null_type && !params[2].get_str().empty())
    {        
        ParseEntityIdentifier(params[2],&entity, FC_ENT_TYPE_ASSET);           
        meFCpy(buf,entity.GetFullRef(),FC_AST_ASSET_FULLREF_SIZE);
        if(FC_gState->m_Features->ShortTxIDInTx() == 0)
        {
            if(entity.IsUnconfirmedGenesis())
            {
                throw JSONRPCError(RPC_UNCONFIRMED_ENTITY, string("Unconfirmed asset: ")+params[2].get_str());            
            }
        }
        multiple=entity.GetAssetMultiple();        
    }
    else
    {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid asset identifier");        
    }
    
    Value raw_qty=params[3];
    
    int64_t quantity = (int64_t)(raw_qty.get_real() * multiple + 0.499999);
    if(quantity<0)
    {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid asset quantity");        
    }
        
    FC_SetABQuantity(buf,quantity);
    
    FC_Buffer *lpBuffer;
    lpBuffer=new FC_Buffer;
    
    FC_InitABufferDefault(lpBuffer);
    
    lpBuffer->Add(buf);
    
    lpScript->SetAssetQuantities(lpBuffer,FC_SCR_ASSET_SCRIPT_TYPE_FOLLOWON);
    
    delete lpBuffer;
        
        
    
    // Wallet comments
    CWalletTx wtx;
    
    

    if(!AddressCanReceive(address.Get()))
    {
        throw JSONRPCError(RPC_INSUFFICIENT_PERMISSIONS, "Destination address doesn't have receive permission");        
    }
    
    
    FC_Script *lpDetailsScript;
    lpDetailsScript=NULL;
    
    FC_Script *lpDetails;
    lpDetails=new FC_Script;
    lpDetails->AddElement();
        
    if (params.size() > 5)
    {
        if(params[5].type() == obj_type)
        {
            Object objParams = params[5].get_obj();
            BOOST_FOREACH(const Pair& s, objParams) 
            {  
                lpDetails->SetParamValue(s.name_.c_str(),s.name_.size(),(unsigned char*)s.value_.get_str().c_str(),s.value_.get_str().size());                
            }
        }
        else
        {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid extra-params, expecting object");                                        
        }
    }
    
    int err;
    size_t bytes;
    const unsigned char *script;
    size_t elem_size;
    const unsigned char *elem;
    CScript scriptOpReturn=CScript();
    
    script=lpDetails->GetData(0,&bytes);
    if(bytes > 0)
    {
//        FC_DumpSize("script",script,bytes,bytes);
        lpDetailsScript=new FC_Script;
        if(FC_gState->m_Features->OpDropDetailsScripts())
        {
            lpDetailsScript->SetEntity(entity.GetTxID()+FC_AST_SHORT_TXID_OFFSET);
            err=lpDetailsScript->SetNewEntityType(FC_ENT_TYPE_ASSET,1,script,bytes);
            if(err)
            {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid custom fields, too long");                                                        
            }

            elem = lpDetailsScript->GetData(0,&elem_size);
            scriptOpReturn << vector<unsigned char>(elem, elem + elem_size) << OP_DROP;
            elem = lpDetailsScript->GetData(1,&elem_size);
            scriptOpReturn << vector<unsigned char>(elem, elem + elem_size) << OP_DROP << OP_RETURN;                
        }
        else
        {
            lpDetailsScript->SetGeneralDetails(script,bytes);
            elem = lpDetailsScript->GetData(0,&elem_size);
            scriptOpReturn << OP_RETURN << vector<unsigned char>(elem, elem + elem_size);
        }
    }
        
    

    vector<CTxDestination> addresses;    
    addresses.push_back(address.Get());
    
    vector<CTxDestination> fromaddresses;        
    
    if(params[0].get_str() != "*")
    {
        fromaddresses=ParseAddresses(params[0].get_str(),false,false);

        if(fromaddresses.size() != 1)
        {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Single from-address should be specified");                        
        }
        
        if( (IsMine(*pwalletMain, fromaddresses[0]) & ISMINE_SPENDABLE) != ISMINE_SPENDABLE )
        {
            throw JSONRPCError(RPC_WALLET_ADDRESS_NOT_FOUND, "Private key for from-address is not found in this wallet");                        
        }        
    }
    else
    {
/*        
        CPubKey pkey;
        if(!pwalletMain->GetKeyFromAddressBook(pkey,FC_PTP_ISSUE))
        {
            throw JSONRPCError(RPC_WALLET_ADDRESS_NOT_FOUND, "This wallet doesn't have keys with issue permission");                
        }        
 */ 
    }
    
    if(FC_gState->m_Assets->FindEntityByFullRef(&entity,buf))
    {
        if(entity.AllowedFollowOns())
        {
            if(fromaddresses.size() == 1)
            {
                CKeyID *lpKeyID=boost::get<CKeyID> (&fromaddresses[0]);
                if(lpKeyID != NULL)
                {
                    if(FC_gState->m_Permissions->CanIssue(entity.GetTxID(),(unsigned char*)(lpKeyID)) == 0)
                    {
                        throw JSONRPCError(RPC_INSUFFICIENT_PERMISSIONS, "Issuing more units for this asset is not allowed from this address");                                                                        
                    }                                                 
                }
                else
                {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "Issuing more units is allowed only from P2PKH addresses");                                                
                }
            }
            else
            {
                bool issuer_found=false;
                BOOST_FOREACH(const PAIRTYPE(CBitcoinAddress, CAddressBookData)& item, pwalletMain->mapAddressBook)
                {
                    const CBitcoinAddress& address = item.first;
                    CKeyID keyID;

                    if(address.GetKeyID(keyID))
                    {
                        if(FC_gState->m_Permissions->CanIssue(entity.GetTxID(),(unsigned char*)(&keyID)))
                        {
                            issuer_found=true;
                        }
                    }
                }                    
                if(!issuer_found)
                {
                    throw JSONRPCError(RPC_INSUFFICIENT_PERMISSIONS, "Issuing more units for this asset is not allowed from this wallet");                                                                                            
                }
            }
        }
        else
        {
            throw JSONRPCError(RPC_NOT_ALLOWED, "Issuing more units not allowed for this asset: "+params[2].get_str());                            
        }
    }   
    else
    {
        throw JSONRPCError(RPC_ENTITY_NOT_FOUND, "Asset not found");                
    }
    
    
    EnsureWalletIsUnlocked();
    LOCK (pwalletMain->cs_wallet_send);
    
    SendMoneyToSeveralAddresses(addresses, nAmount, wtx, lpScript, scriptOpReturn,fromaddresses);

    if(lpDetailsScript)
    {
        delete lpDetailsScript;
    }
    delete lpDetails;
    delete lpScript;
  
    return wtx.GetHash().GetHex();    
}
 
Value issuemorecmd(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 3)
        throw runtime_error("Help message not found\n");

    Array ext_params;
    ext_params.push_back("*");
    BOOST_FOREACH(const Value& value, params)
    {
        ext_params.push_back(value);
    }
    
    return issuemorefroFCmd(ext_params,fHelp);
}    

Value getmultibalances(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 5)
        throw runtime_error("Help message not found\n");

    isminefilter filter = ISMINE_SPENDABLE;
    
    Object balances;

    bool fUnlockedOnly=true;
    bool fIncludeWatchOnly=false;    
    int nMinDepth = 1;
    
    if(params.size() > 2)
    {
        nMinDepth = params[2].get_int();
    }
    
    if(params.size() > 3)
    {
        if(params[3].get_bool())
        {
            fIncludeWatchOnly=true;
        }
    }
    
    if(params.size() > 4)
    {
        if(params[4].get_bool())
        {
            fUnlockedOnly=false;
        }
    }
    
    if(fIncludeWatchOnly)
    {
        filter = filter | ISMINE_WATCH_ONLY;        
    }
    
    set<string> setAddresses;
    set<string> setAddressesWithBalances;
    if(params.size() > 0)
    {
        if( (params[0].type() != str_type) || (params[0].get_str() != "*") )
        {        
            filter = filter | ISMINE_WATCH_ONLY;        
            
            setAddresses=ParseAddresses(params[0],filter);
            if(setAddresses.size() == 0)
            {
                return balances;
            }
        }
    }
    
    vector<string> inputStrings;
    
    if (params.size() > 1 && params[1].type() != null_type && ((params[1].type() != str_type) || (params[1].get_str() !="*" ) ) )
    {        
        if(params[1].type() == str_type)
        {
            inputStrings.push_back(params[1].get_str());
            if(params[1].get_str() == "")
            {
                return balances;                
            }
        }
        else
        {
            inputStrings=ParseStringList(params[1]);
            if(inputStrings.size() == 0)
            {
                return balances;
            }
        }
    }
    
    set<uint256> setAssets;        
    if(inputStrings.size())
    {
        for(int is=0;is<(int)inputStrings.size();is++)
        {
            FC_EntityDetails entity;
            ParseEntityIdentifier(inputStrings[is],&entity, FC_ENT_TYPE_ASSET);           
            uint256 hash=*(uint256*)entity.GetTxID();
            if (setAssets.count(hash))
            {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, duplicate asset: " + inputStrings[is]);                        
            }
            setAssets.insert(hash);
        }
    }

    FC_Buffer *asset_amounts;
    FC_Buffer *addresstxid_amounts;
    unsigned char buf[80+FC_AST_ASSET_QUANTITY_SIZE];
    unsigned char totbuf[80+FC_AST_ASSET_QUANTITY_SIZE];
    int64_t quantity;
    int row;
    unsigned char *ptr;  
    
    assert(pwalletMain != NULL);

    
    {
        LOCK(cs_main);
        
        FC_Script *lpScript;
        lpScript=new FC_Script;    
        
        asset_amounts=new FC_Buffer;
        FC_InitABufferMap(asset_amounts);
        asset_amounts->Clear();

        addresstxid_amounts=new FC_Buffer;
        addresstxid_amounts->Initialize(80,80+FC_AST_ASSET_QUANTITY_SIZE,FC_BUF_MODE_MAP);
        addresstxid_amounts->Clear();

        memset(totbuf,0,80+FC_AST_ASSET_QUANTITY_SIZE);
        addresstxid_amounts->Add(totbuf,totbuf+80);
        
        
        vector<COutput> vecOutputs;
        pwalletMain->AvailableCoins(vecOutputs, false, NULL, fUnlockedOnly,true);
        BOOST_FOREACH(const COutput& out, vecOutputs) 
        {        
            if(!out.IsTrusted())
            {
                if (out.nDepth < nMinDepth)
                {
                    continue;           
                }
            }
            
            CTxOut txout;
            uint256 hash=out.GetHashAndTxOut(txout);
            
            string str_addr;
            CTxDestination address;
            if (!ExtractDestination(txout.scriptPubKey, address))
            {
                continue;
            }
            
            str_addr=CBitcoinAddress(address).ToString();
            if ( (setAddresses.size()>0) && (setAddresses.count(str_addr) == 0) )
            {
                continue;
            }
            isminetype fIsMine=pwalletMain->IsMine(txout);

            if (!(fIsMine & filter))
            {
                continue;        
            }
         
            memset(totbuf,0,80+FC_AST_ASSET_QUANTITY_SIZE);
            memset(buf,0,80+FC_AST_ASSET_QUANTITY_SIZE);
            meFCpy(buf,str_addr.c_str(),str_addr.size());
            
            quantity=txout.nValue;
            if(quantity > 0)
            {
                quantity+=FC_GetLE(addresstxid_amounts->GetRow(0)+80,FC_AST_ASSET_QUANTITY_SIZE);
                FC_PutLE(addresstxid_amounts->GetRow(0)+80,&quantity,FC_AST_ASSET_QUANTITY_SIZE);
                row=addresstxid_amounts->Seek(buf);
                quantity=txout.nValue;
                if(row >= 0)
                {
                    quantity+=FC_GetLE(addresstxid_amounts->GetRow(row)+80,FC_AST_ASSET_QUANTITY_SIZE);
                    FC_PutLE(addresstxid_amounts->GetRow(row)+80,&quantity,FC_AST_ASSET_QUANTITY_SIZE);
                }
                else
                {                             
                    FC_PutLE(buf+80,&quantity,FC_AST_ASSET_QUANTITY_SIZE);
                    addresstxid_amounts->Add(buf,buf+80);                        
                }                
            }
            asset_amounts->Clear();
            if(CreateAssetBalanceList(txout,asset_amounts,lpScript))
            {
                for(int a=0;a<asset_amounts->GetCount();a++)
                {
                    const unsigned char *txid;
                    txid=NULL;
                    ptr=(unsigned char *)asset_amounts->GetRow(a);
                    if(FC_GetABRefType(ptr) == FC_AST_ASSET_REF_TYPE_GENESIS)
//                    if(FC_GetLE(ptr,4) == 0)
                    {
//                        hash=out.tx->GetHash();
                        txid=(unsigned char*)&hash;
                    }
                    else
                    {
                        FC_EntityDetails entity;

                        if(FC_gState->m_Assets->FindEntityByFullRef(&entity,ptr))
                        {
                            txid=entity.GetTxID();
                        }                                
                    }
                    if(txid)
                    {
                        if(setAssets.size())
                        {
//                            hash=*(uint256*)txid;
                            if(setAssets.count(*(uint256*)txid) == 0)
                            {
                                txid=NULL;
                            }
                        }
                    }
                    
                    if(txid)
                    {
                        quantity=FC_GetABQuantity(ptr);
                        meFCpy(totbuf+48,txid,32);
                        row=addresstxid_amounts->Seek(totbuf);
                        if(row >= 0)
                        {
                            quantity+=FC_GetLE(addresstxid_amounts->GetRow(row)+80,FC_AST_ASSET_QUANTITY_SIZE);
                            FC_PutLE(addresstxid_amounts->GetRow(row)+80,&quantity,FC_AST_ASSET_QUANTITY_SIZE);
                        }
                        else
                        {                             
                            FC_PutLE(totbuf+80,&quantity,FC_AST_ASSET_QUANTITY_SIZE);
                            addresstxid_amounts->Add(totbuf,totbuf+80);                        
                        }                
                        
                        quantity=FC_GetABQuantity(ptr);
                        meFCpy(buf+48,txid,32);
                        row=addresstxid_amounts->Seek(buf);
                        if(row >= 0)
                        {
                            quantity+=FC_GetLE(addresstxid_amounts->GetRow(row)+80,FC_AST_ASSET_QUANTITY_SIZE);
                            FC_PutLE(addresstxid_amounts->GetRow(row)+80,&quantity,FC_AST_ASSET_QUANTITY_SIZE);
                        }
                        else
                        {                             
                            FC_PutLE(buf+80,&quantity,FC_AST_ASSET_QUANTITY_SIZE);
                            addresstxid_amounts->Add(buf,buf+80);                        
                        }                
                    }
                }                
            }        
        }

        bool take_total=false;
        bool have_addr=true;
        
        memset(totbuf,0,80+FC_AST_ASSET_QUANTITY_SIZE);
        while(have_addr)
        {
            memset(buf,0,80+FC_AST_ASSET_QUANTITY_SIZE);
            Array addr_balances;
            set<uint256> setAssetsWithBalances;
            int64_t btc=0;
            for(int i=0;i<addresstxid_amounts->GetCount();i++)
            {
                bool take_it=false;
                ptr=addresstxid_amounts->GetRow(i);
                if(ptr[47] == 0)
                {
                    if(take_total || (meFCmp(ptr,totbuf,48) != 0))
                    {
                        if(take_total || (meFCmp(buf,totbuf,48) == 0))
                        {
                            take_it=true;
                            if(!take_total)
                            {
                                meFCpy(buf,ptr,48);
                            }
                        }
                        else
                        {
                            if(meFCmp(ptr,buf,48) == 0)
                            {
                                take_it=true;
                            }
                        }
                    }
                }
                if(take_it)
                {
                    quantity=FC_GetLE(ptr+80,FC_AST_ASSET_QUANTITY_SIZE);
                    if(meFCmp(ptr+48,totbuf+48,32) == 0)
                    {
                        btc=quantity;
                    }
                    else
                    {
                        Object asset_entry;
                        asset_entry=AssetEntry(ptr+48,quantity,0x00);
                        addr_balances.push_back(asset_entry);  
                        if(setAssets.size())
                        {
                            uint256 issue_txid=*(uint256*)(ptr+48);
                            setAssetsWithBalances.insert(issue_txid);
                        }
                    }
                    ptr[47]=0x01;
                }
            }
            if(setAssets.size())
            {
                BOOST_FOREACH(const uint256& rem_asset, setAssets) 
                {
                    if(setAssetsWithBalances.count(rem_asset) == 0)
                    {
                        Object asset_entry;
                        asset_entry=AssetEntry((unsigned char*)&rem_asset,0,0x00);
                        addr_balances.push_back(asset_entry);   
                    }
                }                
            }
            if(FCP_WITH_NATIVE_CURRENCY)
            {
                Object asset_entry;
                asset_entry=AssetEntry(NULL,btc,0x00);
                addr_balances.push_back(asset_entry);                        
            }
            
            string out_addr="";
            if(take_total)
            {
                have_addr=false;
                out_addr="total";
                if(setAddresses.size())
                {
                    BOOST_FOREACH(const string& rem_addr, setAddresses) 
                    {
                        if(setAddressesWithBalances.count(rem_addr) == 0)
                        {
                            Array empty_balances;
                            if(setAssets.size())
                            {
                                BOOST_FOREACH(const uint256& rem_asset, setAssets) 
                                {
                                    Object asset_entry;
                                    asset_entry=AssetEntry((unsigned char*)&rem_asset,0,0x00);
                                    empty_balances.push_back(asset_entry);   
                                }                
                            }
                            balances.push_back(Pair(rem_addr, empty_balances));                                    
                        }
                    }
                }
            }
            else
            {
                if(meFCmp(buf,totbuf,48) == 0)
                {
                    take_total=true;
                }
                else
                {
                    out_addr=string((char*)buf);
                }
            }       
            if(out_addr.size())
            {
                setAddressesWithBalances.insert(out_addr);
                balances.push_back(Pair(out_addr, addr_balances));                
            }
        }
        
        
        delete addresstxid_amounts;
        delete asset_amounts;
        delete lpScript;
    }        
        
    return balances;    
}

Value getaddressbalances(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 3)
        throw runtime_error("Help message not found\n");

    vector<CTxDestination> fromaddresses;        
    fromaddresses=ParseAddresses(params[0].get_str(),false,true);
    
    if(fromaddresses.size() != 1)
    {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Single from-address should be specified");                        
    }
    
    isminefilter filter = ISMINE_SPENDABLE;
    
    filter = filter | ISMINE_WATCH_ONLY;

    bool fUnlockedOnly=true;
    
    if(params.size() > 2)
        if(params[2].get_bool())
            fUnlockedOnly=false;
    
    
    
    set<CBitcoinAddress> setAddress;
    
    
    bool check_account=true;
    
    BOOST_FOREACH(const CTxDestination& fromaddress, fromaddresses)
    {
        setAddress.insert(fromaddress);
    }
        
    int nMinDepth = 1;
    if (params.size() > 1)
        nMinDepth = params[1].get_int();

    FC_Buffer *asset_amounts;
    asset_amounts=new FC_Buffer;
    FC_InitABufferMap(asset_amounts);
    asset_amounts->Clear();
    
    FC_Buffer *genesis_amounts;
    genesis_amounts=new FC_Buffer;
    genesis_amounts->Initialize(32,32+FC_AST_ASSET_QUANTITY_SIZE,FC_BUF_MODE_MAP);
    genesis_amounts->Clear();
    
    FC_Script *lpScript;
    lpScript=new FC_Script;    

    int last_size=0;
    Array assets;
    CAmount totalBTC=0;
    vector<COutput> vecOutputs;        
    assert(pwalletMain != NULL);
    
    uint160 addr=0;
    
    if(fromaddresses.size() == 1)
    {
        CTxDestination addressRet=fromaddresses[0];        
        const CKeyID *lpKeyID=boost::get<CKeyID> (&addressRet);
        const CScriptID *lpScriptID=boost::get<CScriptID> (&addressRet);
        if(lpKeyID)
        {
            addr=*(uint160*)lpKeyID;
        }
        if(lpScriptID)
        {
            addr=*(uint160*)lpScriptID;
        }
    }
    
    pwalletMain->AvailableCoins(vecOutputs, false, NULL, fUnlockedOnly,true,addr);
    BOOST_FOREACH(const COutput& out, vecOutputs) {
        
/*        
        if(out.nDepth == 0)
        {
            if(!out.tx->IsTrusted(out.nDepth))
            {
                continue;
            }
        }
        else
        {
 */ 
        if(!out.IsTrusted())
        {
            if (out.nDepth < nMinDepth)
            {
                continue;           
            }
        }
/*
         }
*/
        CTxOut txout;
        uint256 hash=out.GetHashAndTxOut(txout);
        if(check_account)
        {
            CTxDestination address;
            if (!ExtractDestination(txout.scriptPubKey, address))
                continue;
            
            if (!setAddress.count(address))
                continue;
        }
        
        isminetype fIsMine=pwalletMain->IsMine(txout);
        if (!(fIsMine & filter))
        {
            continue;        
        }
        
        
        
        CAmount nValue = txout.nValue;
        totalBTC+=nValue;
        if(CreateAssetBalanceList(txout,asset_amounts,lpScript))
        {
            unsigned char *ptr;
            string assetref="";
            int64_t quantity;
            unsigned char buf[FC_AST_ASSET_FULLREF_BUF_SIZE];
            memset(buf,0,FC_AST_ASSET_FULLREF_BUF_SIZE);
            int n;
            bool is_genesis;
            
            n=asset_amounts->GetCount();
            
            for(int a=last_size;a<n;a++)
            {
                Object asset_entry;
                ptr=(unsigned char *)asset_amounts->GetRow(a);
                is_genesis=false;
                if(FC_GetABRefType(ptr) == FC_AST_ASSET_REF_TYPE_GENESIS)
                {
                    FC_EntityDetails entity;
                    quantity=FC_GetABQuantity(asset_amounts->GetRow(a));
                    if(FC_gState->m_Assets->FindEntityByTxID(&entity,(unsigned char*)&hash))
                    {
                        if((entity.IsUnconfirmedGenesis() != 0) && (FC_gState->m_Features->ShortTxIDInTx() == 0) )
                        {
                            is_genesis=true;                                
                        }
                        else
                        {
                            ptr=(unsigned char *)entity.GetFullRef();
                            meFCpy(buf,ptr,FC_AST_ASSET_FULLREF_SIZE);
                            int row=asset_amounts->Seek(buf);
                            if(row >= 0)
                            {
                                int64_t last=FC_GetABQuantity(asset_amounts->GetRow(row));
                                quantity+=last;
                                FC_SetABQuantity(asset_amounts->GetRow(row),quantity);
                            }
                            else
                            {
                                FC_SetABQuantity(buf,quantity);
                                asset_amounts->Add(buf);                        
                            }
                        }
                    }                
                    
                    if(is_genesis)
                    {
                        int row=genesis_amounts->Seek(&hash);
                        if(row >= 0)
                        {
                            int64_t last=FC_GetLE(genesis_amounts->GetRow(row)+32,FC_AST_ASSET_QUANTITY_SIZE);
                            quantity+=last;
                            FC_PutLE(genesis_amounts->GetRow(row)+32,&quantity,FC_AST_ASSET_QUANTITY_SIZE);
                        }
                        else
                        {
                            FC_SetABQuantity(buf,quantity);
                            genesis_amounts->Add(&hash,buf+FC_AST_ASSET_QUANTITY_OFFSET);                        
                        }                        
                    }
                }                
            }
            last_size=asset_amounts->GetCount();
        }
    }
    
    
    unsigned char *ptr;
    string assetref="";

    mergeGenesisWithAssets(genesis_amounts, asset_amounts);
    
    for(int a=0;a<asset_amounts->GetCount();a++)
    {
        Object asset_entry;
        ptr=(unsigned char *)asset_amounts->GetRow(a);
        
        FC_EntityDetails entity;
        const unsigned char *txid;
        
        if(FC_gState->m_Assets->FindEntityByFullRef(&entity,ptr))
        {
            txid=entity.GetTxID();
            asset_entry=AssetEntry(txid,FC_GetABQuantity(ptr),0x00);
            assets.push_back(asset_entry);
        }        
    }
                
    for(int a=0;a<genesis_amounts->GetCount();a++)
    {
        Object asset_entry;
        ptr=(unsigned char *)genesis_amounts->GetRow(a);
        
        asset_entry=AssetEntry(ptr,FC_GetLE(ptr+32,FC_AST_ASSET_QUANTITY_SIZE),0x00);
        assets.push_back(asset_entry);
    }
    
    if(FCP_WITH_NATIVE_CURRENCY)
    {
        Object asset_entry;
        asset_entry=AssetEntry(NULL,totalBTC,0x00);
        assets.push_back(asset_entry);        
    }
    

    delete lpScript;
    delete asset_amounts;
    delete genesis_amounts;
    
/* FCHN END */        
    return assets;
}



Value getassetbalances(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 4)
        throw runtime_error("Help message not found\n");
    
    bool check_account=false;
    
    isminefilter filter = ISMINE_SPENDABLE;
    if(params.size() > 2)
        if(params[2].get_bool())
            filter = filter | ISMINE_WATCH_ONLY;

    bool fUnlockedOnly=true;
    
    if(params.size() > 3)
        if(params[3].get_bool())
            fUnlockedOnly=false;
    
    set<CBitcoinAddress> setAddress;
        
    if (params.size() > 0)
    {
        if (params[0].get_str() != "*") 
        {
            if (params[0].get_str() != "") 
            {
                if(FC_gState->m_WalletMode & FC_WMD_ADDRESS_TXS)
                {
                    throw JSONRPCError(RPC_NOT_SUPPORTED, "Accounts are not supported with scalable wallet - if you need getassetbalances, run floatchaind -walletdbversion=1 -rescan, but the wallet will perform worse");        
                }            
            }
            BOOST_FOREACH(const PAIRTYPE(CBitcoinAddress, CAddressBookData)& item, pwalletMain->mapAddressBook)
            {
                const CBitcoinAddress& address = item.first;
                const string& strName = item.second.name;
                if (strName == params[0].get_str())
                {
                   setAddress.insert(address);                    
                }
            }            
            check_account=true;
        }
    }

    
    int nMinDepth = 1;
    if (params.size() > 1)
        nMinDepth = params[1].get_int();

    FC_Buffer *asset_amounts;
    asset_amounts=new FC_Buffer;
    FC_InitABufferMap(asset_amounts);
    asset_amounts->Clear();
    
    FC_Buffer *genesis_amounts;
    genesis_amounts=new FC_Buffer;
    genesis_amounts->Initialize(32,32+FC_AST_ASSET_QUANTITY_SIZE,FC_BUF_MODE_MAP);
    genesis_amounts->Clear();
    
    FC_Script *lpScript;
    lpScript=new FC_Script;    

    int last_size=0;
    Array assets;
    CAmount totalBTC=0;
    vector<COutput> vecOutputs;
    assert(pwalletMain != NULL);
    pwalletMain->AvailableCoins(vecOutputs, false, NULL, fUnlockedOnly,true);
    BOOST_FOREACH(const COutput& out, vecOutputs) {
        
/*        
        if(out.nDepth == 0)
        {
            if(!out.tx->IsTrusted(out.nDepth))
            {
                continue;
            }
        }
        else
        {
 */ 
            if(!out.IsTrusted())
            {
                if (out.nDepth < nMinDepth)
                {
                    continue;           
                }
            }
/*
        }
*/
        CTxOut txout;
        uint256 hash=out.GetHashAndTxOut(txout);
        
        if(check_account)
        {
            CTxDestination address;
            if (!ExtractDestination(txout.scriptPubKey, address))
                continue;

            if (!setAddress.count(address))
                continue;
        }
        
        isminetype fIsMine=pwalletMain->IsMine(txout);

        if (!(fIsMine & filter))
        {
            continue;        
        }
        
        CAmount nValue = txout.nValue;
        totalBTC+=nValue;
        
        if(CreateAssetBalanceList(txout,asset_amounts,lpScript))
        {
            unsigned char *ptr;
            string assetref="";
            int64_t quantity;
            unsigned char buf[FC_AST_ASSET_FULLREF_BUF_SIZE];
            memset(buf,0,FC_AST_ASSET_FULLREF_BUF_SIZE);
            int n;
            bool is_genesis;
            
            n=asset_amounts->GetCount();
            
            for(int a=last_size;a<n;a++)
            {
                Object asset_entry;
                ptr=(unsigned char *)asset_amounts->GetRow(a);
                is_genesis=false;
                if(FC_GetABRefType(ptr) == FC_AST_ASSET_REF_TYPE_GENESIS)
                {
                    FC_EntityDetails entity;
                    quantity=FC_GetABQuantity(asset_amounts->GetRow(a));
                    if(FC_gState->m_Assets->FindEntityByTxID(&entity,(unsigned char*)&hash))
                    {
                        if((entity.IsUnconfirmedGenesis() != 0) && (FC_gState->m_Features->ShortTxIDInTx() == 0) )
                        {
                            is_genesis=true;
                        }
                        else
                        {
                            ptr=(unsigned char *)entity.GetFullRef();
                            meFCpy(buf,ptr,FC_AST_ASSET_FULLREF_SIZE);
                            int row=asset_amounts->Seek(buf);
                            if(row >= 0)
                            {
                                int64_t last=FC_GetABQuantity(asset_amounts->GetRow(row));
                                quantity+=last;
                                FC_SetABQuantity(asset_amounts->GetRow(row),quantity);
                            }
                            else
                            {
                                FC_SetABQuantity(buf,quantity);
                                asset_amounts->Add(buf);                        
                            }
                        }
                    }                
                    
                    if(is_genesis)
                    {
                        int row=genesis_amounts->Seek(&hash);
                        if(row >= 0)
                        {
                            int64_t last=FC_GetLE(genesis_amounts->GetRow(row)+32,FC_AST_ASSET_QUANTITY_SIZE);
                            quantity+=last;
                            FC_PutLE(genesis_amounts->GetRow(row)+32,&quantity,FC_AST_ASSET_QUANTITY_SIZE);
                        }
                        else
                        {
                            FC_SetABQuantity(buf,quantity);
                            genesis_amounts->Add(&hash,buf+FC_AST_ASSET_QUANTITY_OFFSET);                        
                        }                        
                    }
                }                
            }
            last_size=asset_amounts->GetCount();
        }
    }
    
    
    unsigned char *ptr;
    string assetref="";

    mergeGenesisWithAssets(genesis_amounts, asset_amounts);   
    
    for(int a=0;a<asset_amounts->GetCount();a++)
    {
        Object asset_entry;
        ptr=(unsigned char *)asset_amounts->GetRow(a);
        
        FC_EntityDetails entity;
        const unsigned char *txid;
        
        if(FC_gState->m_Assets->FindEntityByFullRef(&entity,ptr))
        {
            txid=entity.GetTxID();
            asset_entry=AssetEntry(txid,FC_GetABQuantity(ptr),0x00);
            assets.push_back(asset_entry);
        }        
    }
    
    for(int a=0;a<genesis_amounts->GetCount();a++)
    {
        Object asset_entry;
        ptr=(unsigned char *)genesis_amounts->GetRow(a);
        
        asset_entry=AssetEntry(ptr,FC_GetLE(ptr+32,FC_AST_ASSET_QUANTITY_SIZE),0x00);
        assets.push_back(asset_entry);
    }
    
    delete lpScript;
    delete asset_amounts;
    delete genesis_amounts;
    
/* FCHN END */        
    return assets;
}


Value gettotalbalances(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 3)
        throw runtime_error("Help message not found\n");

    Array new_params;
    
    new_params.push_back("*");
    
    for(int i=0;i<(int)params.size();i++)
    {
        new_params.push_back(params[i]);
    }

    return  getassetbalances(new_params,fHelp);
}


Value listassets(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 4)
       throw runtime_error("Help message not found\n");

    FC_Buffer *assets;
    unsigned char *txid;
    uint32_t output_level;
    Array results;
    
    int count,start;
    count=2147483647;
    if (params.size() > 2)    
    {
        count=paramtoint(params[2],true,0,"Invalid count");
    }
    start=-count;
    if (params.size() > 3)    
    {
        start=paramtoint(params[3],false,0,"Invalid start");
    }
    
    assets=NULL;
    vector<string> inputStrings;
    if (params.size() > 0 && params[0].type() != null_type && ((params[0].type() != str_type) || (params[0].get_str() !="*" ) ) )
    {        
        if(params[0].type() == str_type)
        {
            inputStrings.push_back(params[0].get_str());
            if(params[0].get_str() == "")
            {
                return results;                
            }
        }
        else
        {
            inputStrings=ParseStringList(params[0]);        
            if(inputStrings.size() == 0)
            {
                return results;
            }
        }
    }
    if(inputStrings.size())
    {
        {
            LOCK(cs_main);
            for(int is=0;is<(int)inputStrings.size();is++)
            {
                FC_EntityDetails entity;
                ParseEntityIdentifier(inputStrings[is],&entity, FC_ENT_TYPE_ASSET);           
                uint256 hash=*(uint256*)entity.GetTxID();

                assets=FC_gState->m_Assets->GetEntityList(assets,(unsigned char*)&hash,FC_ENT_TYPE_ASSET);
            }
        }
    }
    else
    {        
        {
            LOCK(cs_main);
            assets=FC_gState->m_Assets->GetEntityList(assets,NULL,FC_ENT_TYPE_ASSET);
        }
    }
    
    if(assets == NULL)
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Cannot open asset database");

    output_level=0x0F;
    
    if (params.size() > 1)    
    {
        if(paramtobool(params[1]))
        {
            output_level|=0x20;
        }
    }
    
    FC_AdjustStartAndCount(&count,&start,assets->GetCount());
    
    Array partial_results;
    int unconfirmed_count=0;
    if(count > 0)
    {
        for(int i=0;i<assets->GetCount();i++)
        {
            Object entry;

            txid=assets->GetRow(i);
            entry=AssetEntry(txid,-1,output_level);
            if(entry.size()>0)
            {
                BOOST_FOREACH(const Pair& p, entry) 
                {
                    if(p.name_ == "assetref")
                    {
                        if(p.value_.type() == str_type)
                        {
                            results.push_back(entry);                        
                        }
                        else
                        {
                            unconfirmed_count++;
                        }
                    }
                }            
            }            
        }

        sort(results.begin(), results.end(), AssetCompareByRef);
        
        for(int i=0;i<assets->GetCount();i++)
        {
            Object entry;

            txid=assets->GetRow(i);

            entry=AssetEntry(txid,-1,output_level);
            if(entry.size()>0)
            {
                BOOST_FOREACH(const Pair& p, entry) 
                {
                    if(p.name_ == "assetref")
                    {
                        if(p.value_.type() != str_type)
                        {
                            results.push_back(entry);                        
                        }
                    }
                }            
            }            
        }
    }

    bool return_partial=false;
    if(count != assets->GetCount())
    {
        return_partial=true;
    }
    FC_gState->m_Assets->FreeEntityList(assets);
    if(return_partial)
    {
        for(int i=start;i<start+count;i++)
        {
            partial_results.push_back(results[i]);                                                                
        }
        return partial_results;
    }
     
    return results;
}
Object ListAssetTransactions(const CWalletTx& wtx, FC_EntityDetails *entity, bool fLong, FC_Buffer *amounts,FC_Script *lpScript)
{
    Object entry;
    unsigned char bufEmptyAssetRef[FC_AST_ASSET_QUANTITY_OFFSET];
    uint32_t new_entity_type;
    set<uint256> streams_already_seen;
    Array aMetaData;
    Array aItems;
    
    double units=1.;
    units= 1./(double)(entity->GetAssetMultiple());
    
    uint256 hash=wtx.GetHash();
    
    map <CTxDestination,int64_t> mAddresses;
    
    memset(bufEmptyAssetRef,0,FC_AST_ASSET_QUANTITY_OFFSET);
    
    BOOST_FOREACH(const CTxIn& txin, wtx.vin)
    {
        CTxOut prevout;
                
        int err;
        const CWalletTx& prev=pwalletTxsMain->GetWalletTx(txin.prevout.hash,NULL,&err);
        if(err == FC_ERR_NOERROR)
        {
            if (txin.prevout.n < prev.vout.size())
            {
                prevout=prev.vout[txin.prevout.n];
                CTxDestination address;
                int required=0;
                
                ExtractDestination(prevout.scriptPubKey, address);
                
                string strFailReason;
                amounts->Clear();
                if(CreateAssetBalanceList(prevout,amounts,lpScript,&required))
                {
                    for(int i=0;i<amounts->GetCount();i++)
                    {
                        int64_t quantity=-1;
                        if(meFCmp(entity->GetFullRef(),amounts->GetRow(i),FC_AST_ASSET_FULLREF_SIZE) == 0)
                        {
                            quantity=FC_GetABQuantity(amounts->GetRow(i));
                        }
                        else
                        {
                            if(meFCmp(entity->GetTxID(),&(txin.prevout.hash),sizeof(uint256)) == 0)
                            {
                                if( FC_GetABRefType(amounts->GetRow(i)) == FC_AST_ASSET_REF_TYPE_GENESIS )                    
//                                if(meFCmp(bufEmptyAssetRef,amounts->GetRow(i),FC_AST_ASSET_QUANTITY_OFFSET) == 0)
                                {
                                    quantity=FC_GetABQuantity(amounts->GetRow(i));
                                }                            
                            }
                        }

                        if(quantity >= 0)
                        {                        
                            map<CTxDestination, int64_t>::iterator itold = mAddresses.find(address);
                            if (itold == mAddresses.end())
                            {
                                mAddresses.insert(make_pair(address, -quantity));
                            }
                            else
                            {
                                itold->second-=quantity;
                            }                            
                        }
                    }
                }
            }
        }
    }    

    for (int i = 0; i < (int)wtx.vout.size(); ++i)
    {
        const CTxOut& txout = wtx.vout[i];
        if(!txout.scriptPubKey.IsUnspendable())
        {
            string strFailReason;
            CTxDestination address;
            int required=0;

            ExtractDestination(txout.scriptPubKey, address);

            amounts->Clear();
            if(CreateAssetBalanceList(txout,amounts,lpScript,&required))
            {
                for(int i=0;i<amounts->GetCount();i++)
                {
                    int64_t quantity=-1;
                    if(meFCmp(entity->GetFullRef(),amounts->GetRow(i),FC_AST_ASSET_FULLREF_SIZE) == 0)
                    {
                        quantity=FC_GetABQuantity(amounts->GetRow(i));
                    }
                    else
                    {
                        if(meFCmp(entity->GetTxID(),&hash,sizeof(uint256)) == 0)
                        {
                            if( FC_GetABRefType(amounts->GetRow(i)) == FC_AST_ASSET_REF_TYPE_GENESIS )                    
//                            if(meFCmp(bufEmptyAssetRef,amounts->GetRow(i),FC_AST_ASSET_QUANTITY_OFFSET) == 0)
                            {
                                quantity=FC_GetABQuantity(amounts->GetRow(i));
                            }                            
                        }
                    }
                            
                    if(quantity >= 0)
                    {
                        map<CTxDestination, int64_t>::iterator itold = mAddresses.find(address);
                        if (itold == mAddresses.end())
                        {
                            mAddresses.insert(make_pair(address, +quantity));
                        }
                        else
                        {
                            itold->second+=quantity;
                        }                            
                    }
                }
            }            
        }
        else
        {            
            const CScript& script2 = wtx.vout[i].scriptPubKey;        
            CScript::const_iterator pc2 = script2.begin();

            lpScript->Clear();
            lpScript->SetScript((unsigned char*)(&pc2[0]),(size_t)(script2.end()-pc2),FC_SCR_TYPE_SCRIPTPUBKEY);
            
            size_t elem_size;
            const unsigned char *elem;

            if(lpScript->GetNumElements()<=1)
            {
                if(lpScript->GetNumElements()==1)
                {
                    elem = lpScript->GetData(lpScript->GetNumElements()-1,&elem_size);
                    aMetaData.push_back(OpReturnEntry(elem,elem_size,wtx.GetHash(),i));
                }                        
            }
            else
            {
                elem = lpScript->GetData(lpScript->GetNumElements()-1,&elem_size);
                if(elem_size)
                {
                    aMetaData.push_back(OpReturnEntry(elem,elem_size,wtx.GetHash(),i));
                }
                
                lpScript->SetElement(0);
                lpScript->GetNewEntityType(&new_entity_type);
            }
        }
    }
    
    if(mAddresses.size() == 0)
    {
        return entry;            
    }
    
    Array vin;
    Array vout;
    
    if(fLong)
    {
        BOOST_FOREACH(const CTxIn& txin, wtx.vin)
        {
            CTxOut TxOutIn;
            vin.push_back(TxOutEntry(TxOutIn,-1,txin,txin.prevout.hash,amounts,lpScript));
        }
    }
    for (int i = 0; i < (int)wtx.vout.size(); ++i)
    {
        CTxIn TxIn;
        Value data_item_entry=DataItemEntry(wtx,i,streams_already_seen,0x01);
        if(!data_item_entry.is_null())
        {
            aItems.push_back(data_item_entry);
        }
        if(fLong)
        {
            Array aTxOutItems;
            if(!data_item_entry.is_null())
            {
                aTxOutItems.push_back(data_item_entry);
            }
            Object txout_entry=TxOutEntry(wtx.vout[i],i,TxIn,wtx.GetHash(),amounts,lpScript);
            txout_entry.push_back(Pair("items", aTxOutItems));
            vout.push_back(txout_entry);
        }
    }        
    
    int64_t other_amount=0;
    
    Object oBalance;
    BOOST_FOREACH(const PAIRTYPE(CTxDestination, int64_t)& item, mAddresses)
    {
        const CTxDestination dest=item.first;
        const CKeyID *lpKeyID=boost::get<CKeyID> (&dest);
        const CScriptID *lpScript=boost::get<CScriptID> (&dest);
        if( (lpKeyID == NULL) && (lpScript == NULL) )
        {
            other_amount=item.second;
        }
        else            
        {
            oBalance.push_back(Pair(CBitcoinAddress(item.first).ToString(), units*item.second));            
        }
    }
    if(other_amount != 0)
    {
        oBalance.push_back(Pair("", other_amount));                    
    }
    
    entry.push_back(Pair("addresses", oBalance));
    entry.push_back(Pair("items", aItems));
    entry.push_back(Pair("data", aMetaData));
    
    WalletTxToJSON(wtx, entry, true);

    if(fLong)
    {
        entry.push_back(Pair("vin", vin));
        entry.push_back(Pair("vout", vout));
        string strHex = EncodeHexTx(static_cast<CTransaction>(wtx));
        entry.push_back(Pair("hex", strHex));
    }    
    
    return entry;    
}


Value getassettransaction(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 3)
        throw runtime_error("Help message not found\n");
    
    if((FC_gState->m_WalletMode & FC_WMD_TXS) == 0)
    {
        throw JSONRPCError(RPC_NOT_SUPPORTED, "API is not supported with this wallet version. To get this functionality, run \"floatchaind -walletdbversion=2 -rescan\" ");        
    }   
           
    FC_EntityDetails asset_entity;
    ParseEntityIdentifier(params[0],&asset_entity,FC_ENT_TYPE_ASSET);
    
    FC_TxEntityStat entStat;
    entStat.Zero();
    meFCpy(&entStat,asset_entity.GetShortRef(),FC_gState->m_NetworkParams->m_AssetRefSize);
    entStat.m_Entity.m_EntityType=FC_TET_ASSET;
    entStat.m_Entity.m_EntityType |= FC_TET_CHAINPOS;
    
    if(!pwalletTxsMain->FindEntity(&entStat))
    {
        throw JSONRPCError(RPC_NOT_SUBSCRIBED, "Not subscribed to this asset");                                
    }
    
    uint256 hash = ParseHashV(params[1], "parameter 2");
    
    bool verbose=false;
    
    if (params.size() > 2)    
    {
        verbose=paramtobool(params[2]);
    }
    
    const CWalletTx& wtx=pwalletTxsMain->GetWalletTx(hash,NULL,NULL);
    FC_Buffer *asset_amounts;
    asset_amounts=new FC_Buffer;
    FC_InitABufferMap(asset_amounts);
    
    FC_Script *lpScript;
    lpScript=new FC_Script;    
    
    Object entry=ListAssetTransactions(wtx, &asset_entity, verbose, asset_amounts, lpScript);
    
    if(entry.size() == 0)
    {
        throw JSONRPCError(RPC_TX_NOT_FOUND, "This transaction was not found for this asset");                
    }
    
    delete asset_amounts;
    delete lpScript;
    
    return entry;
}


Value listassettransactions(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 5)
        throw runtime_error("Help message not found\n");

    if((FC_gState->m_WalletMode & FC_WMD_TXS) == 0)
    {
        throw JSONRPCError(RPC_NOT_SUPPORTED, "API is not supported with this wallet version. To get this functionality, run \"floatchaind -walletdbversion=2 -rescan\" ");        
    }   

    Array retArray;

    FC_TxEntityStat entStat;
    FC_TxEntityRow *lpEntTx;
    
    FC_EntityDetails asset_entity;
    ParseEntityIdentifier(params[0],&asset_entity,FC_ENT_TYPE_ASSET);

    int count,start,shift;
    bool verbose=false;
    bool fGenesis;
    
    if (params.size() > 1)    
    {
        verbose=paramtobool(params[1]);
    }
    
    count=10;
    if (params.size() > 2)    
    {
        count=paramtoint(params[2],true,0,"Invalid count");
    }
    start=-count;
    if (params.size() > 3)    
    {
        start=paramtoint(params[3],false,0,"Invalid start");
    }
    
    bool fLocalOrdering = false;
    if (params.size() > 4)
        fLocalOrdering = params[4].get_bool();
    
    entStat.Zero();
    meFCpy(&entStat,asset_entity.GetShortRef(),FC_gState->m_NetworkParams->m_AssetRefSize);
    entStat.m_Entity.m_EntityType=FC_TET_ASSET;
    if(fLocalOrdering)
    {
        entStat.m_Entity.m_EntityType |= FC_TET_TIMERECEIVED;
    }
    else
    {
        entStat.m_Entity.m_EntityType |= FC_TET_CHAINPOS;
    }
    if(!pwalletTxsMain->FindEntity(&entStat))
    {
        throw JSONRPCError(RPC_NOT_SUBSCRIBED, "Not subscribed to this asset");                                
    }
    
    FC_Buffer *entity_rows;
    entity_rows=new FC_Buffer;
    entity_rows->Initialize(FC_TDB_ENTITY_KEY_SIZE,FC_TDB_ROW_SIZE,FC_BUF_MODE_DEFAULT);

    FC_Buffer *asset_amounts;
    asset_amounts=new FC_Buffer;
    FC_InitABufferMap(asset_amounts);
    
    FC_Script *lpScript;
    lpScript=new FC_Script;    

    pwalletTxsMain->GetList(&entStat.m_Entity,1,1,entity_rows);
    shift=1;
    if(entity_rows->GetCount())
    {
        lpEntTx=(FC_TxEntityRow*)entity_rows->GetRow(0);
        if(meFCmp(lpEntTx->m_TxId,asset_entity.GetTxID(),FC_TDB_TXID_SIZE) == 0)
        {
            shift=0;
        }
    }
    
    FC_AdjustStartAndCount(&count,&start,entStat.m_LastPos+shift);
    
    fGenesis=false;
    if(shift)
    {
        if(start)
        {
            start-=shift;
            shift=0;
        }
        else
        {
            count-=shift;
            fGenesis=true;
        }
    }
    
    pwalletTxsMain->GetList(&entStat.m_Entity,start+1,count,entity_rows);
    
    
    for(int i=0;i<entity_rows->GetCount()+shift;i++)
    {
        uint256 hash;
        if(fGenesis && (i == 0))
        {
            meFCpy(&hash,asset_entity.GetTxID(),FC_TDB_TXID_SIZE);            
        }
        else
        {
            lpEntTx=(FC_TxEntityRow*)entity_rows->GetRow(i-shift);
            meFCpy(&hash,lpEntTx->m_TxId,FC_TDB_TXID_SIZE);            
        }
        const CWalletTx& wtx=pwalletTxsMain->GetWalletTx(hash,NULL,NULL);
        Object entry=ListAssetTransactions(wtx, &asset_entity, verbose, asset_amounts, lpScript);

                
        if(entry.size())
        {
            retArray.push_back(entry);                                
        }
    }
    
    delete entity_rows;
    delete asset_amounts;
    delete lpScript;
    
    return retArray;
}
