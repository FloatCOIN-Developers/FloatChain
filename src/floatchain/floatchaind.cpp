// Copyright (c) 2014-2016 The Bitcoin Core developers
// Original code was distributed under the MIT software license.
// Copyright (c) 2014-2017 Coin Sciences Ltd
// FloatChain code distributed under the GPLv3 license, see COPYING file.

#include "version/clientversion.h"
#include "rpc/rpcserver.h"
#include "core/init.h"
#include "core/main.h"
#include "ui/noui.h"
#include "ui/ui_interface.h"
#include "utils/util.h"


#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>

#include "floatchain/floatchain.h"
#include "chainparams/globals.h"
static bool fDaemon;

void DebugPrintClose();

void DetectShutdownThread(boost::thread_group* threadGroup)
{
    bool fShutdown = ShutdownRequested();
    // Tell the main threads to shutdown.
    while (!fShutdown)
    {
        MilliSleep(200);
        fShutdown = ShutdownRequested();
    }
    if (threadGroup)
    {
        threadGroup->interrupt_all();
        threadGroup->join_all();
    }
}

bool FC_DoesParentDataDirExist()
{
    if (mapArgs.count("-datadir"))
    {
        boost::filesystem::path path=boost::filesystem::system_complete(mapArgs["-datadir"]);
        if (!boost::filesystem::is_directory(path)) 
        {
            return false;
        }    
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////////
//
// Start
//
bool AppInit(int argc, char* argv[])
{
    boost::thread_group threadGroup;
    boost::thread* detectShutdownThread = NULL;

    bool fRet = false;

    int size;
    int err = FC_ERR_NOERROR;
    int pipes[2];
    int bytes_read;
    int bytes_written;
    char bufOutput[4096];
    bool is_daemon;
    //
    // Parameters
    //
    // If Qt is used, parameters/bitcoin.conf are parsed in qt/bitcoin.cpp's main()

        
    FC_gState=new FC_State;
    
    FC_gState->m_Params->Parse(argc, argv);
    FC_CheckDataDirInConfFile();
    
    if(FC_gState->m_Params->NetworkName())
    {
        if(strlen(FC_gState->m_Params->NetworkName()) > FC_PRM_NETWORK_NAME_MAX_SIZE)
        {
            fprintf(stderr, "Error: invalid chain name: %s\n",FC_gState->m_Params->NetworkName());
            return false;
        }
    }
    
    
    
    if(!FC_DoesParentDataDirExist())
    {
        fprintf(stderr,"\nError: Data directory %s needs to exist before calling floatchaind. Exiting...\n\n",mapArgs["-datadir"].c_str());
        return false;        
    }
        
    
    FC_gState->m_Params->HasOption("-?");
            
    // Process help and version before taking care about datadir
    if (FC_gState->m_Params->HasOption("-?") || 
        FC_gState->m_Params->HasOption("-help") || 
        FC_gState->m_Params->HasOption("-version") || 
        (FC_gState->m_Params->NetworkName() == NULL))
    {
        fprintf(stdout,"\nFloatChain Core Daemon %s\n\n",FC_gState->GetFullVersion());
        std::string strUsage = "";
        if (FC_gState->m_Params->HasOption("-version"))
        {
            strUsage += LicenseInfo();
        }
        else
        {
            strUsage += "\n" + _("Usage:") + "\n" +
                  "  floatchaind <blockchain-name> [options]                     " + _("Start FloatChain Core Daemon") + "\n";

            strUsage += "\n" + HelpMessage(HMM_BITCOIND);                       // FCHN-TODO edit help message
        }

        fprintf(stdout, "%s", strUsage.c_str());

        delete FC_gState;                
        return true;
    }

    if(!GetBoolArg("-shortoutput", false))
    {
        fprintf(stdout,"\nFloatChain Core Daemon %s\n\n",FC_gState->GetFullVersion());
    }
    
    pipes[1]=STDOUT_FILENO;
    is_daemon=false;
#ifndef WIN32
        fDaemon = GetBoolArg("-daemon", false);
        
        if (fDaemon)
        {
            delete FC_gState;                
            
            if(!GetBoolArg("-shortoutput", false))
            {
                fprintf(stdout, "FloatChain server starting\n");
            }
            
            if (pipe(pipes)) 
            {
                fprintf(stderr, "Error: couldn't create pipe between parent and child processes\n");
                return false;
            }
            
            pid_t pid = fork();
            if (pid < 0)
            {
                fprintf(stderr, "Error: fork() returned %d errno %d\n", pid, errno);
//                delete FC_gState;
                return false;
            }
            
            if(pid == 0)
            {
                is_daemon=true;
                close(pipes[0]);
            }
            
            if (pid > 0)                                                        // Parent process, pid is child process id
            {
//                delete FC_gState;
                close(pipes[1]);            
                bytes_read=1;                
                while(bytes_read>0)
                {
                    bytes_read=read(pipes[0],bufOutput,4096);
                    if(bytes_read <= 0)
                    {
                        return true;                        
                    }
                    bytes_written=write(STDOUT_FILENO,bufOutput,bytes_read);
                    if(bytes_written != bytes_read)
                    {
                        return true;                                                
                    }
                }
                return true;
            }
            
            pid_t sid = setsid();
            if (sid < 0)
                fprintf(stderr, "ERROR: setsid() returned %d errno %d\n", sid, errno);
            
            FC_gState=new FC_State;

            FC_gState->m_Params->Parse(argc, argv);
        }
#endif
        
    FC_GenerateConfFiles(FC_gState->m_Params->NetworkName());                
    
    err=FC_gState->m_Params->ReadConfig(FC_gState->m_Params->NetworkName());
    
    if(err)
    {
        fprintf(stderr,"ERROR: Couldn't read parameter file for blockchain %s. Exiting...\n",FC_gState->m_Params->NetworkName());
        delete FC_gState;                
        return false;
    }

    err=FC_gState->m_NetworkParams->Read(FC_gState->m_Params->NetworkName());
    if(err)
    {
        fprintf(stderr,"ERROR: Couldn't read configuration file for blockchain %s. Please try upgrading FloatChain. Exiting...\n",FC_gState->m_Params->NetworkName());
        delete FC_gState;                
        return false;
    }
    
    err=FC_gState->m_NetworkParams->Validate();
    if(err)
    {
        fprintf(stderr,"ERROR: Couldn't validate parameter set for blockchain %s. Exiting...\n",FC_gState->m_Params->NetworkName());
        delete FC_gState;                
        return false;
    }
 
    if(GetBoolArg("-reindex", false))
    {
        FC_RemoveDir(FC_gState->m_Params->NetworkName(),"permissions.db");
        FC_RemoveFile(FC_gState->m_Params->NetworkName(),"permissions",".dat",FC_FOM_RELATIVE_TO_DATADIR);
        FC_RemoveFile(FC_gState->m_Params->NetworkName(),"permissions",".log",FC_FOM_RELATIVE_TO_DATADIR);
    }
    
    FC_gState->m_Permissions= new FC_Permissions;
    err=FC_gState->m_Permissions->Initialize(FC_gState->m_Params->NetworkName(),0);                                
    if(err)
    {
        if(err == FC_ERR_CORRUPTED)
        {
            fprintf(stderr,"\nERROR: Couldn't initialize permission database for blockchain %s. Please restart floatchaind with reindex=1.\n",FC_gState->m_Params->NetworkName());                        
        }
        else
        {
            fprintf(stderr,"\nERROR: Couldn't initialize permission database for blockchain %s. Probably floatchaind for this blockchain is already running. Exiting...\n",FC_gState->m_Params->NetworkName());
        }
        delete FC_gState;                
        return false;
    }

    if( (FC_gState->m_NetworkParams->GetParam("protocolversion",&size) != NULL) &&
        (FC_gState->GetProtocolVersion() < (int)FC_gState->m_NetworkParams->GetInt64Param("protocolversion")) )
    {
            fprintf(stderr,"ERROR: Parameter set for blockchain %s was generated by FloatChain running newer protocol version (%d)\n\n",
                    FC_gState->m_Params->NetworkName(),(int)FC_gState->m_NetworkParams->GetInt64Param("protocolversion"));                        
            fprintf(stderr,"Please upgrade FloatChain\n\n");
            delete FC_gState;                
            return false;
    }

    if( (FC_gState->m_NetworkParams->GetParam("protocolversion",&size) != NULL) &&
        (FC_gState->m_Features->MinProtocolVersion() > (int)FC_gState->m_NetworkParams->GetInt64Param("protocolversion")) )
    {
            fprintf(stderr,"ERROR: The protocol version (%d) for blockchain %s has been deprecated and was last supported in FloatChain 1.0 beta 1\n\n",
                    (int)FC_gState->m_NetworkParams->GetInt64Param("protocolversion"),FC_gState->m_Params->NetworkName());                        
            delete FC_gState;                
            return false;
    }

    if(!GetBoolArg("-verifyparamsethash", true))
    {
        if(FC_gState->m_NetworkParams->m_Status == FC_PRM_STATUS_INVALID)
        {
            FC_gState->m_NetworkParams->m_Status=FC_PRM_STATUS_VALID;
        }
    }
    
    switch(FC_gState->m_NetworkParams->m_Status)
    {
        case FC_PRM_STATUS_EMPTY:
        case FC_PRM_STATUS_MINIMAL:
            if(FC_gState->GetSeedNode() == NULL)
            {
                fprintf(stderr,"ERROR: Parameter set for blockchain %s is not complete. \n\n\n",FC_gState->m_Params->NetworkName());  
                fprintf(stderr,"If you want to create new blockchain please run one of the following:\n\n");
                fprintf(stderr,"  floatchain-util create %s\n",FC_gState->m_Params->NetworkName());
                fprintf(stderr,"  floatchain-util clone <old-blockchain-name> %s\n",FC_gState->m_Params->NetworkName());
                fprintf(stderr,"\nAnd rerun floatchaind %s\n\n\n",FC_gState->m_Params->NetworkName());                        
                fprintf(stderr,"If you want to connect to existing blockchain please specify seed node:\n\n");
                fprintf(stderr,"  floatchaind %s@<seed-node-ip>\n",FC_gState->m_Params->NetworkName());
                fprintf(stderr,"  floatchaind %s@<seed-node-ip>:<seed-node-port>\n\n\n",FC_gState->m_Params->NetworkName());
                delete FC_gState;                
                return false;
            }
            break;
        case FC_PRM_STATUS_ERROR:
            fprintf(stderr,"ERROR: Parameter set for blockchain %s has errors. Please run one of the following:\n\n",FC_gState->m_Params->NetworkName());                        
            fprintf(stderr,"  floatchain-util create %s\n",FC_gState->m_Params->NetworkName());
            fprintf(stderr,"  floatchain-util clone <old-blockchain-name> %s\n",FC_gState->m_Params->NetworkName());
            fprintf(stderr,"\nAnd rerun floatchaind %s\n",FC_gState->m_Params->NetworkName());                        
            delete FC_gState;                
            return false;
        case FC_PRM_STATUS_INVALID:
            fprintf(stderr,"ERROR: Parameter set for blockchain %s is invalid. You may generate new network using these parameters by running:\n\n",FC_gState->m_Params->NetworkName());                        
            fprintf(stderr,"  floatchain-util clone %s <new-blockchain-name>\n",FC_gState->m_Params->NetworkName());
            delete FC_gState;                
            return false;
        case FC_PRM_STATUS_GENERATED:
        case FC_PRM_STATUS_VALID:
            break;
        default:
            fprintf(stderr,"INTERNAL ERROR: Unknown parameter set status %d\n",FC_gState->m_NetworkParams->m_Status);
            delete FC_gState;                
            return false;
            break;
    }
            
    SelectFloatChainParams(FC_gState->m_Params->NetworkName());

    try
    {
/*        
#ifndef WIN32
        fDaemon = GetBoolArg("-daemon", false);
        
        if (fDaemon)
        {
            fprintf(stdout, "FloatChain server starting\n");

            pid_t pid = fork();
            if (pid < 0)
            {
                fprintf(stderr, "Error: fork() returned %d errno %d\n", pid, errno);
                delete FC_gState;
                return false;
            }
            if (pid > 0) // Parent process, pid is child process id
            {
                delete FC_gState;
                return true;
            }
            
            pid_t sid = setsid();
            if (sid < 0)
                fprintf(stderr, "ERROR: setsid() returned %d errno %d\n", sid, errno);
            
        }
#endif
*/
        SoftSetBoolArg("-server", true);
        detectShutdownThread = new boost::thread(boost::bind(&DetectShutdownThread, &threadGroup));
        fRet = AppInit2(threadGroup,pipes[1]);
        if(is_daemon)
        {
            close(pipes[1]);            
        }
    }
    catch (std::exception& e) {
        PrintExceptionContinue(&e, "AppInit()");
    } catch (...) {
        PrintExceptionContinue(NULL, "AppInit()");
    }

    if (!fRet)
    {
        if (detectShutdownThread)
            detectShutdownThread->interrupt();

        threadGroup.interrupt_all();
        // threadGroup.join_all(); was left out intentionally here, because we didn't re-test all of
        // the startup-failure cases to make sure they don't result in a hang due to some
        // thread-blocking-waiting-for-another-thread-during-startup case
    }
    if (detectShutdownThread)
    {
        detectShutdownThread->join();
        delete detectShutdownThread;
        detectShutdownThread = NULL;
    }

    Shutdown();    
    DebugPrintClose();
    
    std::string datadir_to_delete="";
    
    if(FC_gState->m_NetworkParams->Validate() == 0)
    {
        if(FC_gState->m_NetworkParams->m_Status == FC_PRM_STATUS_EMPTY) 
        {
            datadir_to_delete=strprintf("%s",FC_gState->m_Params->NetworkName());
        }            
    }
    
    delete FC_gState;
    
    if(datadir_to_delete.size())
    {
        FC_RemoveDataDir(datadir_to_delete.c_str());        
    }
    
    return fRet;
}


int main(int argc, char* argv[])
{
    SetupEnvironment();



    // Connect bitcoind signal handlers
    noui_connect();

    return (AppInit(argc, argv) ? 0 : 1);

}

