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
#include "zstd/lib/zstd.h"
#include "zstd/lib/zstd_static.h"

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

    result = ZSTD_compress(output, size, Z_STRVAL_P(data), Z_STRLEN_P(data));

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
#if ZEND_MODULE_API_NO >= 20141001
    smart_string decomp = {0};
#else
    smart_str decomp = {0};
#endif
    char *input;
    char header[MAX_HEADER_SIZE];
    char *in, *out, *op, *end;
    uint32_t block_size = 128 * (1 << 10);
    size_t in_size = block_size + BLOCK_HEADER_SIZE;
    size_t out_size = 4 * block_size;
    size_t read, size;
    ZSTD_cctx_t dctx;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
                              "z", &data) == FAILURE) {
        RETURN_FALSE;
    }

    if (Z_TYPE_P(data) != IS_STRING) {
        zend_error(E_WARNING,
                   "zstd_uncompress: expects parameter to be string.");
        RETURN_FALSE;
    }

    input = Z_STRVAL_P(data);

    dctx = ZSTD_createDCtx();

    /* check header */
    read = ZSTD_nextSrcSizeToDecompress(dctx);
    if (read > MAX_HEADER_SIZE) {
        zend_error(E_WARNING,
                   "zstd_uncompress: not enough memory to read header");
        ZSTD_freeDCtx(dctx);
        RETURN_FALSE;
    }

    memcpy(header, input, read);
    input += read;

    size = ZSTD_decompressContinue(dctx, NULL, 0, header, read);
    if (ZSTD_isError(size)) {
        zend_error(E_WARNING, "zstd_uncompress: decoding header error");
        ZSTD_freeDCtx(dctx);
        RETURN_FALSE;
    }

    /* allocate memory */
    in = (char *)emalloc(in_size);
    if (!in) {
        zend_error(E_WARNING, "zstd_compress: allocation memory error");
        ZSTD_freeDCtx(dctx);
        RETURN_FALSE;
    }

    out = (char *)emalloc(out_size);
    if (!out) {
        zend_error(E_WARNING, "zstd_compress: allocation memory error");
        efree(in);
        ZSTD_freeDCtx(dctx);
        RETURN_FALSE;
    }

    op = out;
    end = out + out_size;

    /* decompression loop */
    read = ZSTD_nextSrcSizeToDecompress(dctx);
    while (read) {
        size_t read_size, decoded_size;

        memcpy(in, input, read);
        input += read;
        read_size = read;

        decoded_size = ZSTD_decompressContinue(dctx, op, end - op,
                                               in, read_size);
        if (decoded_size) {
#if ZEND_MODULE_API_NO >= 20141001
            smart_string_appendl(&decomp, op, decoded_size);
#else
            smart_str_appendl(&decomp, op, decoded_size);
#endif
            op += decoded_size;
            if (op == end) {
                op = out;
            }
        }

        read = ZSTD_nextSrcSizeToDecompress(dctx);
    }

    efree(in);
    efree(out);

    ZSTD_freeDCtx(dctx);

    if (decomp.len) {
#if ZEND_MODULE_API_NO >= 20141001
        RETVAL_STRINGL(decomp.c, decomp.len);
#else
        RETVAL_STRINGL(decomp.c, decomp.len, 1);
#endif
    } else {
        RETVAL_FALSE;
    }

#if ZEND_MODULE_API_NO >= 20141001
    smart_string_free(&decomp);
#else
    smart_str_free(&decomp);
#endif
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
