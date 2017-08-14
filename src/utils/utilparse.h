// Copyright (c) 2014-2017 Coin Sciences Ltd
// FloatChain code distributed under the GPLv3 license, see COPYING file.

#ifndef FLOATCHAINUTILS_H
#define	FLOATCHAINUTILS_H

#include "structs/base58.h"
#include "floatchain/floatchain.h"
#include "primitives/transaction.h"
#include "keys/key.h"
#include "core/main.h"

bool ExtractDestinationScriptValid(const CScript& scriptPubKey, CTxDestination& addressRet);
const unsigned char* GetAddressIDPtr(const CTxDestination& address);
bool ParseFloatchainTxOutToBuffer(uint256 hash,const CTxOut& txout,FC_Buffer *amounts,FC_Script *lpScript,int *allowed,int *required,std::map<uint32_t, uint256>* mapSpecialEntity,std::string& strFailReason);
bool ParseFloatchainTxOutToBuffer(uint256 hash,const CTxOut& txout,FC_Buffer *amounts,FC_Script *lpScript,int *allowed,int *required,std::string& strFailReason);
bool CreateAssetBalanceList(const CTxOut& txout,FC_Buffer *amounts,FC_Script *lpScript,int *required);
bool CreateAssetBalanceList(const CTxOut& txout,FC_Buffer *amounts,FC_Script *lpScript);
void LogAssetTxOut(std::string message,uint256 hash,int index,unsigned char* assetrefbin,int64_t quantity);
bool AddressCanReceive(CTxDestination address);
bool FindFollowOnsInScript(const CScript& script1,FC_Buffer *amounts,FC_Script *lpScript);






#endif	/* FLOATCHAINUTILS_H */

