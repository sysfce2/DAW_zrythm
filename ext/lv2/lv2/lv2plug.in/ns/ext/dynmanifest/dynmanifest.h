// Copyright 2008-2011 Stefano D'Angelo <zanga.mail@gmail.com>
// SPDX-License-Identifier: ISC

#ifndef LV2_DYNMANIFEST_DYNMANIFEST_H
#define LV2_DYNMANIFEST_DYNMANIFEST_H

/**
   @defgroup dynmanifest Dynamic Manifest
   @ingroup lv2

   Support for dynamic data generation.

   See <http://lv2plug.in/ns/ext/dynmanifest> for details.

   @{
*/

#include <lv2/core/lv2.h>

#include <stdio.h>

// clang-format off

#define LV2_DYN_MANIFEST_URI    "http://lv2plug.in/ns/ext/dynmanifest"  ///< http://lv2plug.in/ns/ext/dynmanifest
#define LV2_DYN_MANIFEST_PREFIX LV2_DYN_MANIFEST_URI "#"                ///< http://lv2plug.in/ns/ext/dynmanifest#

// clang-format on

#ifdef __cplusplus
extern "C" {
#endif

/**
   Dynamic manifest generator handle.

   This handle indicates a particular status of a dynamic manifest generator.
   The host MUST NOT attempt to interpret it and, unlikely LV2_Handle, it is
   NOT even valid to compare this to NULL. The dynamic manifest generator MAY
   use it to reference internal data.
*/
typedef void* LV2_Dyn_Manifest_Handle;

/**
   Generate the dynamic manifest.

   @param handle Pointer to an uninitialized dynamic manifest generator handle.

   @param features NULL terminated array of LV2_Feature structs which represent
   the features the host supports. The dynamic manifest generator may refuse to
   (re)generate the dynamic manifest if required features are not found here
   (however hosts SHOULD NOT use this as a discovery mechanism, instead of
   reading the static manifest file). This array must always exist; if a host
   has no features, it MUST pass a single element array containing NULL.

   @return 0 on success, otherwise a non-zero error code. The host SHOULD
   evaluate the result of the operation by examining the returned value and
   MUST NOT try to interpret the value of handle.
*/
LV2_SYMBOL_EXPORT int
lv2_dyn_manifest_open(LV2_Dyn_Manifest_Handle*  handle,
                      const LV2_Feature* const* features);

/**
   Fetch a "list" of subject URIs described in the dynamic manifest.

   The dynamic manifest generator has to fill the resource only with the needed
   triples to make the host aware of the "objects" it wants to expose. For
   example, if the plugin library exposes a regular LV2 plugin, it should
   output only a triple like the following:

   <http://example.org/plugin> a lv2:Plugin .

   The objects that are eligible for exposure are those that would need to be
   represented by a subject node in a static manifest.

   @param handle Dynamic manifest generator handle.

   @param fp FILE * identifying the resource the host has to set up for the
   dynamic manifest generator. The host MUST pass a writable, empty resource to
   this function, and the dynamic manifest generator MUST ONLY perform write
   operations on it at the end of the stream (for example, using only
   fprintf(), fwrite() and similar).

   @return 0 on success, otherwise a non-zero error code.
*/
LV2_SYMBOL_EXPORT int
lv2_dyn_manifest_get_subjects(LV2_Dyn_Manifest_Handle handle, FILE* fp);

/**
   Function that fetches data related to a specific URI.

   The dynamic manifest generator has to fill the resource with data related to
   object represented by the given URI. For example, if the library exposes a
   regular LV2 plugin whose URI, as retrieved by the host using
   lv2_dyn_manifest_get_subjects() is http://example.org/plugin then it
   should output something like:

   <pre>
   <http://example.org/plugin>
       a lv2:Plugin ;
       doap:name "My Plugin" ;
       lv2:binary <mylib.so> ;
       etc:etc "..." .
   </pre>

   @param handle Dynamic manifest generator handle.

   @param fp FILE * identifying the resource the host has to set up for the
   dynamic manifest generator. The host MUST pass a writable resource to this
   function, and the dynamic manifest generator MUST ONLY perform write
   operations on it at the current position of the stream (for example, using
   only fprintf(), fwrite() and similar).

   @param uri URI to get data about (in the "plain" form, i.e., absolute URI
   without Turtle prefixes).

   @return 0 on success, otherwise a non-zero error code.
*/
LV2_SYMBOL_EXPORT int
lv2_dyn_manifest_get_data(LV2_Dyn_Manifest_Handle handle,
                          FILE*                   fp,
                          const char*             uri);

/**
   Function that ends the operations on the dynamic manifest generator.

   This function SHOULD be used by the dynamic manifest generator to perform
   cleanup operations, etc.

   Once this function is called, referring to handle will cause undefined
   behavior.

   @param handle Dynamic manifest generator handle.
*/
LV2_SYMBOL_EXPORT void
lv2_dyn_manifest_close(LV2_Dyn_Manifest_Handle handle);

#ifdef __cplusplus
} /* extern "C" */
#endif

/**
   @}
*/

#endif // LV2_DYNMANIFEST_DYNMANIFEST_H
