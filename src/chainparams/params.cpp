// Copyright (c) 2014-2017 Coin Sciences Ltd
// Copyright (c) 2017 Float Coin Developers
// FloatChain code distributed under the GPLv3 license, see COPYING file.

#include "floatchain/floatchain.h"


#define FC_PRM_DAT_FILE_LINE_SIZE 39

const uint32_t FreePortRangesOver50[]={2644,2744,2870,4244,4324,4374,4754,5744,6264,6446,
                                       6716,6790,7172,7314,7404,7718,8338,9218,9538,9696};

const unsigned char c_DefaultMessageStart[4]={0xfb,0xb4,0xc7,0xde};

#include "chainparams/paramlist.h"


int fc_OneFloatchainParam::IsRelevant(int version)
{
    int ret=1;
    
    if(m_ProtocolVersion > version)
    {
        ret=0;
    }
    
    if(m_Removed > 0)
    {
        if(m_Removed <= version)
        {
            ret=0;
        }
    }
    
    return ret;
}

void fc_FloatchainParams::Zero()
{
    m_lpData = NULL;
    m_lpParams = NULL;        
    m_lpIndex=NULL;    
    m_lpCoord=NULL;
    m_Status=FC_PRM_STATUS_EMPTY;
    m_Size=0;
    m_IsProtocolFloatChain=1;
    m_ProtocolVersion=0;
    
    m_AssetRefSize=FC_AST_SHORT_TXID_SIZE;
}

void fc_FloatchainParams::Destroy()
{
    if(m_lpData)
    {
        fc_Delete(m_lpData);
        m_lpData = NULL;
    }
    if(m_lpParams)
    {
        fc_Delete(m_lpParams);
        m_lpParams = NULL;        
    }
    if(m_lpCoord)
    {
        fc_Delete(m_lpCoord);
        m_lpCoord = NULL;        
    }
    if(m_lpIndex)
    {
        delete m_lpIndex;
        m_lpIndex=NULL;
    }    
}


int64_t fc_FloatchainParams::GetInt64Param(const char *param)
{
    int size;
    void* ptr=GetParam(param,&size);
    if(ptr == NULL)
    {
        if(m_lpIndex == NULL)
        {
            return -1;
        }
        int index=m_lpIndex->Get(param);
        if(index<0)
        {
            printf("Parameter not found: %s\n",param);
            return -1;
        }   
        
        return (m_lpParams+index)->m_DefaultIntegerValue;
    }
   
    return fc_GetLE(ptr,size);
}

double fc_FloatchainParams::GetDoubleParam(const char *param)
{
    int n=(int)fc_gState->m_NetworkParams->GetInt64Param(param);
    if(n < 0)
    {
        return -((double)(-n) / FC_PRM_DECIMAL_GRANULARITY);
    }
    return (double)n / FC_PRM_DECIMAL_GRANULARITY;    
}


void* fc_FloatchainParams::GetParam(const char *param,int* size)
{
    if(m_lpIndex == NULL)
    {
        return NULL;
    }
    int index=m_lpIndex->Get(param);
    if(index<0)
    {
        return NULL;
    }   
    int offset=m_lpCoord[2 * index + 0];
    if(offset<0)
    {
        return NULL;
    }   
    if(size)
    {
        *size=m_lpCoord[2 * index + 1];
    }
    
    return m_lpData+offset;
}


int fc_FloatchainParams::SetParam(const char *param,const char* value,int size)
{
    int offset;
    if(m_lpIndex == NULL)
    {
        return FC_ERR_INTERNAL_ERROR;
    }
    int index=m_lpIndex->Get(param);
    if(index<0)
    {
        return FC_ERR_INTERNAL_ERROR;
    }   
    
    switch((m_lpParams+index)->m_Type & FC_PRM_SOURCE_MASK)
    {
        case FC_PRM_COMMENT:
        case FC_PRM_CALCULATED:
            break;
        default:
            return FC_ERR_INTERNAL_ERROR;
    }
    
    if(m_lpCoord[2 * index + 0] >= 0)
    {
        return FC_ERR_INTERNAL_ERROR;        
    }
    
    offset=m_Size;
    
    strcpy(m_lpData+offset,param);
    offset+=strlen(param)+1;

    if(size)
    {
        mefcpy(m_lpData+offset+FC_PRM_PARAM_SIZE_BYTES,value,size);
    }
    
    fc_PutLE(m_lpData+offset,&size,FC_PRM_PARAM_SIZE_BYTES);
    offset+=FC_PRM_PARAM_SIZE_BYTES;
    m_lpCoord[2 * index + 0]=offset;
    m_lpCoord[2 * index + 1]=size;
    offset+=size;                        
    
    m_Size=offset;
    
    return FC_ERR_NOERROR;    
}

int fc_FloatchainParams::SetParam(const char *param,int64_t value)
{
    int size;
    char buf[8];
    if(m_lpIndex == NULL)
    {
        return FC_ERR_INTERNAL_ERROR;
    }
    int index=m_lpIndex->Get(param);
    if(index<0)
    {
        return FC_ERR_INTERNAL_ERROR;
    }   
    
    size=0;
    switch((m_lpParams+index)->m_Type & FC_PRM_DATA_TYPE_MASK)
    {
        case FC_PRM_BOOLEAN:
            size=1;
            break;
        case FC_PRM_INT32:
        case FC_PRM_UINT32:            
            size=4;
            break;
        case FC_PRM_INT64:    
            size=8;
            break;
        default:
            return FC_ERR_INTERNAL_ERROR;
    }

    fc_PutLE(buf,&value,size);
    return SetParam(param,buf,size);
}


void fc_FloatchainParams::Init()
{
    int size,max_size,i;
    
    Destroy();
    
    m_lpIndex=new fc_MapStringIndex;            
    
    m_Count=sizeof(FloatchainParamArray)/sizeof(fc_OneFloatchainParam);
    max_size=0;
    
    for(i=0;i<m_Count;i++)
    {
        size=0;
        switch((FloatchainParamArray+i)->m_Type & FC_PRM_DATA_TYPE_MASK)
        {
            case FC_PRM_BINARY  : size=(FloatchainParamArray+i)->m_MaxStringSize;   break;
            case FC_PRM_STRING  : size=(FloatchainParamArray+i)->m_MaxStringSize+1; break;
            case FC_PRM_BOOLEAN : size=1;                                           break;
            case FC_PRM_INT32   : size=4;                                           break;
            case FC_PRM_INT64   : size=8;                                           break;
            case FC_PRM_DOUBLE  : size=sizeof(double);                              break;
            case FC_PRM_UINT32  : size=4;                                           break;
        }
                
        max_size+=FC_PRM_MAX_PARAM_NAME_SIZE+1+FC_PRM_PARAM_SIZE_BYTES+size;
    }
    
    m_lpParams=(fc_OneFloatchainParam*)fc_New(sizeof(FloatchainParamArray));
    m_lpData=(char*)fc_New(max_size);
    m_lpCoord=(int*)fc_New(2*m_Count*sizeof(int));
    
    mefcpy(m_lpParams,FloatchainParamArray,sizeof(FloatchainParamArray));
    for(i=0;i<m_Count;i++)
    {
        m_lpIndex->Add((m_lpParams+i)->m_Name,i);
        m_lpCoord[2 * i + 0]=-1;
    }    
    m_Size=0;
    
}

int fc_FloatchainParams::Create(const char* name,int version)
{
    int size,offset,i,set;
    fc_OneFloatchainParam *param;
    char *ptrData;
    int num_sets;
    uint32_t network_port=FC_DEFAULT_NETWORK_PORT;
    uint32_t rpc_port=FC_DEFAULT_RPC_PORT;
    
    int param_sets[]={FC_PRM_COMMENT, FC_PRM_USER, FC_PRM_GENERATED};
    num_sets=sizeof(param_sets)/sizeof(int);    
    
    Init();
 
    fc_RandomSeed(fc_TimeNowAsUInt());
    
    offset=0;

    for(set=0;set<num_sets;set++)
    {    
        param=m_lpParams;
        for(i=0;i<m_Count;i++)
        {
            if(((m_lpParams+i)->m_Type & FC_PRM_SOURCE_MASK) == param_sets[set])
            {                                                                
                strcpy(m_lpData+offset,param->m_Name);
                offset+=strlen(param->m_Name)+1;
                size=0;

                ptrData=m_lpData+offset+FC_PRM_PARAM_SIZE_BYTES;

                switch(param->m_Type & FC_PRM_SOURCE_MASK)
                {
                    case FC_PRM_COMMENT:
                    case FC_PRM_USER:
                        switch((FloatchainParamArray+i)->m_Type & FC_PRM_DATA_TYPE_MASK)
                        {
                            case FC_PRM_BINARY  : 
                                size=0;   
                                break;
                            case FC_PRM_STRING  : 
                                size=1;   
                                if((FloatchainParamArray+i)->m_Type & FC_PRM_SPECIAL)
                                {
                                    if(strcmp(param->m_Name,"chaindescription") == 0)
                                    {
                                        if(strlen(name)+19<=(size_t)(param->m_MaxStringSize))
                                        {
                                            sprintf(ptrData,"FloatChain %s",name);
                                        }
                                        size=strlen(ptrData)+1;
                                    }                                   
                                    if(strcmp(param->m_Name,"rootstreamname") == 0)
                                    {
                                        sprintf(ptrData,"root");
                                        size=strlen(ptrData)+1;
                                    }                                   
                                    if(strcmp(param->m_Name,"seednode") == 0)
                                    {
                                        size=1;
                                    }                                   
                                    if(strcmp(param->m_Name,"chainprotocol") == 0)
                                    {
                                        sprintf(ptrData,"floatchain");
                                        size=strlen(ptrData)+1;
                                    }                                   
                                }
                                break;
                            case FC_PRM_BOOLEAN:
                                size=1;
                                ptrData[0]=0;
                                if(param->m_DefaultIntegerValue)
                                {
                                    ptrData[0]=1;                            
                                }
                                break;
                            case FC_PRM_INT32:
                            case FC_PRM_UINT32:
                                size=4;
                                fc_PutLE(ptrData,&(param->m_DefaultIntegerValue),4);
                                break;
                            case FC_PRM_INT64:
                                size=8;
                                fc_PutLE(ptrData,&(param->m_DefaultIntegerValue),8);
                                break;
                            case FC_PRM_DOUBLE:
                                size=8;
                                *((double*)ptrData)=param->m_DefaultDoubleValue;
                                break;
                        }            
                        break;
                    case FC_PRM_GENERATED:
                        if(strcmp(param->m_Name,"defaultnetworkport") == 0)
                        {
                            size=4;
                            network_port=fc_RandomInRange(0,sizeof(FreePortRangesOver50)/sizeof(uint32_t)-1);
                            network_port=FreePortRangesOver50[network_port];
                            network_port+=1+2*fc_RandomInRange(0,24);
                            fc_PutLE(ptrData,&network_port,4);
                        }
                        if(strcmp(param->m_Name,"defaultrpcport") == 0)
                        {
                            size=4;
                            rpc_port=network_port-1;
                            fc_PutLE(ptrData,&rpc_port,4);
                        }
                        if(strcmp(param->m_Name,"protocolversion") == 0)
                        {
                            size=4;
                            fc_PutLE(ptrData,&version,4);
                        }
                        if(strcmp(param->m_Name,"chainname") == 0)
                        {
                            if(strlen(name)>(size_t)(param->m_MaxStringSize))
                            {
                                fc_print("Invalid network name - too long");
                                return FC_ERR_INVALID_PARAMETER_VALUE;
                            }
                            size=strlen(name)+1;
                            strcpy(ptrData,name);
                        }
                        if(strcmp(param->m_Name,"networkmessagestart") == 0)
                        {
                            size=4;
                            mefcpy(ptrData,c_DefaultMessageStart,4);
                            while(mefcmp(ptrData,c_DefaultMessageStart,4) == 0)
                            {
                                *((unsigned char*)ptrData+0)=fc_RandomInRange(0xf0,0xff);
                                *((unsigned char*)ptrData+1)=fc_RandomInRange(0xc0,0xff);
                                *((unsigned char*)ptrData+2)=fc_RandomInRange(0xc0,0xff);
                                *((unsigned char*)ptrData+3)=fc_RandomInRange(0xe0,0xff);
                            }
                        }
                        if(strcmp(param->m_Name,"addresspubkeyhashversion") == 0)
                        {
                            size=4;
                            *((unsigned char*)ptrData+0)=0x00;
                            *((unsigned char*)ptrData+1)=fc_RandomInRange(0x00,0xff);
                            *((unsigned char*)ptrData+2)=fc_RandomInRange(0x00,0xff);
                            *((unsigned char*)ptrData+3)=fc_RandomInRange(0x00,0xff);
                        }
                        if(strcmp(param->m_Name,"addressscripthashversion") == 0)
                        {
                            size=4;
                            *((unsigned char*)ptrData+0)=0x05;
                            *((unsigned char*)ptrData+1)=fc_RandomInRange(0x00,0xff);
                            *((unsigned char*)ptrData+2)=fc_RandomInRange(0x00,0xff);
                            *((unsigned char*)ptrData+3)=fc_RandomInRange(0x00,0xff);
                        }
                        if(strcmp(param->m_Name,"privatekeyversion") == 0)
                        {
                            size=4;
                            *((unsigned char*)ptrData+0)=0x80;
                            *((unsigned char*)ptrData+1)=fc_RandomInRange(0x00,0xff);
                            *((unsigned char*)ptrData+2)=fc_RandomInRange(0x00,0xff);
                            *((unsigned char*)ptrData+3)=fc_RandomInRange(0x00,0xff);
                        }
                        if(strcmp(param->m_Name,"addresschecksumvalue") == 0)
                        {
                            size=4;
                            *((unsigned char*)ptrData+0)=fc_RandomInRange(0x00,0xff);
                            *((unsigned char*)ptrData+1)=fc_RandomInRange(0x00,0xff);
                            *((unsigned char*)ptrData+2)=fc_RandomInRange(0x00,0xff);
                            *((unsigned char*)ptrData+3)=fc_RandomInRange(0x00,0xff);
                        }
                        break;
                }

                fc_PutLE(m_lpData+offset,&size,FC_PRM_PARAM_SIZE_BYTES);
                offset+=FC_PRM_PARAM_SIZE_BYTES;
                m_lpCoord[2 * i + 0]=offset;
                m_lpCoord[2 * i + 1]=size;
                offset+=size;            
            }
            param++;
        }
        
    }
        
    m_Size=offset;
    
    return FC_ERR_NOERROR;
}

double fc_FloatchainParams::ParamAccuracy()
{
    return 1./(double)FC_PRM_DECIMAL_GRANULARITY;
}


int fc_FloatchainParams::Read(const char* name)
{
    return Read(name,0,NULL,0);
}

int fc_FloatchainParams::Read(const char* name,int argc, char* argv[],int create_version)
{
    fc_MapStringString *mapConfig;
    int err;
    int size,offset,i,version,len0,len1,len2;
    fc_OneFloatchainParam *param;
    char *ptrData;
    const char *ptr;
    
    if(name == NULL)
    {
        return FC_ERR_INTERNAL_ERROR;
    }
    
    err=FC_ERR_NOERROR;
    offset=0;
    
    fc_FloatchainParams *lpDefaultParams;
    
    lpDefaultParams=NULL;
    mapConfig=new fc_MapStringString;
    if(argc)
    {
        err=fc_ReadParamArgs(mapConfig,argc,argv,"");
    }
    else
    {
        err=fc_ReadGeneralConfigFile(mapConfig,name,"params",".dat");
    }

    if(err)
    {
        goto exitlbl;
    }    
    
    Init();

    version=0;
    if(mapConfig->Get("protocolversion") != NULL)
    {
        version=atoi(mapConfig->Get("protocolversion"));
    }
    if(create_version)
    {
        version=create_version;
    }
    
    if(version == 0)
    {
        version=fc_gState->GetProtocolVersion();
    }

    if(argc == 0)
    {
        if(mapConfig->Get("chainname") == NULL)
        {
            err=FC_ERR_NOERROR;
            goto exitlbl;
        }


        if(strcmp(name,mapConfig->Get("chainname")) != 0)
        {
            printf("chain-name parameter (%s) doesn't match <network-name> (%s)\n",mapConfig->Get("chainname"),name);
            err=FC_ERR_INVALID_PARAMETER_VALUE;                        
            goto exitlbl;
        }
    }

    lpDefaultParams = new fc_FloatchainParams;
    
    err=lpDefaultParams->Create(name,version);
 
    if(err)
    {
        goto exitlbl;
    }
    

    param=m_lpParams;
    for(i=0;i<m_Count;i++)
    {
        ptr=mapConfig->Get(param->m_Name);
        if(ptr)
        {
            if(strcmp(ptr,"[null]") == 0)
            {
                ptr=NULL;;
            }
        }        
        if(ptr)
        {        
            strcpy(m_lpData+offset,param->m_Name);
            offset+=strlen(param->m_Name)+1;
            size=0;

            ptrData=m_lpData+offset+FC_PRM_PARAM_SIZE_BYTES;
            
            switch((FloatchainParamArray+i)->m_Type & FC_PRM_DATA_TYPE_MASK)
            {
                case FC_PRM_BINARY  : 
                    if(strlen(ptr) % 2)
                    {
                        printf("Invalid parameter value for %s - odd length: %d\n",param->m_DisplayName,(int)strlen(ptr));
                        return FC_ERR_INVALID_PARAMETER_VALUE;                        
                    }
                    size=strlen(ptr) / 2;   
                    if(size > param->m_MaxStringSize)
                    {
                        printf("Invalid parameter value for %s - too long: %d\n",param->m_DisplayName,(int)strlen(ptr));
                        return FC_ERR_INVALID_PARAMETER_VALUE;                                                
                    }
                    if(fc_HexToBin(ptrData,ptr,size) != size)
                    {
                        printf("Invalid parameter value for %s - cannot parse hex string: %s\n",param->m_DisplayName,ptr);
                        return FC_ERR_INVALID_PARAMETER_VALUE;                                                
                    }
                    break;
                case FC_PRM_STRING  : 
                    size=strlen(ptr);
                    if(size > param->m_MaxStringSize)
                    {
                        printf("Invalid parameter value for %s - too long: %d\n",param->m_DisplayName,(int)strlen(ptr));
                        return FC_ERR_INVALID_PARAMETER_VALUE;                                                
                    }
                    strcpy(ptrData,ptr);
                    size++;
                    break;
                case FC_PRM_BOOLEAN:
                    ptrData[0]=0;
                    if(strcasecmp(ptr,"true") == 0)
                    {
                        ptrData[0]=1;    
                    }
                    else
                    {
                        ptrData[0]=atoi(ptr);
                    }
                    size=1;
                    break;
                case FC_PRM_INT32:
                    size=4;
                    if(((FloatchainParamArray+i)->m_Type & FC_PRM_SOURCE_MASK) == FC_PRM_USER)
                    {
                        if(atoll(ptr) > (FloatchainParamArray+i)->m_MaxIntegerValue)
                        {
                            printf("Invalid parameter value for %s - too high: %s\n",param->m_DisplayName,ptr);                        
                            return FC_ERR_INVALID_PARAMETER_VALUE;                                                
                        }
                        if(atoll(ptr) < (FloatchainParamArray+i)->m_MinIntegerValue)
                        {
                            printf("Invalid parameter value for %s - too low: %s\n",param->m_DisplayName,ptr);                        
                            return FC_ERR_INVALID_PARAMETER_VALUE;                                                
                        }
                    }
                    if((FloatchainParamArray+i)->m_Type & FC_PRM_DECIMAL)
                    {
                        double d=atof(ptr);
                        if(d >= 0)
                        {
                            *(int32_t*)ptrData=(int32_t)(d*FC_PRM_DECIMAL_GRANULARITY+ParamAccuracy());                            
                        }
                        else
                        {
                            *(int32_t*)ptrData=-(int32_t)(-d*FC_PRM_DECIMAL_GRANULARITY+ParamAccuracy());                                                        
                        }
                    }
                    else
                    {
                        *(int32_t*)ptrData=(int32_t)atol(ptr);
                    }
                    break;
                case FC_PRM_UINT32:
                    size=4;
                    if(((FloatchainParamArray+i)->m_Type & FC_PRM_SOURCE_MASK) == FC_PRM_USER)
                    {
                        if(atoll(ptr) > (FloatchainParamArray+i)->m_MaxIntegerValue)
                        {
                            printf("Invalid parameter value for %s - too high: %s\n",param->m_DisplayName,ptr);                        
                            return FC_ERR_INVALID_PARAMETER_VALUE;                                                
                        }
                        if(atoll(ptr) < (FloatchainParamArray+i)->m_MinIntegerValue)
                        {
                            printf("Invalid parameter value for %s - too low: %s\n",param->m_DisplayName,ptr);                        
                            return FC_ERR_INVALID_PARAMETER_VALUE;                                                
                        }
                    }
                    if((FloatchainParamArray+i)->m_Type & FC_PRM_DECIMAL)
                    {
                        *(int32_t*)ptrData=(int32_t)(atof(ptr)*FC_PRM_DECIMAL_GRANULARITY+ParamAccuracy());
                    }
                    else
                    {
                        *(int32_t*)ptrData=(int32_t)atol(ptr);
                    }
                    if(ptr[0]=='-')
                    {
                        printf("Invalid parameter value for %s - should be non-negative\n",param->m_DisplayName);
                        return FC_ERR_INVALID_PARAMETER_VALUE;                                                                        
                    }
                    break;
                case FC_PRM_INT64:
                    if(((FloatchainParamArray+i)->m_Type & FC_PRM_SOURCE_MASK) == FC_PRM_USER)
                    {
                        if(atoll(ptr) > (FloatchainParamArray+i)->m_MaxIntegerValue)
                        {
                            printf("Invalid parameter value for %s - too high: %s\n",param->m_DisplayName,ptr);                        
                            return FC_ERR_INVALID_PARAMETER_VALUE;                                                
                        }
                        if(atoll(ptr) < (FloatchainParamArray+i)->m_MinIntegerValue)
                        {
                            printf("Invalid parameter value for %s - too low: %s\n",param->m_DisplayName,ptr);                        
                            return FC_ERR_INVALID_PARAMETER_VALUE;                                                
                        }
                    }
                    size=8;
                    *(int64_t*)ptrData=(int64_t)atoll(ptr);
                    break;
                case FC_PRM_DOUBLE:
                    size=8;
                    *((double*)ptrData)=atof(ptr);
                    break;
            }            
            
            fc_PutLE(m_lpData+offset,&size,FC_PRM_PARAM_SIZE_BYTES);
            offset+=FC_PRM_PARAM_SIZE_BYTES;
            m_lpCoord[2 * i + 0]=offset;
            m_lpCoord[2 * i + 1]=size;
            offset+=size;            
        }
        else
        {
            if( ((((FloatchainParamArray+i)->m_Type & FC_PRM_SOURCE_MASK) != FC_PRM_CALCULATED) && 
               (((FloatchainParamArray+i)->m_Type & FC_PRM_SOURCE_MASK) != FC_PRM_GENERATED)) ||
               (argc > 0) )     
            {
                strcpy(m_lpData+offset,param->m_Name);
                offset+=strlen(param->m_Name)+1;
                size=0;

                ptrData=m_lpData+offset+FC_PRM_PARAM_SIZE_BYTES;

                ptr=(char*)lpDefaultParams->GetParam(param->m_Name,&size);
                if(size)
                {
                    mefcpy(ptrData,ptr,size);
                }            
                fc_PutLE(m_lpData+offset,&size,FC_PRM_PARAM_SIZE_BYTES);
                offset+=FC_PRM_PARAM_SIZE_BYTES;
                m_lpCoord[2 * i + 0]=offset;
                m_lpCoord[2 * i + 1]=size;
                offset+=size;                        
            }
        }

        param++;
    }

exitlbl:
                    
    m_Size=offset;

    delete mapConfig;
    if(lpDefaultParams)
    {
        delete lpDefaultParams;
    }
    if(err == FC_ERR_NOERROR)
    {
        len0=0;
        GetParam("addresspubkeyhashversion",&len0);
        len1=0;
        GetParam("addressscripthashversion",&len1);
        len2=0;
        GetParam("privatekeyversion",&len2);
        if(len1)                                                                // If params.dat is complete
        {
            if( (len0 != len1) || (len0 != len2) )            
            {
                printf("address-pubkeyhash-version, address-scripthash-version and private-key-version should have identical length \n");
                return FC_ERR_INVALID_PARAMETER_VALUE;                                                                                    
            }
        }               
    }
    
    return err;
}

int fc_FloatchainParams::Set(const char *name,const char *source,int source_size)
{
    int size,offset,i,j,n;
    char *ptrData;
        
    Init();
    offset=0;

    j=0;
    while((j < source_size) && (*(source+j)!=0x00))
    {
        n=j;
        i=m_lpIndex->Get(source+j);
        j+=strlen(source+j)+1;
        if(j+FC_PRM_PARAM_SIZE_BYTES <= source_size)
        {
            size=fc_GetLE((void*)(source+j),FC_PRM_PARAM_SIZE_BYTES);
        }
        j+=FC_PRM_PARAM_SIZE_BYTES;
        if(j+size <= source_size)
        {
            if(i >= 0)
            {
                if(m_lpCoord[2 * i + 0] < 0)
                {
                    strcpy(m_lpData+offset,source+n);
                    offset+=strlen(source+n)+1;

                    ptrData=m_lpData+offset+FC_PRM_PARAM_SIZE_BYTES;
                    if(size>0)
                    {
                        mefcpy(ptrData,source+j,size);
                    }
                    fc_PutLE(m_lpData+offset,&size,FC_PRM_PARAM_SIZE_BYTES);
                    offset+=FC_PRM_PARAM_SIZE_BYTES;
                    m_lpCoord[2 * i + 0]=offset;
                    m_lpCoord[2 * i + 1]=size;
                    offset+=size;                                                    
                }
            }
            j+=size;
        }
    }
    
    m_Size=offset;
    return FC_ERR_NOERROR;    
}

int fc_FloatchainParams::Clone(const char* name, fc_FloatchainParams* source)
{
    int err;
    int size,offset,i,version;
    fc_OneFloatchainParam *param;
    char *ptrData;
    void *ptr;
    
    version=source->ProtocolVersion();
    if(version == 0)
    {
        version=fc_gState->GetProtocolVersion();
    }
    
    fc_FloatchainParams *lpDefaultParams;
    lpDefaultParams = new fc_FloatchainParams;
    
    err=lpDefaultParams->Create(name,version);
 
    if(err)
    {
        delete lpDefaultParams;
        return  err;
    }
    
    Init();
    offset=0;

    param=m_lpParams;
    for(i=0;i<m_Count;i++)
    {
        ptr=NULL;
        if(param->m_Type & FC_PRM_CLONE)
        {
            ptr=source->GetParam(param->m_Name,&size);
        }
        if(ptr == NULL)
        {
            ptr=lpDefaultParams->GetParam(param->m_Name,&size);
        }
        if(ptr)
        {            
            strcpy(m_lpData+offset,param->m_Name);
            offset+=strlen(param->m_Name)+1;

            ptrData=m_lpData+offset+FC_PRM_PARAM_SIZE_BYTES;
            if(size)
            {
                mefcpy(ptrData,ptr,size);
            }
            fc_PutLE(m_lpData+offset,&size,FC_PRM_PARAM_SIZE_BYTES);
            offset+=FC_PRM_PARAM_SIZE_BYTES;
            m_lpCoord[2 * i + 0]=offset;
            m_lpCoord[2 * i + 1]=size;
            offset+=size;                        
        }
        param++;            
    }
    
    m_Size=offset;
    delete lpDefaultParams;
    
    return FC_ERR_NOERROR;
}

int fc_FloatchainParams::CalculateHash(unsigned char *hash)
{
    int i;
    int take_it;
    fc_SHA256 *hasher;
    
    hasher=new fc_SHA256;
    for(i=0;i<m_Count;i++)
    {
        take_it=1;
        if(((m_lpParams+i)->m_Type & FC_PRM_SOURCE_MASK) == FC_PRM_COMMENT)
        {
            take_it=0;
        }
        if((m_lpParams+i)->m_Type & FC_PRM_NOHASH)
        {
            take_it=0;            
        }
        if((m_lpParams+i)->m_Removed > 0)
        {
            if((m_lpParams+i)->m_Removed <= (int)GetInt64Param("protocolversion"))
            {
                take_it=0;
            }
        }
        if((m_lpParams+i)->IsRelevant((int)GetInt64Param("protocolversion")) == 0)
        {
            take_it=0;            
            switch((m_lpParams+i)->m_Type & FC_PRM_DATA_TYPE_MASK)
            {
                case FC_PRM_BOOLEAN:
                case FC_PRM_INT32:
                case FC_PRM_UINT32:
                case FC_PRM_INT64:
                    if(GetInt64Param((m_lpParams+i)->m_Name) != (m_lpParams+i)->m_DefaultIntegerValue)
                    {
                        take_it=1;
                    }
                    break;
                case FC_PRM_STRING:
                    take_it=1;
                    
                    if(strcmp((m_lpParams+i)->m_Name,"rootstreamname") == 0)
                    {
                        if(strcmp((char*)GetParam((m_lpParams+i)->m_Name,NULL),"root") == 0)
                        {
                            take_it=0;
                        }
                    }                                   
                    break;
                case FC_PRM_BINARY:
                    take_it=1;
                    
                    if(strcmp((m_lpParams+i)->m_Name,"genesisopreturnscript") == 0)
                    {
                        take_it=0;
                    }                  
                    break;
                default:
                    take_it=1;                                                  // Not supported
                    break;
            }
        }
        if(take_it)
        {
            if(m_lpCoord[2* i + 1] > 0)
            {
                hasher->Write(m_lpData+m_lpCoord[2* i + 0],m_lpCoord[2* i + 1]);
            }
        }
    }
    
    hasher->GetHash(hash);
    hasher->Reset();
    hasher->Write(hash,32);
    hasher->GetHash(hash);
    delete hasher;
    
    return FC_ERR_NOERROR;
}


int fc_FloatchainParams::Validate()
{
    int i,size,offset;
    int isGenerated;
    int isMinimal;
    int isValid;
    unsigned char hash[32];
    char *ptrData;
    void *stored_hash;
    void *protocol_name;
    double dv;
    int64_t iv;
    
    m_Status=FC_PRM_STATUS_EMPTY;

    if((m_lpParams == NULL) || (m_Size == 0))
    {
        return FC_ERR_NOERROR;
    }
        
    isGenerated=1;
    isMinimal=1;
    isValid=1;
    
    offset=m_Size;
    
    for(i=0;i<m_Count;i++)
    {
        if(m_lpCoord[2* i + 0] < 0)
        {
            switch((m_lpParams+i)->m_Type & FC_PRM_SOURCE_MASK)
            {
                case FC_PRM_COMMENT:                                            break;
                case FC_PRM_USER: 
                    if((m_lpParams+i)->IsRelevant((int)GetInt64Param("protocolversion")) == 0)
                    {
                        switch((m_lpParams+i)->m_Type & FC_PRM_DATA_TYPE_MASK)
                        {
                            case FC_PRM_BOOLEAN:
                            case FC_PRM_INT32:
                            case FC_PRM_UINT32:
                            case FC_PRM_INT64:
                            case FC_PRM_DOUBLE:
                            case FC_PRM_STRING:

                                strcpy(m_lpData+offset,(m_lpParams+i)->m_Name);
                                offset+=strlen((m_lpParams+i)->m_Name)+1;
                                size=0;

                                ptrData=m_lpData+offset+FC_PRM_PARAM_SIZE_BYTES;
                                
                                switch((m_lpParams+i)->m_Type & FC_PRM_DATA_TYPE_MASK)
                                {
                                    case FC_PRM_STRING:   
                                        if(strcmp((m_lpParams+i)->m_Name,"rootstreamname") == 0)
                                        {
                                            sprintf(ptrData,"root");
                                            size=strlen(ptrData)+1;
                                        }                                   
                                        break;
                                    case FC_PRM_BOOLEAN:
                                        size=1;
                                        if((m_lpParams+i)->m_DefaultIntegerValue)
                                        {
                                            ptrData[0]=1;
                                        }
                                        else
                                        {
                                            ptrData[0]=0;                                            
                                        }
                                        break;
                                    case FC_PRM_INT64:
                                        size=8;
                                        fc_PutLE(ptrData,&((m_lpParams+i)->m_DefaultIntegerValue),8);
                                        break;
                                    case FC_PRM_DOUBLE:
                                        size=8;
                                        *((double*)ptrData)=(m_lpParams+i)->m_DefaultDoubleValue;
                                        break;
                                    default:
                                        size=4;
                                        fc_PutLE(ptrData,&((m_lpParams+i)->m_DefaultIntegerValue),4);
                                        break;
                                }
                                
                                fc_PutLE(m_lpData+offset,&size,FC_PRM_PARAM_SIZE_BYTES);
                                offset+=FC_PRM_PARAM_SIZE_BYTES;
                                m_lpCoord[2 * i + 0]=offset;
                                m_lpCoord[2 * i + 1]=size;
                                offset+=size;            

                                break;
                            default:                                            // Not supported
                                isGenerated=0; 
                                isValid=0;
                                break;
                        }
                    }
                    else
                    {
                        isGenerated=0; 
                        isValid=0;
                    }
                    break;
                case FC_PRM_GENERATED: 
                    isGenerated=0; 
                    isValid=0;         
                    break;
                case FC_PRM_CALCULATED: 
                    if((m_lpParams+i)->IsRelevant((int)GetInt64Param("protocolversion")))
                    {                    
                        isValid=0;                       
                    }
                    break;
            }
            if((m_lpParams+i)->m_Type & FC_PRM_MINIMAL)
            {
                isMinimal=0;
            }
        }
    }   

    m_Size=offset;
    
    if(isValid)
    {
        m_Status=FC_PRM_STATUS_VALID;

        CalculateHash(hash);
        
        stored_hash=GetParam("chainparamshash",&size);
        if((stored_hash == NULL) || (size != 32))
        {
            m_Status=FC_PRM_STATUS_INVALID;
        }
        else
        {
            protocol_name=GetParam("chainprotocol",NULL);
            if(strcmp((char*)protocol_name,"floatchain") == 0)
            {
                if(mefcmp(hash,stored_hash,32))
                {
                    m_Status=FC_PRM_STATUS_INVALID;                
                }
            }
        }
    }
    else
    {
        if(isGenerated)
        {
            m_Status=FC_PRM_STATUS_GENERATED;
            iv=GetInt64Param("targetblocktime");
            if(iv>0)
            {
                dv=2*(double)GetInt64Param("rewardhalvinginterval")/(double)iv;
                dv*=(double)GetInt64Param("initialblockreward");
                iv=GetInt64Param("firstblockreward");
                if(iv<0)
                {
                    iv=GetInt64Param("initialblockreward");
                }
                dv+=(double)iv;
                if(dv > 9.e+18)
                {
                    printf("Total mining reward over blockchain's history is more than 2^63 raw units. Please reduce initial-block-reward or reward-halving-interval.\n");
                    return FC_ERR_INVALID_PARAMETER_VALUE;                                                                                    
                }
           }
        }
        else
        {
            if(isMinimal)
            {
                m_Status=FC_PRM_STATUS_MINIMAL;                            
            }
            else
            {
                m_Status=FC_PRM_STATUS_ERROR;            
            }
        }        
    }
    
    return FC_ERR_NOERROR;
}

int fc_FloatchainParams::Print(FILE* fileHan)
{
    int i,c,size;
    int version;
    int header_printed;
    int set,chars_remaining;
    void *ptr;
    char line[FC_PRM_DAT_FILE_LINE_SIZE+1+100];
    char *cptr;
    int num_sets;
    double d,d1,d2;
    
    int param_sets[]={FC_PRM_COMMENT, FC_PRM_USER, FC_PRM_GENERATED, FC_PRM_CALCULATED};
    num_sets=sizeof(param_sets)/sizeof(int);    
    
    fprintf(fileHan,"# ==== FloatChain configuration file ====\n\n");
    fprintf(fileHan,"# Created by floatchain-util \n");
    
    version=ProtocolVersion();
    if(version)
    {
        fprintf(fileHan,"# Protocol version: %d \n\n",version);
    }
    else
    {
        version=fc_gState->m_Features->LastVersionNotSendingProtocolVersionInHandShake();
    }
    
    switch(m_Status)
    {
        case FC_PRM_STATUS_EMPTY:
            fprintf(fileHan,"# Parameter set is EMPTY \n");
            fprintf(fileHan,"# To join network please run \"floatchaind %s@<seed-node-ip>[:<seed-node-port>]\".\n",Name());
            return FC_ERR_NOERROR;
        case FC_PRM_STATUS_ERROR:
            fprintf(fileHan,"# This parameter set cannot be used for generating network. \n");
            fprintf(fileHan,"# One of the parameters is invalid. \n");
            fprintf(fileHan,"# Please fix it and rerun floatchain-util. \n");
            break;
        case FC_PRM_STATUS_MINIMAL:
            fprintf(fileHan,"# This parameter set contains MINIMAL number of parameters required for connection to existing network. \n");
            fprintf(fileHan,"# To join network please run \"floatchaind %s@<seed-node-ip>[:<seed-node-port>]\".\n",Name());
            break;
        case FC_PRM_STATUS_GENERATED:
            fprintf(fileHan,"# This parameter set is properly GENERATED. \n");
            fprintf(fileHan,"# To generate network please run \"floatchaind %s\".\n",Name());
            break;
        case FC_PRM_STATUS_VALID:
            fprintf(fileHan,"# This parameter set is VALID. \n");
            fprintf(fileHan,"# To join network please run \"floatchaind %s\".\n",Name());
            break;
    }
        
    for(set=0;set<num_sets;set++)
    {
        header_printed=0;
                
        i=0;
        while(i>=0)
        {
            if( (((m_lpParams+i)->m_Type & FC_PRM_SOURCE_MASK) == param_sets[set]) && 
                    ((m_lpParams+i)->IsRelevant(version) > 0))
            {
                if(header_printed == 0)
                {
                    fprintf(fileHan,"\n");
                    header_printed=1;
                    switch(param_sets[set])
                    {
                        case FC_PRM_COMMENT:
                            fprintf(fileHan,"# The following parameters don't influence floatchain network configuration. \n");
                            fprintf(fileHan,"# They may be edited at any moment. \n");                            
                            break;
                        case FC_PRM_USER:
                            if(m_Status == FC_PRM_STATUS_ERROR)
                            {
                                fprintf(fileHan,"# The following parameters can be edited to fix errors. \n");
                                if(Name())
                                {
                                    fprintf(fileHan,"# Please rerun \"floatchain-util clone %s <new-network-name>\". \n",Name());
                                }
                            }
                            else
                            {
                                if(m_Status == FC_PRM_STATUS_GENERATED)
                                {
                                    fprintf(fileHan,"# The following parameters can be edited before running floatchaind for this chain. \n");                                    
                                }
                                else
                                {
                                    fprintf(fileHan,"# The following parameters can only be edited if this file is a prototype of another configuration file. \n");
                                    fprintf(fileHan,"# Please run \"floatchain-util clone %s <new-network-name>\" to generate new network. \n",Name());
                                }
                            }
                            break;
                        case FC_PRM_GENERATED:
                            fprintf(fileHan,"# The following parameters were generated by floatchain-util.\n");
                            fprintf(fileHan,"# They SHOULD ONLY BE EDITED IF YOU KNOW WHAT YOU ARE DOING. \n");
                            break;
                        case FC_PRM_CALCULATED:
                            fprintf(fileHan,"# The following parameters were generated by floatchaind.\n");
                            fprintf(fileHan,"# They SHOULD NOT BE EDITED. \n");
                            break;
                    }
                    fprintf(fileHan,"\n");
                }

                if(strlen((m_lpParams+i)->m_Group))
                {
                    fprintf(fileHan,"\n# %s\n\n",(m_lpParams+i)->m_Group);
                }
                
                chars_remaining=0;
                
                sprintf(line,"%s = ",(m_lpParams+i)->m_DisplayName);
                ptr=GetParam((m_lpParams+i)->m_Name,&size);
                if(size == 0)
                {
                    ptr=NULL;
                }                
                else
                {
                    if(((m_lpParams+i)->m_Type & FC_PRM_DATA_TYPE_MASK) == FC_PRM_STRING)
                    {
                        if(size == 1)
                        {
                            if( (((m_lpParams+i)->m_Type & FC_PRM_SPECIAL) == 0) ||
                                (strcmp((m_lpParams+i)->m_Name,"rootstreamname") != 0) )   
                            {
                                ptr=NULL;                    
                            }
                        }
                    }
                }

                if(ptr)
                {
                    switch((m_lpParams+i)->m_Type & FC_PRM_DATA_TYPE_MASK)
                    {
                        case FC_PRM_BINARY:
                            if(2*size+strlen(line)>FC_PRM_DAT_FILE_LINE_SIZE)
                            {
                                fprintf(fileHan,"%s",line);
                                for(c=0;c<size;c++)
                                {
                                    fprintf(fileHan,"%02x",*((unsigned char*)ptr+c));
                                }
                                chars_remaining=1;
                            }
                            else
                            {
                                for(c=0;c<size;c++)
                                {
                                    sprintf(line+strlen(line),"%02x",*((unsigned char*)ptr+c));
                                }                            
                            }
                            break;
                        case FC_PRM_STRING:
                            if(size+strlen(line)>FC_PRM_DAT_FILE_LINE_SIZE)
                            {
                                fprintf(fileHan,"%s",line);
                                fprintf(fileHan,"%s",(char*)ptr);
                                chars_remaining=1;                                
                            }
                            else
                            {
                                sprintf(line+strlen(line),"%s",(char*)ptr);
                            }
                            break;
                        case FC_PRM_BOOLEAN:
                            if(*(char*)ptr)
                            {
                                sprintf(line+strlen(line),"true");                                
                            }
                            else
                            {
                                sprintf(line+strlen(line),"false");                                                                
                            }
                            break;
                        case FC_PRM_INT32:
                            if((m_lpParams+i)->m_Type & FC_PRM_DECIMAL)
                            {
                                if(fc_GetLE(ptr,4))
                                {
                                    int n=(int)fc_GetLE(ptr,4);
                                    if(n >= 0)
                                    {
                                        d=((double)n+ParamAccuracy())/FC_PRM_DECIMAL_GRANULARITY;
                                    }
                                    else
                                    {
                                        d=-((double)(-n)+ParamAccuracy())/FC_PRM_DECIMAL_GRANULARITY;                                        
                                    }
                                    sprintf(line+strlen(line),"%0.6g",d);                                                                
                                }
                                else
                                {
                                    d=0;
                                    sprintf(line+strlen(line),"0.0");                                                                
                                }
                            }
                            else
                            {
                                sprintf(line+strlen(line),"%d",(int)fc_GetLE(ptr,4));                                                                
                            }
                            break;
                        case FC_PRM_UINT32:
                            if((m_lpParams+i)->m_Type & FC_PRM_DECIMAL)
                            {
                                if(fc_GetLE(ptr,4))
                                {
                                    d=((double)fc_GetLE(ptr,4)+ParamAccuracy())/FC_PRM_DECIMAL_GRANULARITY;
                                    sprintf(line+strlen(line),"%0.6g",d);                                                                
                                }
                                else
                                {
                                    d=0;
                                    sprintf(line+strlen(line),"0.0");                                                                
                                }
                            }
                            else
                            {
                                sprintf(line+strlen(line),"%ld",fc_GetLE(ptr,4));                                                                
                            }
                            break;
                        case FC_PRM_INT64:
                            sprintf(line+strlen(line),"%lld",(long long int)fc_GetLE(ptr,8));                                                                
                            break;
                        case FC_PRM_DOUBLE:
                            sprintf(line+strlen(line),"%f",*(double*)ptr);                                                                
                            break;
                    }
                }
                else
                {
                    sprintf(line+strlen(line),"[null]");                                                                                    
                }
                if(chars_remaining == 0)
                {
                    fprintf(fileHan,"%s",line);
                    chars_remaining=FC_PRM_DAT_FILE_LINE_SIZE-strlen(line)+1;
                }
                for(c=0;c<chars_remaining;c++)
                {
                    fprintf(fileHan," ");
                }
                cptr=(m_lpParams+i)->m_Description;
                while(*cptr)
                {
                    c=0;
                    
                    while((c<(int)strlen(cptr)) && (cptr[c]!='\n'))
                    {
                        c++;
                    }
                    
                    if(c<(int)strlen(cptr))
                    {
                        cptr[c]=0x00;
                        fprintf(fileHan,"# %s",cptr);
                        memset(line,0x20,FC_PRM_DAT_FILE_LINE_SIZE);
                        line[FC_PRM_DAT_FILE_LINE_SIZE]=0x00;
                        fprintf(fileHan,"\n%s ",line);
                        cptr+=c+1;
                    }
                    else
                    {
                        fprintf(fileHan,"# %s",cptr);
                        cptr+=c;
                    }
                }

                switch((m_lpParams+i)->m_Type & FC_PRM_DATA_TYPE_MASK)
                {
                    case FC_PRM_INT32:
                    case FC_PRM_INT64:
                    case FC_PRM_UINT32:
                        switch(param_sets[set])
                        {
                            case FC_PRM_COMMENT:
                            case FC_PRM_USER:
                                if((m_lpParams+i)->m_MinIntegerValue <= (m_lpParams+i)->m_MaxIntegerValue)
                                {
                                    if((m_lpParams+i)->m_Type & FC_PRM_DECIMAL)
                                    {
                                        d1=0;
                                        if((m_lpParams+i)->m_MinIntegerValue)
                                        {
                                            d1=((double)((m_lpParams+i)->m_MinIntegerValue)+ParamAccuracy())/FC_PRM_DECIMAL_GRANULARITY;
                                        }
                                        d2=0;
                                        if((m_lpParams+i)->m_MaxIntegerValue)
                                        {
                                            d2=((double)((m_lpParams+i)->m_MaxIntegerValue)+ParamAccuracy())/FC_PRM_DECIMAL_GRANULARITY;
                                        }
                                        fprintf(fileHan," (%0.6g - %0.6g)",d1,d2);                            
                                    }
                                    else
                                    {
                                        fprintf(fileHan," (%ld - %ld)",(m_lpParams+i)->m_MinIntegerValue,(m_lpParams+i)->m_MaxIntegerValue);                            
                                    }
                                }
                                break;
                        }
                        break;
                }
                fprintf(fileHan,"\n");                    
            }
            
            if(strlen((m_lpParams+i)->m_Next))
            {
                i=m_lpIndex->Get((m_lpParams+i)->m_Next);
            }
            else
            {
                i=-1;
            }
        }   
        
    }

    fprintf(fileHan,"\n");                    
    return FC_ERR_NOERROR;

}

int fc_FloatchainParams::Write(int overwrite)
{
    FILE *fileHan;
    int create;
    char fileName[FC_DCT_DB_MAX_PATH];
    
    if(Name() == NULL)
    {
        return FC_ERR_INTERNAL_ERROR;
    }
    
    fileHan=fc_OpenFile(Name(),"params",".dat","r",FC_FOM_RELATIVE_TO_DATADIR);
    if(fileHan)
    {
        fc_CloseFile(fileHan);
        if(overwrite)
        {
            fc_BackupFile(Name(),"params",".dat",FC_FOM_RELATIVE_TO_DATADIR);
        }
        else
        {
            fc_GetFullFileName(fc_gState->m_Params->m_Arguments[1],"params", ".dat",FC_FOM_RELATIVE_TO_DATADIR,fileName);
            printf("Cannot create chain parameter set, file %s already exists\n",fileName);
            return FC_ERR_INVALID_PARAMETER_VALUE;                
        }
    }

    create=FC_FOM_CREATE_DIR;
    if(overwrite)
    {
        create=0;
    }
    
    fileHan=fc_OpenFile(Name(),"params",".dat","w",FC_FOM_RELATIVE_TO_DATADIR | create);
    if(fileHan == NULL)
    {
        fc_GetFullFileName(fc_gState->m_Params->m_Arguments[1],"params", ".dat",FC_FOM_RELATIVE_TO_DATADIR,fileName);
        printf("Cannot create chain parameter set, cannot open file %s for writing\n",fileName);
        return FC_ERR_INVALID_PARAMETER_VALUE;                
    }

    if(Print(fileHan))
    {
        fc_GetFullFileName(fc_gState->m_Params->m_Arguments[1],"params", ".dat",FC_FOM_RELATIVE_TO_DATADIR,fileName);
        printf("Cannot create chain parameter set, write error to file %s\n",fileName);
        fc_CloseFile(fileHan);
        if(overwrite)
        {
            fc_RecoverFile(Name(),"params",".dat",FC_FOM_RELATIVE_TO_DATADIR);
        }
        return FC_ERR_INVALID_PARAMETER_VALUE;                        
    }
    
    fc_CloseFile(fileHan);
    
    
    return FC_ERR_NOERROR;
}




const char* fc_FloatchainParams::Name()
{
    return (char*)GetParam("chainname",NULL);
}

const unsigned char* fc_FloatchainParams::MessageStart()
{
    return (unsigned char*)GetParam("networkmessagestart",NULL);
}

const unsigned char* fc_FloatchainParams::DefaultMessageStart()
{
    return c_DefaultMessageStart;
}


const unsigned char* fc_FloatchainParams::AddressVersion()
{
    return (unsigned char*)GetParam("addresspubkeyhashversion",NULL);
}

const unsigned char* fc_FloatchainParams::AddressScriptVersion()
{
    return (unsigned char*)GetParam("addressscripthashversion",NULL);
}

const unsigned char* fc_FloatchainParams::AddressCheckumValue()
{
    return (unsigned char*)GetParam("addresschecksumvalue",NULL);
}


int fc_FloatchainParams::ProtocolVersion()
{
    if(m_ProtocolVersion)
    {
        return m_ProtocolVersion;
    }
    void *ptr=GetParam("protocolversion",NULL);
    if(ptr)
    {
        return fc_GetLE(ptr,4);
    }
    return 0;
}

int fc_FloatchainParams::IsProtocolFloatchain()
{
    return m_IsProtocolFloatChain;
}


int fc_Features::MinProtocolVersion()
{
    return 10004;
}

int fc_Features::ActivatePermission()                                           // This test is eliminated from the code as 10002 is not supported
{
    int ret=0;
    int protocol=fc_gState->m_NetworkParams->ProtocolVersion();
    
    if(protocol)
    {
        if(protocol >= 10003)
        {
            ret=1;
        }
    }
    
    return ret;
}

int fc_Features::LastVersionNotSendingProtocolVersionInHandShake()
{
    return 10002;
}

int fc_Features::VerifySizeOfOpDropElements()                                   // This test is still in the code to keep protocol!-floatchain untouched
{
    
    int ret=0;        
    int protocol=fc_gState->m_NetworkParams->ProtocolVersion();
    
    if(fc_gState->m_NetworkParams->IsProtocolFloatchain() == 0)
    {
        return 1;
    }
    
    if(protocol)
    {
        if(protocol >= 10003)
        {
            ret=1;
        }
    }
    
    return ret;
}

int fc_Features::PerEntityPermissions()                                         // This test is eliminated from the code as 10002 is not supported
{
    if(fc_gState->m_NetworkParams->IsProtocolFloatchain() == 0)
    {
        return 0;
    }
    int ret=0;
    int protocol=fc_gState->m_NetworkParams->ProtocolVersion();
    
    if(protocol)
    {
        if(protocol >= 10004)
        {
            ret=1;
        }
    }
    
    return ret;
}

int fc_Features::FollowOnIssues()                                               // This test is eliminated from the code as 10002 is not supported
{
    int ret=0;
    if(fc_gState->m_NetworkParams->IsProtocolFloatchain() == 0)
    {
        return 0;
    }
    int protocol=fc_gState->m_NetworkParams->ProtocolVersion();
    if(protocol)
    {
        if(protocol >= 10004)
        {
            ret=1;
        }
    }
    
    return ret;
}

int fc_Features::SpecialParamsInDetailsScript()                                 // This test is eliminated from the code as 10002 is not supported
{
    int ret=0;
    int protocol=fc_gState->m_NetworkParams->ProtocolVersion();
    
    if(protocol)
    {
        if(protocol >= 10004)
        {
            ret=1;
        }
    }
    
    return ret;    
}

int fc_Features::FixedGrantsInTheSameTx()                                       // This test is eliminated from the code as 10002 is not supported
{
    int ret=0;
    int protocol=fc_gState->m_NetworkParams->ProtocolVersion();
    
    if(protocol)
    {
        if(protocol >= 10004)
        {
            ret=1;
        }
    }
    
    return ret;    
}

int fc_Features::UnconfirmedMinersCannotMine()
{
    int ret=0;
    int protocol=fc_gState->m_NetworkParams->ProtocolVersion();
    
    if(protocol)
    {
        if(protocol >= 10004)
        {
            ret=1;
        }
    }
    
    return ret;    
}

int fc_Features::Streams()
{
    int ret=0;
    if(fc_gState->m_NetworkParams->IsProtocolFloatchain() == 0)
    {
        return 0;
    }
    int protocol=fc_gState->m_NetworkParams->ProtocolVersion();
    
    if(protocol)
    {
        if(protocol >= 10006)
        {
            ret=1;
        }
    }
    
    return ret;    
}

int fc_Features::OpDropDetailsScripts()
{
    int ret=0;
    if(fc_gState->m_NetworkParams->IsProtocolFloatchain() == 0)
    {
        return 0;
    }
    int protocol=fc_gState->m_NetworkParams->ProtocolVersion();
    
    if(protocol)
    {
        if(protocol >= 10007)
        {
            ret=1;
        }
    }
    
    return ret;    
}

int fc_Features::ShortTxIDInTx()
{
    int ret=0;
    if(fc_gState->m_NetworkParams->IsProtocolFloatchain() == 0)
    {
        return 0;
    }
    int protocol=fc_gState->m_NetworkParams->ProtocolVersion();
    
    if(protocol)
    {
        if(protocol >= 10007)
        {
            ret=1;
        }
    }
    
    return ret;    
}

int fc_Features::CachedInputScript()
{
    int ret=0;
    if(fc_gState->m_NetworkParams->IsProtocolFloatchain() == 0)
    {
        return 0;
    }
    int protocol=fc_gState->m_NetworkParams->ProtocolVersion();
    
    if(protocol)
    {
        if(protocol >= 10007)
        {
            ret=1;
        }
    }
    
    return ret;    
}

int fc_Features::AnyoneCanReceiveEmpty()
{
    int ret=0;
    if(fc_gState->m_NetworkParams->IsProtocolFloatchain() == 0)
    {
        return 0;
    }
    int protocol=fc_gState->m_NetworkParams->ProtocolVersion();
    
    if(protocol)
    {
        if(protocol >= 10007)
        {
            if(FCP_ANYONE_CAN_RECEIVE_EMPTY)                                
            {
                ret=1;
            }
        }
    }
    
    return ret;    
}

int fc_Features::FixedIn10007()
{
    int ret=0;
    if(fc_gState->m_NetworkParams->IsProtocolFloatchain() == 0)
    {
        return 0;
    }
    int protocol=fc_gState->m_NetworkParams->ProtocolVersion();
    
    if(protocol)
    {
        if(protocol >= 10007)
        {
            ret=1;
        }
    }
    
    return ret;    
}

int fc_Features::Upgrades()
{
    int ret=0;
    if(fc_gState->m_NetworkParams->IsProtocolFloatchain() == 0)
    {
        return 0;
    }
    int protocol=fc_gState->m_NetworkParams->ProtocolVersion();
    
    if(protocol)
    {
        if(protocol >= 10008)
        {
            ret=1;
        }
    }
    
    return ret;    
}

int fc_Features::FixedIn10008()
{
    int ret=0;
    if(fc_gState->m_NetworkParams->IsProtocolFloatchain() == 0)
    {
        return 0;
    }
    int protocol=fc_gState->m_NetworkParams->ProtocolVersion();
    
    if(protocol)
    {
        if(protocol >= 10008)
        {
            ret=1;
        }
    }
    
    return ret;    
}


