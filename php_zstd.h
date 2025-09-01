/*
  Copyright (c) 2015 kjdev

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  'Software'), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to
  the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef PHP_ZSTD_H
#define PHP_ZSTD_H

#define PHP_ZSTD_VERSION "0.15.2"
#define PHP_ZSTD_NS "Zstd"

extern zend_module_entry zstd_module_entry;
#define phpext_zstd_ptr &zstd_module_entry

#ifdef PHP_WIN32
#   define PHP_ZSTD_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#   define PHP_ZSTD_API __attribute__ ((visibility("default")))
#else
#   define PHP_ZSTD_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

typedef struct _php_zstd_context php_zstd_context;

#if PHP_VERSION_ID >= 80000
ZEND_BEGIN_MODULE_GLOBALS(zstd)
    zend_long output_compression;
    zend_long output_compression_default;
    zend_long output_compression_level;
    char *output_compression_dict;
    php_zstd_context *ob_handler;
    bool handler_registered;
    int compression_coding;
ZEND_END_MODULE_GLOBALS(zstd);
#endif

#ifdef ZTS
#define PHP_ZSTD_G(v) TSRMG(zstd_globals_id, zend_zstd_globals *, v)
#else
#define PHP_ZSTD_G(v) (zstd_globals.v)
#endif

#endif  /* PHP_ZSTD_H */
