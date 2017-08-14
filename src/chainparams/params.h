// Copyright (c) 2014-2017 Coin Sciences Ltd
// Copyright (c) 2017 Float Coin Developers
// FloatChain code distributed under the GPLv3 license, see COPYING file.

#ifndef FLOATCHAINPARAMS_H
#define	FLOATCHAINPARAMS_H

#include "utils/declare.h"

#define FC_DEFAULT_NETWORK_PORT 8571
#define FC_DEFAULT_RPC_PORT 8570


#define FC_PRM_MAX_PARAM_NAME_SIZE       31
#define FC_PRM_MAX_ARG_NAME_SIZE         31
#define FC_PRM_PARAM_SIZE_BYTES           2
#define FC_PRM_MAX_DESCRIPTION_SIZE     255
#define FC_PRM_DECIMAL_GRANULARITY 1000000

#define FC_PRM_UNKNOWN          0x00000000
#define FC_PRM_BINARY           0x00000001
#define FC_PRM_STRING           0x00000002
#define FC_PRM_BOOLEAN          0x00000003
#define FC_PRM_INT32            0x00000004
#define FC_PRM_INT64            0x00000005
#define FC_PRM_DOUBLE           0x00000006
#define FC_PRM_UINT32           0x00000007
#define FC_PRM_DATA_TYPE_MASK   0x0000000F

#define FC_PRM_COMMENT          0x00000010
#define FC_PRM_USER             0x00000020
#define FC_PRM_GENERATED        0x00000030
#define FC_PRM_CALCULATED       0x00000040
#define FC_PRM_SOURCE_MASK      0x000000F0

#define FC_PRM_CLONE            0x00010000
#define FC_PRM_SPECIAL          0x00020000
#define FC_PRM_NOHASH           0x00040000
#define FC_PRM_MINIMAL          0x00080000
#define FC_PRM_DECIMAL          0x00100000


#define FC_PRM_STATUS_EMPTY              0
#define FC_PRM_STATUS_MINIMAL            1
#define FC_PRM_STATUS_ERROR              2
#define FC_PRM_STATUS_GENERATED          3
#define FC_PRM_STATUS_INVALID            4
#define FC_PRM_STATUS_VALID              5

extern int FCP_MAX_STD_OP_RETURN_COUNT;
extern int64_t FCP_INITIAL_BLOCK_REWARD;
extern int64_t FCP_FIRST_BLOCK_REWARD;
extern int FCP_TARGET_BLOCK_TIME;
extern int FCP_ANYONE_CAN_ADMIN;
extern int FCP_ANYONE_CAN_MINE;
extern int FCP_ANYONE_CAN_CONNECT;
extern int FCP_ANYONE_CAN_SEND;
extern int FCP_ANYONE_CAN_RECEIVE;
extern int FCP_ANYONE_CAN_ACTIVATE;
extern int64_t FCP_MINIMUM_PER_OUTPUT;
extern int FCP_ALLOW_MULTISIG_OUTPUTS;
extern int FCP_ALLOW_P2SH_OUTPUTS;
extern int FCP_WITH_NATIVE_CURRENCY;
extern int FCP_STD_OP_DROP_COUNT;
extern int FCP_STD_OP_DROP_SIZE;
extern int FCP_ANYONE_CAN_RECEIVE_EMPTY;

typedef struct FC_OneFloatchainParam
{    
    char m_Name[FC_PRM_MAX_ARG_NAME_SIZE+1]; 
    char m_DisplayName[FC_PRM_MAX_ARG_NAME_SIZE+1]; 
    int m_Type;
    int m_MaxStringSize;
    int64_t m_DefaultIntegerValue;
    int64_t m_MinIntegerValue;
    int64_t m_MaxIntegerValue;
    double m_DefaultDoubleValue;
    int m_ProtocolVersion;
    int m_Removed;
    char m_ArgName[FC_PRM_MAX_PARAM_NAME_SIZE+1]; 
    char m_Next[FC_PRM_MAX_ARG_NAME_SIZE+1]; 
    char m_Group[FC_PRM_MAX_DESCRIPTION_SIZE+1]; 
    char m_Description[FC_PRM_MAX_DESCRIPTION_SIZE+1]; 
    
    int IsRelevant(int version);
} FC_OneFloatchainParam;

typedef struct FC_FloatchainParams
{    
    char *m_lpData;
    FC_MapStringIndex *m_lpIndex;
    FC_OneFloatchainParam *m_lpParams;
    int *m_lpCoord;
    int m_Status;
    int m_Size;
    int m_Count;
    int m_IsProtocolFloatChain;
    int m_ProtocolVersion;
    
    int m_AssetRefSize;
    
    FC_FloatchainParams()
    {
        Zero();
    }

    ~FC_FloatchainParams()
    {
        Destroy();
    }
    
    void Zero();                                                                // Initializes parameters set object
    void Init();                                                                // Initializes parameters set object
    void Destroy();                                                             // Destroys parameters set object
    
    int Create(const char *name,int version);
    int Read(const char *name);    
    int Read(const char* name,int argc, char* argv[],int create_version);
    int Clone(const char *name,FC_FloatchainParams *source);
    int Build(const unsigned char* pubkey,int pubkey_size);
    int Validate();
    int CalculateHash(unsigned char *hash);
    int Write(int overwrite);
    int Print(FILE *);
    int SetGlobals();
    int Import(const char *name,const char *source_address);
    int Set(const char *name,const char *source,int source_size);
    
    int FindParam(const char *param);
    void* GetParam(const char *param,int* size);
    int64_t GetInt64Param(const char *param);
    double GetDoubleParam(const char *param);
    
    int SetParam(const char *param,const char* value,int size);
    int SetParam(const char *param,int64_t value);
    
    
    const char* Name();
    const unsigned char* DefaultMessageStart();
    const unsigned char* MessageStart();
    const unsigned char* AddressVersion();
    const unsigned char* AddressCheckumValue();
    const unsigned char* AddressScriptVersion();
    int ProtocolVersion();
    int IsProtocolFloatchain();
    double ParamAccuracy();
    

    
} FC_FloatchainParams;


#endif	/* FLOATCHAINPARAMS_H */

