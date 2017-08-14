// Copyright (c) 2014-2017 Coin Sciences Ltd
// Copyright (c) 2017 Float Coin Developers
// FloatChain code distributed under the GPLv3 license, see COPYING file.

#ifndef GLOBALS_H
#define	GLOBALS_H

FC_State* FC_gState;
unsigned int MIN_RELAY_TX_FEE = 1000;                                           // new
unsigned int MAX_OP_RETURN_RELAY = 40;                                          // standard.h
unsigned int MAX_BLOCK_SIZE = 1000000;                                          // block.h
unsigned int DEFAULT_BLOCK_MAX_SIZE = 750000;                                   // main.h
unsigned int MAX_BLOCKFILE_SIZE = 0x8000000;                                    // main.h
unsigned int MAX_STANDARD_TX_SIZE = 100000;                                     // main.h
int COINBASE_MATURITY = 100;                                                    // main.h
unsigned int MAX_SIZE = 0x02000000;                                             // serialize,h
int64_t COIN = 100000000;                                                       // amount.h
int64_t CENT = 1000000;                                                         // amount.h
int64_t MAX_MONEY = 21000000 * COIN;                                            // amount.h
unsigned int MAX_SCRIPT_ELEMENT_SIZE=520;                                       // script.h
int MAX_OP_RETURN_SHOWN=16384;

int FCP_MAX_STD_OP_RETURN_COUNT=0;
int64_t FCP_INITIAL_BLOCK_REWARD=0;
int64_t FCP_FIRST_BLOCK_REWARD=0;
int FCP_TARGET_BLOCK_TIME=0;
int FCP_ANYONE_CAN_ADMIN=0;
int FCP_ANYONE_CAN_MINE=0;
int FCP_ANYONE_CAN_CONNECT=0;
int FCP_ANYONE_CAN_SEND=0;
int FCP_ANYONE_CAN_RECEIVE=0;
int FCP_ANYONE_CAN_ACTIVATE=0;
int64_t FCP_MINIMUM_PER_OUTPUT=0;
int FCP_ALLOW_MULTISIG_OUTPUTS=0;
int FCP_ALLOW_P2SH_OUTPUTS=0;
int FCP_WITH_NATIVE_CURRENCY=0;
int FCP_STD_OP_DROP_COUNT=0;
int FCP_STD_OP_DROP_SIZE=0;
int FCP_ANYONE_CAN_RECEIVE_EMPTY=0;

#endif	/* GLOBALS_H */
