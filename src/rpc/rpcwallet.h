// Copyright (c) 2014-2016 The Bitcoin Core developers
// Original code was distributed under the MIT software license.
// Copyright (c) 2014-2017 Coin Sciences Ltd
// FloatChain code distributed under the GPLv3 license, see COPYING file.

#ifndef RPCWALLET_H
#define	RPCWALLET_H

#include "structs/base58.h"
#include "utils/core_io.h"
#include "rpc/rpcserver.h"
#include "core/init.h"
#include "utils/util.h"
#include "wallet/wallet.h"

#include <boost/assign/list_of.hpp>
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"


#include "floatchain/floatchain.h"
#include "wallet/wallettxs.h"
#include "rpc/rpcutils.h"

void SendMoneyToSeveralAddresses(const std::vector<CTxDestination> addresses, CAmount nValue, CWalletTx& wtxNew,FC_Script *dropscript,CScript scriptOpReturn,const std::vector<CTxDestination>& fromaddresses);
vector<CTxDestination> ParseAddresses(string param, bool create_full_list, bool allow_scripthash);
void FindAddressesWithPublishPermission(std::vector<CTxDestination>& fromaddresses,FC_EntityDetails *stream_entity);
set<string> ParseAddresses(Value param, isminefilter filter);
bool CBitcoinAddressFromTxEntity(CBitcoinAddress &address,FC_TxEntity *lpEntity);
Object StreamItemEntry(const CWalletTx& wtx,const unsigned char *stream_id, bool verbose);
Object TxOutEntry(const CTxOut& TxOutIn,int vout,const CTxIn& TxIn,uint256 hash,FC_Buffer *amounts,FC_Script *lpScript);
void WalletTxToJSON(const CWalletTx& wtx, Object& entry,bool skipWalletConflicts = false, int vout = -1);
void MinimalWalletTxToJSON(const CWalletTx& wtx, Object& entry);
Object AddressEntry(CBitcoinAddress& address,uint32_t verbose);
void SetSynchronizedFlag(CTxDestination &dest,Object &ret);




#endif	/* RPCWALLET_H */

