// Copyright (c) 2014-2017 Coin Sciences Ltd
// FloatChain code distributed under the GPLv3 license, see COPYING file.

#include "floatchain/floatchain.h"


#include "version/version.h"

const char* FC_State::GetVersion()
{
    return FLOATCHAIN_BUILD_DESC;
}

const char* FC_State::GetFullVersion()
{
    return FLOATCHAIN_FULL_VERSION;
}

int FC_State::GetNumericVersion()
{
    return FLOATCHAIN_BUILD_DESC_NUMERIC;
}

int FC_State::GetWalletDBVersion()
{
    if(FC_gState->m_WalletMode & FC_WMD_ADDRESS_TXS)
    {
        if(FC_gState->m_WalletMode & FC_WMD_MAP_TXS)
        {
            return -1;                
        }
        else
        {
            return 2;                
        }
    }
    
    return 1;
}

int FC_State::GetProtocolVersion()
{
    return FLOATCHAIN_PROTOCOL_VERSION;
}
