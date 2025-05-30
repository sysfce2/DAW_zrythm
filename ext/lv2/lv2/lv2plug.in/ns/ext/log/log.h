// Copyright 2012-2016 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef LV2_LOG_LOG_H
#define LV2_LOG_LOG_H

/**
   @defgroup log Log
   @ingroup lv2

   Interface for plugins to log via the host.

   See <http://lv2plug.in/ns/ext/log> for details.

   @{
*/

// clang-format off

#define LV2_LOG_URI    "http://lv2plug.in/ns/ext/log"  ///< http://lv2plug.in/ns/ext/log
#define LV2_LOG_PREFIX LV2_LOG_URI "#"                 ///< http://lv2plug.in/ns/ext/log#

#define LV2_LOG__Entry   LV2_LOG_PREFIX "Entry"    ///< http://lv2plug.in/ns/ext/log#Entry
#define LV2_LOG__Error   LV2_LOG_PREFIX "Error"    ///< http://lv2plug.in/ns/ext/log#Error
#define LV2_LOG__Note    LV2_LOG_PREFIX "Note"     ///< http://lv2plug.in/ns/ext/log#Note
#define LV2_LOG__Trace   LV2_LOG_PREFIX "Trace"    ///< http://lv2plug.in/ns/ext/log#Trace
#define LV2_LOG__Warning LV2_LOG_PREFIX "Warning"  ///< http://lv2plug.in/ns/ext/log#Warning
#define LV2_LOG__log     LV2_LOG_PREFIX "log"      ///< http://lv2plug.in/ns/ext/log#log

// clang-format on

#include <lv2/urid/urid.h>

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond */
#ifdef __GNUC__
/** Allow type checking of printf-like functions. */
#  define LV2_LOG_FUNC(fmt, arg1) __attribute__((format(printf, fmt, arg1)))
#else
#  define LV2_LOG_FUNC(fmt, arg1)
#endif
/** @endcond */

/**
   Opaque data to host data for LV2_Log_Log.
*/
typedef void* LV2_Log_Handle;

/**
   Log feature (LV2_LOG__log)
*/
typedef struct {
  /**
     Opaque pointer to host data.

     This MUST be passed to methods in this struct whenever they are called.
     Otherwise, it must not be interpreted in any way.
  */
  LV2_Log_Handle handle;

  /**
     Log a message, passing format parameters directly.

     The API of this function matches that of the standard C printf function,
     except for the addition of the first two parameters.  This function may
     be called from any non-realtime context, or from any context if `type`
     is @ref LV2_LOG__Trace.
  */
  LV2_LOG_FUNC(3, 4)
  int (*printf)(LV2_Log_Handle handle, LV2_URID type, const char* fmt, ...);

  /**
     Log a message, passing format parameters in a va_list.

     The API of this function matches that of the standard C vprintf
     function, except for the addition of the first two parameters.  This
     function may be called from any non-realtime context, or from any
     context if `type` is @ref LV2_LOG__Trace.
  */
  LV2_LOG_FUNC(3, 0)
  int (*vprintf)(LV2_Log_Handle handle,
                 LV2_URID       type,
                 const char*    fmt,
                 va_list        ap);
} LV2_Log_Log;

#ifdef __cplusplus
} /* extern "C" */
#endif

/**
   @}
*/

#endif // LV2_LOG_LOG_H
