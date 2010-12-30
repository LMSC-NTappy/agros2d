/*
   This header file contains platform compatibility layer.

   It is included from common.h, so it is automatically included in all hermes
   sources. The implementation of the functions in this file is in the
   src/compat directory.
*/

#ifndef __HERMES_COMPAT_H
#define __HERMES_COMPAT_H
#include <stdio.h>

/// Implementation of GNU fmemopen. Intended to be used if the current platform does not support it.
FILE *fmemopen (void *buf, size_t size, const char *opentype);

// Windows DLL export/import definitions
#define HERMES_API
#define HERMES_API_USED_TEMPLATE(__implementation)
#define HERMES_API_USED_STL_VECTOR(__type)

#ifndef HAVE_STRCASECMP
#define strcasecmp strcmp
#endif

//C99 functions
#include "compat/c99_functions.h"

#ifdef __GNUC__
#define NORETURN __attribute__((noreturn))
#else
#define NORETURN
#ifndef __attribute__
#define __attribute__(x)
#endif
#endif

#endif
