// Copyright (c) 2014-2017 Coin Sciences Ltd
// FloatChain code distributed under the GPLv3 license, see COPYING file.

#include "floatchain/floatchain.h"
#include "chainparams/globals.h"

int main(int argc, char* argv[])
{
    int err;
    int version,v;
    char fileName[FC_DCT_DB_MAX_PATH];
    char DataDirArg[FC_DCT_DB_MAX_PATH];
    int isSetDataDirArg;
    FILE *fHan;
    
    FC_FloatchainParams* params;
    FC_FloatchainParams* paramsOld;
    FC_gState=new FC_State;
     
    FC_gState->m_Params->Parse(argc, argv);
    FC_CheckDataDirInConfFile();

    FC_gState->m_Params->ReadConfig(NULL);
    
    FC_ExpandDataDirParam();
    
    printf("FloatChain utilities %s\n\n",FC_gState->GetFullVersion());
             
    err=FC_ERR_OPERATION_NOT_SUPPORTED;
     
    if(FC_gState->m_Params->Command())
    {
        if(strcmp(FC_gState->m_Params->Command(),"create") == 0)
        {
            if(FC_gState->m_Params->m_NumArguments>1)
            {
                params=new FC_FloatchainParams;
                
                err=FC_ERR_NOERROR;
                
                version=FC_gState->GetProtocolVersion();
                if(FC_gState->m_Params->m_NumArguments>2)
                {                    
                    v=atoi(FC_gState->m_Params->m_Arguments[2]);
                    if( (v>=FC_gState->m_Features->MinProtocolVersion()) && (v<=version) )
                    {
                        version=v;                        
                    }
                    else
                    {
                        fprintf(stderr,"ERROR: Invalid value for protocol version. Valid range: %d - %d\n",FC_gState->m_Features->MinProtocolVersion(), FC_gState->GetProtocolVersion());   
                        err=FC_ERR_INVALID_PARAMETER_VALUE;
                    }
                }
                
                if(err == FC_ERR_NOERROR)
                {
//                    err=params->Create(FC_gState->m_Params->m_Arguments[1],version);
                    err=params->Read(FC_gState->m_Params->m_Arguments[1],argc, argv,version);
                }
                if(err == FC_ERR_NOERROR)
                {
                    err=params->Validate();
                }
                if(err == FC_ERR_NOERROR)
                {
                    err=params->Write(0);
                }                

                if(err == FC_ERR_NOERROR)
                {
                    FC_GenerateConfFiles(FC_gState->m_Params->m_Arguments[1]);                
                }
                if(err == FC_ERR_NOERROR)
                { 
                    printf("Blockchain parameter set was successfully generated.\n");
                    FC_GetFullFileName(FC_gState->m_Params->m_Arguments[1],"params", ".dat",FC_FOM_RELATIVE_TO_DATADIR,fileName);
                    printf("You can edit it in %s before running floatchaind for the first time.\n\n",fileName);
                    printf("To generate blockchain please run \"floatchaind %s -daemon\".\n",params->Name());
                }                
                else
                {
                    fprintf(stderr,"ERROR: Blockchain parameter set was not generated.\n");
                }
                delete params;
            }    
        }
        if(strcmp(FC_gState->m_Params->Command(),"clone") == 0)
        {
            if(FC_gState->m_Params->m_NumArguments>2)
            {
                params=new FC_FloatchainParams;
                paramsOld=new FC_FloatchainParams;

                
                isSetDataDirArg=FC_GetDataDirArg(DataDirArg);
                if(isSetDataDirArg)
                {
                    FC_UnsetDataDirArg();
                }                
                err=FC_ERR_NOERROR;
                FC_GetFullFileName(FC_gState->m_Params->m_Arguments[1],"params", ".dat",FC_FOM_RELATIVE_TO_DATADIR,fileName);
                if ((fHan = fopen(fileName, "r")))
                {
                    fclose(fHan);
                }
                else
                {
                    fprintf(stderr,"Cannot create chain parameter set, file %s does not exist\n",fileName);                    
                    err=FC_ERR_FILE_READ_ERROR;
                }

                if(err == FC_ERR_NOERROR)
                {
                    err=paramsOld->Read(FC_gState->m_Params->m_Arguments[1]);
                }                
                if(isSetDataDirArg)
                {
                    FC_SetDataDirArg(DataDirArg);
                }                

                FC_gState->m_Params->m_Arguments[1]=FC_gState->m_Params->m_Arguments[2];
                if(err == FC_ERR_NOERROR)
                {
                    err=params->Clone(FC_gState->m_Params->m_Arguments[2],paramsOld);
                }                
                if(err == FC_ERR_NOERROR)
                {
                    err=params->Validate();
                }
                if(err == FC_ERR_NOERROR)
                {
                    err=params->Write(0);
                }
                if(err == FC_ERR_NOERROR)
                {
                    
                    printf("Blockchain parameter set was successfully cloned.\n");
                    FC_GetFullFileName(FC_gState->m_Params->m_Arguments[2],"params", ".dat",FC_FOM_RELATIVE_TO_DATADIR,fileName);
                    printf("You can edit it in %s before running floatchaind for the first time.\n\n",fileName);
                    printf("To generate blockchain please run \"floatchaind %s -daemon\".\n",params->Name());
                }                
                else
                {
                    fprintf(stderr,"ERROR: Blockchain parameter set was not generated.\n");
                }
                delete paramsOld;
                delete params;
            }
            err=FC_ERR_NOERROR;
        }
/*        
        if(strcmp(FC_gState->m_Params->Command(),"test") == 0)
        {
            if(FC_gState->m_Params->m_NumArguments>1)
            {
                printf("\n>>>>> Test %s started\n\n",FC_gState->m_Params->m_Arguments[1]);
                if(strcmp(FC_gState->m_Params->m_Arguments[1],"scenario") == 0)
                {
                    if(FC_gState->m_Params->m_NumArguments>2)
                    {
                        err=FC_TestScenario(FC_gState->m_Params->m_Arguments[2]);                        
                    }                    
                }        
            }           
            
            if(err == FC_ERR_NOERROR)
            {
                printf("\n>>>>> Test completed\n\n");
            }            
            else
            {
                if(err == FC_ERR_OPERATION_NOT_SUPPORTED)                
                {
                    printf("\n>>>>> ERROR: Test not found\n\n");                    
                }
                else
                {
                    printf("\n>>>>> ERROR: Test exited with error code %d\n\n",err);                    
                }
            }
            
        }
*/        
        
    }
    
    if(err == FC_ERR_OPERATION_NOT_SUPPORTED)
    {
        FC_GetFullFileName("<blockchain-name>","params", ".dat",FC_FOM_RELATIVE_TO_DATADIR,fileName);
        printf("Usage:\n");
        printf("  floatchain-util create <blockchain-name>  ( <protocol-version> = %d ) [options]        Creates new floatchain configuration file %s with default parameters\n",
                FC_gState->GetProtocolVersion(),fileName);
        FC_GetFullFileName("<new-blockchain-name>","params", ".dat",FC_FOM_RELATIVE_TO_DATADIR,fileName);
        printf("  floatchain-util clone <old-blockchain-name> <new-blockchain-name> [options]               Creates new floatchain configuration file %s copying parameters\n",fileName);
        
        isSetDataDirArg=FC_GetDataDirArg(DataDirArg);
        if(isSetDataDirArg)
        {
            FC_UnsetDataDirArg();
        }                
        FC_GetFullFileName("<old-blockchain-name>","params", ".dat",FC_FOM_RELATIVE_TO_DATADIR,fileName);
        if(isSetDataDirArg)
        {
            FC_SetDataDirArg(DataDirArg);
        }                
        printf("                                                                                            from %s\n",fileName);
        printf("\n");
        printf("Options:\n");
        printf("  -datadir=<dir>                              Specify data directory\n");
        printf("  -<parameter-name>=<parameter-value>         Specify blockchain parameter value, e.g. -anyone-can-connect=true\n\n");
    }
            
    delete FC_gState;

    if(err)
    {
        return 1;
    }
    
    return 0;
}
