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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <php.h>
#include <php_ini.h>
#include <ext/standard/info.h>
#if ZEND_MODULE_API_NO >= 20141001
#include <ext/standard/php_smart_string.h>
#else
#include <ext/standard/php_smart_str.h>
#endif
#include "php_zstd.h"

/* zstd */
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#include "zstd.h"

#define FRAME_HEADER_SIZE 5
#define BLOCK_HEADER_SIZE 3
#define MAX_HEADER_SIZE FRAME_HEADER_SIZE+3


ZEND_BEGIN_ARG_INFO_EX(arginfo_zstd_compress, 0, 0, 1)
    ZEND_ARG_INFO(0, data)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_zstd_uncompress, 0, 0, 1)
    ZEND_ARG_INFO(0, data)
ZEND_END_ARG_INFO()


ZEND_FUNCTION(zstd_compress)
{
    zval *data;
    char *output;
    size_t len, size, result;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
                              "z", &data) == FAILURE) {
        RETURN_FALSE;
    }

    if (Z_TYPE_P(data) != IS_STRING) {
        zend_error(E_WARNING, "zstd_compress: expects parameter to be string.");
        RETURN_FALSE;
    }

    size = ZSTD_compressBound(Z_STRLEN_P(data));
    output = (char *)emalloc(size + 1);
    if (!output) {
        zend_error(E_WARNING, "zstd_compress: memory error");
        RETURN_FALSE;
    }

    result = ZSTD_compress(output, size, Z_STRVAL_P(data), Z_STRLEN_P(data), 1);

    if (ZSTD_isError(result)) {
        RETVAL_FALSE;
    } else if (result <= 0) {
        RETVAL_FALSE;
    } else {
#if ZEND_MODULE_API_NO >= 20141001
        RETVAL_STRINGL(output, result);
#else
        RETVAL_STRINGL(output, result, 1);
#endif
    }

    efree(output);
}

ZEND_FUNCTION(zstd_uncompress)
{
    zval *data;
    unsigned long long size;
    size_t result;
    void *output;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
                              "z", &data) == FAILURE) {
        RETURN_FALSE;
    }

    if (Z_TYPE_P(data) != IS_STRING) {
        zend_error(E_WARNING,
                   "zstd_uncompress: expects parameter to be string.");
        RETURN_FALSE;
    }

    size = ZSTD_getDecompressedSize(Z_STRVAL_P(data), Z_STRLEN_P(data));
    output = emalloc(size);
    if (!output) {
        zend_error(E_WARNING, "zstd_uncompress: memory error");
        RETURN_FALSE;
    }

    result = ZSTD_decompress(output, size, Z_STRVAL_P(data), Z_STRLEN_P(data));

    if (ZSTD_isError(result)) {
        RETVAL_FALSE;
    } else if (result <= 0) {
        RETVAL_FALSE;
    } else {
#if ZEND_MODULE_API_NO >= 20141001
        RETVAL_STRINGL(output, result);
#else
        RETVAL_STRINGL(output, result, 1);
#endif
    }

    efree(output);
}


ZEND_MINFO_FUNCTION(zstd)
{
    char buffer[128];
    php_info_print_table_start();
    php_info_print_table_row(2, "Zstd support", "enabled");
    php_info_print_table_row(2, "Extension Version", PHP_ZSTD_EXT_VERSION);
    snprintf(buffer, 128, "%d.%d.%d",
             ZSTD_VERSION_MAJOR, ZSTD_VERSION_MINOR, ZSTD_VERSION_RELEASE);
    php_info_print_table_row(2, "Interface Version", buffer);
    php_info_print_table_end();
}

static zend_function_entry zstd_functions[] = {
    ZEND_FE(zstd_compress, arginfo_zstd_compress)
    ZEND_FE(zstd_uncompress, arginfo_zstd_uncompress)
    ZEND_NS_FALIAS(PHP_ZSTD_NS, compress,
                   zstd_compress, arginfo_zstd_compress)
    ZEND_NS_FALIAS(PHP_ZSTD_NS, uncompress,
                   zstd_uncompress, arginfo_zstd_uncompress)
    ZEND_FE_END
};

zend_module_entry zstd_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    "zstd",
    zstd_functions,
    NULL,
    NULL,
    NULL,
    NULL,
    ZEND_MINFO(zstd),
#if ZEND_MODULE_API_NO >= 20010901
    PHP_ZSTD_EXT_VERSION,
#endif
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_ZSTD
ZEND_GET_MODULE(zstd)
#endif
