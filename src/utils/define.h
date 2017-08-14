// Copyright (c) 2014-2017 Coin Sciences Ltd
// FloatChain code distributed under the GPLv3 license, see COPYING file.

#ifndef FLOATCHAIN_DEFINE_H
#define	FLOATCHAIN_DEFINE_H

#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#ifdef MAC_OSX
#define lseek64 lseek
#endif

#ifndef WIN32

#include <unistd.h>
#define _O_BINARY 0
#include <sys/resource.h>

#else

#include <io.h>

#endif

#include <fcntl.h>
#include <sys/file.h>
#include <sys/types.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h> 
#include <stdlib.h>
#include <math.h>

/* Error codes */

#define FC_ERR_NOERROR                  0x00000000

/* General errors */

#define FC_ERR_ALLOCATION               0x00000001
#define FC_ERR_TOO_FEW_PARAMETERS       0x00000002
#define FC_ERR_MISSING_PARAMETER        0x00000003
#define FC_ERR_OPERATION_NOT_SUPPORTED  0x00000004
#define FC_ERR_INVALID_PARAMETER_VALUE  0x00000005
#define FC_ERR_INTERNAL_ERROR           0x00000006
#define FC_ERR_FILE_READ_ERROR          0x00000007
#define FC_ERR_FILE_WRITE_ERROR         0x00000008
#define FC_ERR_CONNECTION_ERROR         0x00000009
#define FC_ERR_DBOPEN_ERROR             0x0000000A
#define FC_ERR_CORRUPTED                0x0000000B
#define FC_ERR_NOT_ALLOWED              0x0000000C
#define FC_ERR_WRONG_SCRIPT             0x0000000D
#define FC_ERR_FOUND                    0x0000000E
#define FC_ERR_NOT_FOUND                0x0000000F
#define FC_ERR_NOT_SUPPORTED            0x00000010
#define FC_ERR_ERROR_IN_SCRIPT          0x00000011


#define FC_FOM_NONE                     0x00000000
#define FC_FOM_RELATIVE_TO_DATADIR      0x00000001
#define FC_FOM_RELATIVE_MASK            0x0000000F
#define FC_FOM_CREATE_DIR               0x00000100

#define FC_BUF_MODE_DEFAULT             0x00000000
#define FC_BUF_MODE_MAP                 0x00000001


#define FC_PRM_NETWORK_NAME_MAX_SIZE    32

#endif	/* FLOATCHAIN_DEFINE_H */

