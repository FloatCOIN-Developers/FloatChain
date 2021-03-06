// Copyright (c) 2014-2017 Coin Sciences Ltd
// FloatChain code distributed under the GPLv3 license, see COPYING file.

#ifndef FLOATCHAINVERSION_H
#define	FLOATCHAINVERSION_H

#define FLOATCHAIN_VERSION_MAJOR     1
#define FLOATCHAIN_VERSION_MINOR     0
#define FLOATCHAIN_VERSION_REVISION  0
#define FLOATCHAIN_VERSION_STAGE     2
#define FLOATCHAIN_VERSION_BUILD     2

#define FLOATCHAIN_PROTOCOL_VERSION 10008


#ifndef STRINGIZE
#define STRINGIZE(X) DO_STRINGIZE(X)
#endif

#ifndef DO_STRINGIZE
#define DO_STRINGIZE(X) #X
#endif

#define FLOATCHAIN_BUILD_DESC_WITH_SUFFIX(maj, min, rev, build, suffix) \
    DO_STRINGIZE(maj) "." DO_STRINGIZE(min) "." DO_STRINGIZE(rev) "." DO_STRINGIZE(build) "-" DO_STRINGIZE(suffix)

#define FLOATCHAIN_BUILD_DESC_FROM_UNKNOWN(maj, min, rev, build) \
    DO_STRINGIZE(maj) "." DO_STRINGIZE(min) "." DO_STRINGIZE(rev) "." DO_STRINGIZE(build)


#define FLOATCHAIN_BUILD_DESC "1.0 beta 2"
#define FLOATCHAIN_BUILD_DESC_NUMERIC 10000202

#ifndef FLOATCHAIN_BUILD_DESC
#ifdef BUILD_SUFFIX
#define FLOATCHAIN_BUILD_DESC FLOATCHAIN_BUILD_DESC_WITH_SUFFIX(FLOATCHAIN_VERSION_MAJOR, FLOATCHAIN_VERSION_MINOR, FLOATCHAIN_VERSION_REVISION, FLOATCHAIN_VERSION_BUILD, BUILD_SUFFIX)
#else
#define FLOATCHAIN_BUILD_DESC FLOATCHAIN_BUILD_DESC_FROM_UNKNOWN(FLOATCHAIN_VERSION_MAJOR, FLOATCHAIN_VERSION_MINOR, FLOATCHAIN_VERSION_REVISION, FLOATCHAIN_VERSION_BUILD)
#endif
#endif

#define FLOATCHAIN_FULL_DESC(build, protocol) \
    "build " build " protocol " DO_STRINGIZE(protocol)


#ifndef FLOATCHAIN_FULL_VERSION
#define FLOATCHAIN_FULL_VERSION FLOATCHAIN_FULL_DESC(FLOATCHAIN_BUILD_DESC, FLOATCHAIN_PROTOCOL_VERSION)
#endif


#endif	/* FLOATCHAINVERSION_H */

