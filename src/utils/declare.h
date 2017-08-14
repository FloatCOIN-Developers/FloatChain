// Copyright (c) 2014-2017 Coin Sciences Ltd
// FloatChain code distributed under the GPLv3 license, see COPYING file.

#ifndef FLOATCHAIN_DECLARE_H
#define	FLOATCHAIN_DECLARE_H

#include "utils/define.h"

#define FC_DCT_TERM_BUFFER_SIZE         25165824

/* Classes */


#ifdef	__cplusplus
extern "C" {
#endif


typedef struct FC_MapStringIndex
{    
    void *mapObject;
    FC_MapStringIndex()
    {
        mapObject=NULL;
        Init();
    }    

    ~FC_MapStringIndex()
    {
        Destroy();
    }        
    void Init();
    void Add(const char* key,int value);
    void Add(const unsigned char* key,int size,int value);
    void Remove(const char* key,int size);
    int Get(const char* key);
    int Get(const unsigned char* key,int size);
    void Destroy();
    void Clear();
} FC_MapStringIndex;

typedef struct FC_MapStringString
{    
    void *mapObject;
    FC_MapStringString()
    {
        Init();
    }    

    ~FC_MapStringString()
    {
        Destroy();
    }        
    void Init();
    void Add(const char* key,const char* value);
    const char * Get(const char* key);
    void Destroy();
    int GetCount();
} FC_MapStringString;

typedef struct FC_Buffer
{
    FC_Buffer()
    {
        Zero();
    }

    ~FC_Buffer()
    {
        Destroy();
    }

    FC_MapStringIndex      *m_lpIndex;
    unsigned char          *m_lpData;   
    int                     m_AllocSize;
    int                     m_Size;
    int                     m_KeySize;
    int                     m_RowSize;
    int                     m_Count;
    uint32_t                m_Mode;
    
    void Zero();
    int Destroy();
    int Initialize(int KeySize,int TotalSize,uint32_t Mode);
    
    int Clear();
    int Realloc(int Rows);
    int Add(const void *lpKey,const void *lpValue);
    int Add(const void *lpKeyValue);
    int Seek(void *lpKey);
    unsigned char *GetRow(int RowID);    
    int PutRow(int RowID,const void *lpKey,const void *lpValue);    
    int GetCount();    
    int SetCount(int count);
    
    int Sort();    
    
    void CopyFrom(FC_Buffer *source);
    
} FC_Buffer;

    
typedef struct FC_List
{
    FC_List()
    {
         Zero();
    }

    ~FC_List()
    {
         Destroy();
    }

    unsigned char          *m_lpData;   
    int                     m_AllocSize;
    int                     m_Size;
    int                     m_Pos;
    int                     m_ItemSize;
    
    void Zero();
    int Destroy();
    
    void Clear();
    int Put(unsigned char *ptr, int size);
    unsigned char *First();
    unsigned char *Next();
    
} FC_List;


typedef struct FC_SHA256
{
    void *m_HashObject;
    void Init();
    void Destroy();
    void Reset();
    void Write(const void *lpData,int size);
    void GetHash(unsigned char *hash);
    
    FC_SHA256()
    {
        Init();
    }
    
    ~FC_SHA256()
    {
        Destroy();
    }
} FC_SHA256;


struct FC_TerminalInput
{
    char m_Prompt[64];
    char m_Data[FC_DCT_TERM_BUFFER_SIZE];
    char m_Line[FC_DCT_TERM_BUFFER_SIZE];
    char m_Cache[FC_DCT_TERM_BUFFER_SIZE];
    int m_Offsets[100];
    
    int m_HistoryLines;
    int m_BufferSize;
    int m_ThisLine;
    int m_FirstLine;
    int m_LoadedLine;
    int m_TerminalCols;
    int m_TerminalRows;
    
    char *GetLine();
    int SetPrompt(const char *prompt);
    int Prompt();
    void AddLine();
    int LoadLine(int line);
    void SaveLine();
    void MoveBack(int offset);
    int TerminalCols();
    int IsAlphaNumeric(char c);
    int LoadDataFromLog(const char *fileName);
    
    FC_TerminalInput()
    {
        int i;
        strcpy(m_Prompt,"");
        m_HistoryLines=100;
        m_BufferSize=FC_DCT_TERM_BUFFER_SIZE;
        m_ThisLine=0;
        m_FirstLine=0;
        m_LoadedLine=0;
        memset(m_Line,0,m_BufferSize);
        memset(m_Data,0,m_BufferSize);
        for(i=0;i<m_HistoryLines;i++)
        {
            m_Offsets[i]=-1;
        }
        m_TerminalCols=0;
    }
};


/* Functions */
    
int FC_AllocSize(int items,int chunk_size,int item_size);
void *FC_New(int Size);
void FC_Delete(void *ptr);
void FC_PutLE(void *dest,void *src,int dest_size);
int64_t FC_GetLE(void *src,int size);
int FC_BackupFile(const char *network_name,const char *filename, const char *extension,int options);
int FC_RecoverFile(const char *network_name,const char *filename, const char *extension,int options);
FILE *FC_OpenFile(const char *network_name,const char *filename, const char *extension,const char *mode, int options);        
void FC_CloseFile(FILE *fHan);
int FC_RemoveFile(const char *network_name,const char *filename, const char *extension,int options);
size_t FC_ReadFileToBuffer(FILE *fHan,char **lpptr);
void FC_print(const char *message);
int FC_ReadGeneralConfigFile(FC_MapStringString *mapConfig,const char *network_name,const char *file_name,const char *extension);
int FC_ReadParamArgs(FC_MapStringString *mapConfig,int argc, char* argv[],const char *prefix);
int FC_HexToBin(void *dest,const void *src,int len);
int FC_BinToHex(void *dest,const void *src,int len);
void FC_MemoryDump(const void *ptr,int from,int len);
void FC_MemoryDumpChar(const void *ptr,int from,int len);
void FC_Dump(const char * message,const void *ptr,int size);
void FC_MemoryDumpCharSize(const void *ptr,int from,int len,int row_size);
void FC_MemoryDumpCharSizeToFile(FILE *fHan,const void *ptr, int from, int len,int row_size);          
void FC_DumpSize(const char * message,const void *ptr,int size,int row_size);
void FC_RandomSeed(unsigned int seed);
double FC_RandomDouble();
unsigned int FC_RandomInRange(unsigned int min,unsigned int max);
unsigned int FC_TimeNowAsUInt();
double FC_TimeNowAsDouble();
int FC_GetFullFileName(const char *network_name,const char *filename, const char *extension,int options,char *buf);
int64_t FC_GetVarInt(const unsigned char *buf,int max_size,int64_t default_value,int* shift);
int FC_PutVarInt(unsigned char *buf,int max_size,int64_t value);

void FC_GetCompoundHash160(void *result,const void  *hash1,const void  *hash2);
int FC_SetIPv4ServerAddress(const char* host);
int FC_FindIPv4ServerAddress(uint32_t *all_ips,int max_ips);
int FC_GenerateConfFiles(const char *network_name);
void FC_RemoveDataDir(const char *network_name);
void FC_RemoveDir(const char *network_name,const char *dir_name);
int FC_GetDataDirArg(char *buf);
void FC_UnsetDataDirArg();
void FC_SetDataDirArg(char *buf);
void FC_ExpandDataDirParam();
void FC_CheckDataDirInConfFile();
void FC_AdjustStartAndCount(int *count,int *start,int size);


int FC_TestScenario(char* scenario_file);

int FC_StringToArg(char *src,char *dest);
int FC_SaveCliCommandToLog(const char *fileName, int argc, char* argv[]);

void FC_StringLowerCase(char *buf,uint32_t len);
int FC_StringCompareCaseInsensitive(const char *str1,const char *str2,int len);

uint32_t FC_GetParamFromDetailsScript(const unsigned char *ptr,uint32_t total,uint32_t offset,uint32_t* param_value_start,size_t *bytes);
uint32_t FC_FindSpecialParamInDetailsScript(const unsigned char *ptr,uint32_t total,uint32_t param,size_t *bytes);
uint32_t FC_FindNamedParamInDetailsScript(const unsigned char *ptr,uint32_t total,const char *param,size_t *bytes);

const unsigned char *FC_ParseOpDropOpReturnScript(const unsigned char *src,int size,int *op_drop_offset,int *op_drop_size,int op_drop_count,int *op_return_offset,int *op_return_size);
const unsigned char *FC_ExtractAddressFromInputScript(const unsigned char *src,int size,int *op_addr_offset,int *op_addr_size,int* is_redeem_script,int* sighash_type,int check_last);

void FC_LogString(FILE *fHan, const char* message);

const char *FC_Version();
const char *FC_FullVersion();
void __US_Sleep (int dwMilliseconds);
int __US_Fork(char *lpCommandLine,int WinState,int *lpChildPID);
int __US_BecomeDaemon();
void __US_Dummy();
void* __US_SeFCreate();
void __US_SemWait(void* sem);
void __US_SemPost(void* sem);
void __US_SemDestroy(void* sem);
uint64_t __US_ThreadID();
const char* __US_UserHomeDir();
char * __US_FullPath(const char* path, char *full_path, int len);
void __US_FlushFile(int FileHan);



#ifdef	__cplusplus
}
#endif


#endif	/* FLOATCHAIN_DECLARE_H */

