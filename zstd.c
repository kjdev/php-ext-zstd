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

#define DEFAULT_COMPRESS_LEVEL 3

ZEND_BEGIN_ARG_INFO_EX(arginfo_zstd_compress, 0, 0, 1)
    ZEND_ARG_INFO(0, data)
    ZEND_ARG_INFO(0, level)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_zstd_uncompress, 0, 0, 1)
    ZEND_ARG_INFO(0, data)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_zstd_compress_dict, 0, 0, 1)
    ZEND_ARG_INFO(0, data)
    ZEND_ARG_INFO(0, dictBuffer)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_zstd_uncompress_dict, 0, 0, 1)
    ZEND_ARG_INFO(0, data)
    ZEND_ARG_INFO(0, dictBuffer)
ZEND_END_ARG_INFO()

ZEND_FUNCTION(zstd_compress)
{
    zval *data;
    char *output;
    size_t size, result;
    long level = DEFAULT_COMPRESS_LEVEL;
    uint16_t maxLevel = (uint16_t)ZSTD_maxCLevel();

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
                              "z|l", &data, &level) == FAILURE) {
        RETURN_FALSE;
    }

    if (Z_TYPE_P(data) != IS_STRING) {
        zend_error(E_WARNING, "zstd_compress: expects parameter to be string.");
        RETURN_FALSE;
    }

    if (level > maxLevel || level < 0) {
      zend_error(E_WARNING, "zstd_compress: compression level (%ld)"
                 " must be within 1..%d", level, maxLevel);
      RETURN_FALSE;
    } else if (level == 0) {
#if ZEND_MODULE_API_NO >= 20141001
      RETURN_STRINGL(Z_STRVAL_P(data), Z_STRLEN_P(data));
#else
      RETURN_STRINGL(Z_STRVAL_P(data), Z_STRLEN_P(data), 1);
#endif
    }

    size = ZSTD_compressBound(Z_STRLEN_P(data));
    output = (char *)emalloc(size + 1);
    if (!output) {
        zend_error(E_WARNING, "zstd_compress: memory error");
        RETURN_FALSE;
    }

    result = ZSTD_compress(output, size, Z_STRVAL_P(data), Z_STRLEN_P(data),
                           level);

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
    uint64_t size;
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

ZEND_FUNCTION(zstd_compress_dict)
{
    zval *data, *dictBuffer;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
                              "z|z", &data, &dictBuffer) == FAILURE) {
        RETURN_FALSE;
    }
    if (Z_TYPE_P(data) != IS_STRING) {
        zend_error(E_WARNING, "zstd_compress_dict:"
                   " expects the first parameter to be string.");
        RETURN_FALSE;
    }
    if (Z_TYPE_P(dictBuffer) != IS_STRING) {
        zend_error(E_WARNING, "zstd_compress_dict:"
                   " expects the second parameter to be string.");
        RETURN_FALSE;
    }

    size_t const cBuffSize = ZSTD_compressBound(Z_STRLEN_P(data));
    void* const cBuff = emalloc(cBuffSize);
    if (!cBuff) {
        zend_error(E_WARNING, "zstd_compress_dict: memory error");
        RETURN_FALSE;
    }
    ZSTD_CCtx* const cctx = ZSTD_createCCtx();
    if (cctx == NULL) {
        efree(cBuff);
        zend_error(E_WARNING, "ZSTD_createCCtx() error");
        RETURN_FALSE;
    }
    ZSTD_CDict* const cdict = ZSTD_createCDict(Z_STRVAL_P(dictBuffer),
                                               Z_STRLEN_P(dictBuffer),
                                               DEFAULT_COMPRESS_LEVEL);
    if (!cdict) {
        efree(cBuff);
        zend_error(E_WARNING, "ZSTD_createCDict() error");
        RETURN_FALSE;
    }
    size_t const cSize = ZSTD_compress_usingCDict(cctx, cBuff, cBuffSize,
                                                  Z_STRVAL_P(data),
                                                  Z_STRLEN_P(data),
                                                  cdict);
    if (ZSTD_isError(cSize)) {
        efree(cBuff);
        zend_error(E_WARNING, "zstd_compress_dict: %s",
                   ZSTD_getErrorName(cSize));
        RETURN_FALSE;
    }
    ZSTD_freeCCtx(cctx);

#if ZEND_MODULE_API_NO >= 20141001
    RETVAL_STRINGL(cBuff, cSize);
#else
    RETVAL_STRINGL(cBuff, cSize, 1);
#endif

    efree(cBuff);
}

ZEND_FUNCTION(zstd_uncompress_dict)
{
    zval *data, *dictBuffer;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
                              "z|z", &data, &dictBuffer) == FAILURE) {
        RETURN_FALSE;
    }
    if (Z_TYPE_P(data) != IS_STRING) {
        zend_error(E_WARNING, "zstd_uncompress_dict:"
                   " expects the first parameter to be string.");
        RETURN_FALSE;
    }
    if (Z_TYPE_P(dictBuffer) != IS_STRING) {
        zend_error(E_WARNING, "zstd_uncompress_dict:"
                   " expects the second parameter to be string.");
        RETURN_FALSE;
    }

    unsigned long long const rSize = ZSTD_getDecompressedSize(Z_STRVAL_P(data),
                                                              Z_STRLEN_P(data));
    if (rSize == 0) {
        RETURN_FALSE;
    }
    void* const rBuff = emalloc((size_t)rSize);
    if (!rBuff) {
        zend_error(E_WARNING, "zstd_uncompress_dict: memory error");
        RETURN_FALSE;
    }

    ZSTD_DCtx* const dctx = ZSTD_createDCtx();
    if (dctx == NULL) {
        efree(rBuff);
        zend_error(E_WARNING, "ZSTD_createDCtx() error");
        RETURN_FALSE;
    }
    ZSTD_DDict* const ddict = ZSTD_createDDict(Z_STRVAL_P(dictBuffer),
                                               Z_STRLEN_P(dictBuffer));
    if (!ddict) {
        efree(rBuff);
        zend_error(E_WARNING, "ZSTD_createDDict() error");
        RETURN_FALSE;
    }
    size_t const dSize = ZSTD_decompress_usingDDict(dctx, rBuff, rSize,
                                                    Z_STRVAL_P(data),
                                                    Z_STRLEN_P(data),
                                                    ddict);
    if (dSize != rSize) {
        efree(rBuff);
        zend_error(E_WARNING, "zstd_uncompress_dict: %s",
                   ZSTD_getErrorName(dSize));
        RETURN_FALSE;
    }
    ZSTD_freeDCtx(dctx);

#if ZEND_MODULE_API_NO >= 20141001
    RETVAL_STRINGL(rBuff, rSize);
#else
    RETVAL_STRINGL(rBuff, rSize, 1);
#endif

    efree(rBuff);
}

ZEND_MINFO_FUNCTION(zstd)
{
    char buffer[128];
    php_info_print_table_start();
    php_info_print_table_row(2, "Zstd support", "enabled");
    php_info_print_table_row(2, "Extension Version", PHP_ZSTD_EXT_VERSION);
    php_info_print_table_row(2, "Interface Version", ZSTD_VERSION_STRING);
    php_info_print_table_end();
}

static zend_function_entry zstd_functions[] = {
    ZEND_FE(zstd_compress, arginfo_zstd_compress)
    ZEND_FE(zstd_uncompress, arginfo_zstd_uncompress)
    ZEND_FALIAS(zstd_decompress, zstd_uncompress, arginfo_zstd_uncompress)

    ZEND_FE(zstd_compress_dict, arginfo_zstd_compress_dict)
    ZEND_FE(zstd_uncompress_dict, arginfo_zstd_uncompress_dict)
    ZEND_FALIAS(zstd_compress_usingcdict,
                zstd_compress_dict, arginfo_zstd_compress_dict)
    ZEND_FALIAS(zstd_decompress_dict,
                zstd_uncompress_dict, arginfo_zstd_uncompress_dict)
    ZEND_FALIAS(zstd_uncompress_usingcdict,
                zstd_uncompress_dict, arginfo_zstd_uncompress_dict)
    ZEND_FALIAS(zstd_decompress_usingcdict,
                zstd_uncompress_dict, arginfo_zstd_uncompress_dict)

// PHP 5.3+
#if ZEND_MODULE_API_NO >= 20090626
    ZEND_NS_FALIAS(PHP_ZSTD_NS, compress,
                   zstd_compress, arginfo_zstd_compress)
    ZEND_NS_FALIAS(PHP_ZSTD_NS, uncompress,
                   zstd_uncompress, arginfo_zstd_uncompress)
    ZEND_NS_FALIAS(PHP_ZSTD_NS, decompress,
                   zstd_uncompress, arginfo_zstd_uncompress)
    ZEND_NS_FALIAS(PHP_ZSTD_NS, compress_dict,
                   zstd_compress_dict, arginfo_zstd_compress_dict)
    ZEND_NS_FALIAS(PHP_ZSTD_NS, compress_usingcdict,
                   zstd_compress_dict, arginfo_zstd_compress_dict)
    ZEND_NS_FALIAS(PHP_ZSTD_NS, uncompress_dict,
                   zstd_uncompress_dict, arginfo_zstd_uncompress_dict)
    ZEND_NS_FALIAS(PHP_ZSTD_NS, decompress_dict,
                   zstd_uncompress_dict, arginfo_zstd_uncompress_dict)
    ZEND_NS_FALIAS(PHP_ZSTD_NS, uncompress_usingcdict,
                   zstd_uncompress_dict, arginfo_zstd_uncompress_dict)
    ZEND_NS_FALIAS(PHP_ZSTD_NS, decompress_usingcdict,
                   zstd_uncompress_dict, arginfo_zstd_uncompress_dict)
#endif
    {NULL, NULL, NULL}
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
