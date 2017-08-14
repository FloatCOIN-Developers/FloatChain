// Copyright (c) 2014-2017 Coin Sciences Ltd
// FloatChain code distributed under the GPLv3 license, see COPYING file.

#ifndef FLOATCHAIN_STATE_H
#define	FLOATCHAIN_STATE_H

#include "utils/define.h"
#include "chainparams/params.h"
#include "protocol/floatchainscript.h"
#include "permissions/permission.h"
#include "entities/asset.h"

#define FC_FAT_UNKNOWN     1
#define FC_FAT_COMMAND     2
#define FC_FAT_NETWORK     3
#define FC_FAT_NETWORKSEED 4

#define FC_NTS_UNCONNECTED             0
#define FC_NTS_WAITING_FOR_SEED        1
#define FC_NTS_SEED_READY              2     
#define FC_NTS_NOT_READY               3
#define FC_NTS_SEED_NO_PARAMS          4     

#define FC_NPS_NONE      0x00000000
#define FC_NPS_NETWORK   0x00000001
#define FC_NPS_INCOMING  0x00000002
#define FC_NPS_MINING    0x00000004
#define FC_NPS_ALL       0xFFFFFFFF

#define FC_WMD_NONE                  0x00000000
#define FC_WMD_TXS                   0x00000001
#define FC_WMD_ADDRESS_TXS           0x00000002
#define FC_WMD_MAP_TXS               0x00010000
#define FC_WMD_MODE_MASK             0x00FFFFFF
#define FC_WMD_DEBUG                 0x01000000
#define FC_WMD_AUTOSUBSCRIBE_STREAMS 0x02000000
#define FC_WMD_AUTOSUBSCRIBE_ASSETS  0x04000000
#define FC_WMD_AUTO                  0x10000000


#ifdef	__cplusplus
extern "C" {
#endif

    
typedef struct fc_Params
{    
    int m_NumArguments;
    char** m_Arguments;
    int m_FirstArgumentType;
    char m_DataDirNetSpecific[FC_DCT_DB_MAX_PATH];
    char m_DataDir[FC_DCT_DB_MAX_PATH];
    
    fc_Params()
    {
        InitDefaults();
    }    

    ~fc_Params()
    {
        Destroy();
    }
    
    void  InitDefaults()
    {
        m_NumArguments=0;
        m_Arguments=NULL;
        m_FirstArgumentType=FC_FAT_UNKNOWN;
        m_DataDir[0]=0;
        m_DataDirNetSpecific[0]=0;
    }
    
    void  Destroy()
    {
        if(m_Arguments)
        {
            if(m_Arguments[0])
            {
                fc_Delete(m_Arguments[0]);
            }
            fc_Delete(m_Arguments);
        }
    }
    
    void Parse(int argc, const char* const argv[]);
    int ReadConfig(const char *network_name);
    const char* GetOption(const char* strArg,const char* strDefault);
    int64_t GetOption(const char* strArg,int64_t nDefault);
    int64_t HasOption(const char* strArg);
    
    const char *NetworkName();
    const char *SeedNode();
    const char *Command();
    const char *DataDir();
    const char *DataDir(int network_specific,int create);
    
} fc_Params;

typedef struct fc_Features
{    
    int MinProtocolVersion();
    int ActivatePermission();
    int LastVersionNotSendingProtocolVersionInHandShake();
    int VerifySizeOfOpDropElements();
    int PerEntityPermissions();
    int FollowOnIssues();
    int SpecialParamsInDetailsScript();
    int FixedGrantsInTheSameTx();
    int UnconfirmedMinersCannotMine();
    int Streams();
    int OpDropDetailsScripts();
    int ShortTxIDInTx();
    int CachedInputScript();
    int AnyoneCanReceiveEmpty();
    int FixedIn10007();
    int Upgrades();
    int FixedIn10008();
} fc_Features;

typedef struct fc_BlockHeaderInfo
{    
    unsigned char m_Hash[32];
    int32_t m_NodeId;
    int32_t m_Next;
    
} fc_BlockHeaderInfo;

typedef struct fc_State
{    
    fc_State()
    {
        InitDefaults();
    }    

    ~fc_State()
    {
        Destroy();
    }
    
    fc_Params               *m_Params;
    fc_FloatchainParams     *m_NetworkParams;
    fc_Permissions          *m_Permissions;
    fc_AssetDB              *m_Assets;
    fc_Features             *m_Features;
    int m_NetworkState;
    uint32_t m_NodePausedState;    
    uint32_t m_IPv4Address;
    uint32_t m_WalletMode;
    int m_ProtocolVersionToUpgrade;
    void *m_pSeedNode;
    
    fc_Script               *m_TmpScript;
    fc_Script               *m_TmpScript1;
    fc_Buffer               *m_TmpAssetsOut;
    fc_Buffer               *m_TmpAssetsIn;
    
    fc_Buffer               *m_BlockHeaderSuccessors;
    
    void  InitDefaults()
    {
        m_Params=new fc_Params;     
        m_Features=new fc_Features;
        m_NetworkParams=new fc_FloatchainParams;
        m_Permissions=NULL;
        m_Assets=NULL;
        m_TmpScript=new fc_Script;
        m_TmpScript1=new fc_Script;
        m_NetworkState=FC_NTS_UNCONNECTED;
        m_NodePausedState=FC_NPS_NONE;
        m_ProtocolVersionToUpgrade=0;
        m_IPv4Address=0;
        m_WalletMode=0;
        m_TmpAssetsOut=new fc_Buffer;
        fc_InitABufferMap(m_TmpAssetsOut);
        m_TmpAssetsIn=new fc_Buffer;
        fc_InitABufferMap(m_TmpAssetsIn);
        
        m_BlockHeaderSuccessors=new fc_Buffer;
        m_BlockHeaderSuccessors->Initialize(sizeof(fc_BlockHeaderInfo),sizeof(fc_BlockHeaderInfo),0);            
        fc_BlockHeaderInfo bhi;
        memset(&bhi,0,sizeof(fc_BlockHeaderInfo));
        m_BlockHeaderSuccessors->Add(&bhi);
        
        m_pSeedNode=NULL;
    }
    
    void  Destroy()
    {
        if(m_Params)
        {
            delete m_Params;
        }
        if(m_Features)
        {
            delete m_Features;
        }
        if(m_Permissions)
        {
            delete m_Permissions;
        }
        if(m_Assets)
        {
            delete m_Assets;
        }
        if(m_NetworkParams)
        {
            delete m_NetworkParams;
        }
        if(m_TmpScript)
        {
            delete m_TmpScript;
        }
        if(m_TmpScript1)
        {
            delete m_TmpScript1;
        }
        if(m_TmpAssetsOut)
        {
            delete m_TmpAssetsOut;
        }
        if(m_TmpAssetsIn)
        {
            delete m_TmpAssetsIn;
        }
        if(m_BlockHeaderSuccessors)
        {
            delete m_BlockHeaderSuccessors;
        }
    }
    
    const char* GetVersion();
    const char* GetFullVersion();
    int GetNumericVersion();
    int GetWalletDBVersion();
    int GetProtocolVersion();
    const char* GetSeedNode();
} cs_State;


#ifdef	__cplusplus
}
#endif


extern fc_State* fc_gState;

#endif	/*FLOATCHAIN_STATE_H */

