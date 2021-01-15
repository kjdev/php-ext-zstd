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
#if PHP_MAJOR_VERSION >= 7 && defined(HAVE_APCU_SUPPORT)
#include <ext/standard/php_var.h>
#include <ext/apcu/apc_serializer.h>
#include <zend_smart_str.h>
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

#ifndef ZSTD_CLEVEL_DEFAULT
#define ZSTD_CLEVEL_DEFAULT 3
#endif

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


#if ZSTD_VERSION_NUMBER >= 10304
    if (level > maxLevel) {
      zend_error(E_WARNING, "zstd_compress: compression level (%ld)"
                 " must be within 1..%d or smaller then 0", level, maxLevel);
#else
    if (level > maxLevel || level < 0) {
      zend_error(E_WARNING, "zstd_compress: compression level (%ld)"
                 " must be within 1..%d", level, maxLevel);
#endif
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
    uint8_t streaming = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,
                              "z", &data) == FAILURE) {
        RETURN_FALSE;
    }

    if (Z_TYPE_P(data) != IS_STRING) {
        zend_error(E_WARNING,
                   "zstd_uncompress: expects parameter to be string.");
        RETURN_FALSE;
    }

    size = ZSTD_getFrameContentSize(Z_STRVAL_P(data), Z_STRLEN_P(data));
    if (size == ZSTD_CONTENTSIZE_ERROR) {
        zend_error(E_WARNING, "zstd_uncompress: it was not compressed by zstd");
        RETURN_FALSE;
    } else if (size == ZSTD_CONTENTSIZE_UNKNOWN) {
        streaming = 1;
        size = ZSTD_DStreamOutSize();
    }

    output = emalloc(size);
    if (!output) {
        zend_error(E_WARNING, "zstd_uncompress: memory error");
        RETURN_FALSE;
    }

    if (!streaming) {
        result = ZSTD_decompress(output, size,
                                 Z_STRVAL_P(data), Z_STRLEN_P(data));
    } else {
        ZSTD_DStream *stream;
        ZSTD_inBuffer in = { NULL, 0, 0 };
        ZSTD_outBuffer out = { NULL, 0, 0 };

        stream = ZSTD_createDStream();
        if (stream == NULL) {
            efree(output);
            zend_error(E_WARNING, "zstd_uncompress: can not create stream");
            RETURN_FALSE;
        }

        result = ZSTD_initDStream(stream);
        if (ZSTD_isError(result)) {
            efree(output);
            ZSTD_freeDStream(stream);
            zend_error(E_WARNING, "zstd_uncompress: can not init stream");
            RETURN_FALSE;
        }

        in.src = Z_STRVAL_P(data);
        in.size = Z_STRLEN_P(data);
        in.pos = 0;

        out.dst = output;
        out.size = size;
        out.pos = 0;

        while (in.pos < in.size) {
            if (out.pos == out.size) {
                out.size += size;
                output = erealloc(output, out.size);
                out.dst = output;
            }

            result = ZSTD_decompressStream(stream, &out, &in);
            if (ZSTD_isError(result)) {
                efree(output);
                ZSTD_freeDStream(stream);
                zend_error(E_WARNING,
                           "zstd_uncompress: can not decompress stream");
                RETURN_FALSE;
            }

            if (result == 0) {
                break;
            }
        }

        result = out.pos;

        ZSTD_freeDStream(stream);
    }

    if (ZSTD_isError(result)) {
        RETVAL_FALSE;
    } else if (result < 0) {
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
    ZSTD_freeCDict(cdict);

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
    ZSTD_freeDDict(ddict);

#if ZEND_MODULE_API_NO >= 20141001
    RETVAL_STRINGL(rBuff, rSize);
#else
    RETVAL_STRINGL(rBuff, rSize, 1);
#endif

    efree(rBuff);
}


typedef struct _php_zstd_stream_data {
    char *bufin, *bufout;
    size_t sizein, sizeout;
    ZSTD_CCtx* cctx;
    ZSTD_DCtx* dctx;
    ZSTD_inBuffer input;
    ZSTD_outBuffer output;
    php_stream *stream;
} php_zstd_stream_data;


#define STREAM_DATA_FROM_STREAM() \
    php_zstd_stream_data *self = (php_zstd_stream_data *) stream->abstract

#define STREAM_NAME "compress.zstd"

static int php_zstd_decomp_close(php_stream *stream, int close_handle TSRMLS_DC)
{
    STREAM_DATA_FROM_STREAM();

    if (!self) {
        return EOF;
    }

    if (close_handle) {
        if (self->stream) {
            php_stream_close(self->stream);
            self->stream = NULL;
        }
    }

    ZSTD_freeDCtx(self->dctx);
    efree(self->bufin);
    efree(self->bufout);
    efree(self);
    stream->abstract = NULL;

    return EOF;
}

static int php_zstd_comp_flush_or_end(php_zstd_stream_data *self, int end TSRMLS_DC)
{
    size_t res;
    int ret = 0;

#if ZSTD_VERSION_NUMBER < 10400
    /* Compress remaining data */
    if (self->input.size)  {
        self->input.pos = 0;
        do {
            self->output.size = self->sizeout;
            self->output.pos  = 0;
            res = ZSTD_compressStream(self->cctx, &self->output, &self->input);
            if (ZSTD_isError(res)) {
                php_error_docref(NULL TSRMLS_CC, E_WARNING, "libzstd error %s\n", ZSTD_getErrorName(res));
                ret = EOF;
            }
            php_stream_write(self->stream, self->bufout, self->output.pos);
        } while (self->input.pos != self->input.size);
    }
#endif

    /* Flush / End */
    do {
        self->output.size = self->sizeout;
        self->output.pos  = 0;
#if ZSTD_VERSION_NUMBER >= 10400
        res = ZSTD_compressStream2(self->cctx, &self->output, &self->input, end ? ZSTD_e_end : ZSTD_e_flush);
#else
        if (end) {
            res = ZSTD_endStream(self->cctx, &self->output);
        } else {
            res = ZSTD_flushStream(self->cctx, &self->output);
        }
#endif
        if (ZSTD_isError(res)) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "libzstd error %s\n", ZSTD_getErrorName(res));
            ret = EOF;
        }
        php_stream_write(self->stream, self->bufout, self->output.pos);
    } while (res > 0);

    self->input.pos = 0;
    self->input.size = 0;

    return ret;
}


static int php_zstd_comp_flush(php_stream *stream TSRMLS_DC)
{
    STREAM_DATA_FROM_STREAM();

    return php_zstd_comp_flush_or_end(self, 0 TSRMLS_CC);
}


static int php_zstd_comp_close(php_stream *stream, int close_handle TSRMLS_DC)
{
    STREAM_DATA_FROM_STREAM();

    if (!self) {
        return EOF;
    }

    php_zstd_comp_flush_or_end(self, 1 TSRMLS_CC);

    if (close_handle) {
        if (self->stream) {
            php_stream_close(self->stream);
            self->stream = NULL;
        }
    }

    ZSTD_freeCCtx(self->cctx);
    efree(self->bufin);
    efree(self->bufout);
    efree(self);
    stream->abstract = NULL;

    return EOF;
}


#if PHP_VERSION_ID < 70400
static size_t php_zstd_decomp_read(php_stream *stream, char *buf, size_t count TSRMLS_DC)
{
    size_t ret = 0;
#else
static ssize_t php_zstd_decomp_read(php_stream *stream, char *buf, size_t count TSRMLS_DC)
{
    ssize_t ret = 0;
#endif
    size_t x, res;
    STREAM_DATA_FROM_STREAM();

    while (count > 0) {
        x = self->output.size - self->output.pos;
        /* enough available */
        if (x >= count) {
            memcpy(buf, self->bufout + self->output.pos, count);
            self->output.pos += count;
            ret += count;
            return ret;
        }
        /* take remaining from out  */
        if (x) {
            memcpy(buf, self->bufout + self->output.pos, x);
            self->output.pos += x;
            ret += x;
            buf += x;
            count -= x;
        }
        /* decompress */
        if (self->input.pos < self->input.size) {
            /* for zstd */
            self->output.pos = 0;
            self->output.size = self->sizeout;
            res = ZSTD_decompressStream(self->dctx, &self->output , &self->input);
            if (ZSTD_isError(res)) {
                php_error_docref(NULL TSRMLS_CC, E_WARNING, "libzstd error %s\n", ZSTD_getErrorName(res));
#if PHP_VERSION_ID >= 70400
                return -1;
#endif
            }
            /* for us */
            self->output.size = self->output.pos;
            self->output.pos = 0;
        }  else {
            /* read */
            self->input.pos = 0;
            self->input.size = php_stream_read(self->stream, self->bufin, self->sizein);
            if (!self->input.size) {
                /* EOF */
                count = 0;
            }
        }
    }
    return ret;
}


#if PHP_VERSION_ID < 70400
static size_t php_zstd_comp_write(php_stream *stream, const char *buf, size_t count TSRMLS_DC)
{
    size_t ret = 0;
#else
static ssize_t php_zstd_comp_write(php_stream *stream, const char *buf, size_t count TSRMLS_DC)
{
    ssize_t ret = 0;
#endif
    size_t x, res;

    STREAM_DATA_FROM_STREAM();

    while(count > 0) {
        /* enough room for full data */
        if (self->input.size + count < self->sizein) {
            memcpy(self->bufin + self->input.size, buf, count);
            self->input.size += count;
            ret += count;
            count = 0;
            break;
        }

        /* fill input buffer */
        x = self->sizein - self->input.size;
        memcpy(self->bufin + self->input.size, buf, x);
        self->input.size += x;
        buf += x;
        count -= x;
        ret += x;

        /* compress and write */
        self->input.pos = 0;
        do {
            self->output.size = self->sizeout;
            self->output.pos  = 0;
#if ZSTD_VERSION_NUMBER >= 10400
            res = ZSTD_compressStream2(self->cctx, &self->output, &self->input, ZSTD_e_continue);
#else
            res = ZSTD_compressStream(self->cctx, &self->output, &self->input);
#endif
            if (ZSTD_isError(res)) {
                php_error_docref(NULL TSRMLS_CC, E_WARNING, "libzstd error %s\n", ZSTD_getErrorName(res));
#if PHP_VERSION_ID >= 70400
                return -1;
#endif
            }
            php_stream_write(self->stream, self->bufout, self->output.pos);
        } while (self->input.pos != self->input.size);

        self->input.pos = 0;
        self->input.size = 0;
    }
    return ret;
}


static php_stream_ops php_stream_zstd_read_ops = {
    NULL,    /* write */
    php_zstd_decomp_read,
    php_zstd_decomp_close,
    NULL,    /* flush */
    STREAM_NAME,
    NULL,    /* seek */
    NULL,    /* cast */
    NULL,    /* stat */
    NULL     /* set_option */
};


static php_stream_ops php_stream_zstd_write_ops = {
    php_zstd_comp_write,
    NULL,    /* read */
    php_zstd_comp_close,
    php_zstd_comp_flush,
    STREAM_NAME,
    NULL,    /* seek */
    NULL,    /* cast */
    NULL,    /* stat */
    NULL     /* set_option */
};


static php_stream *
php_stream_zstd_opener(
    php_stream_wrapper *wrapper,
#if PHP_VERSION_ID < 50600
    char *path,
    char *mode,
#else
    const char *path,
    const char *mode,
#endif
    int options,
#if PHP_MAJOR_VERSION < 7
    char **opened_path,
#else
    zend_string **opened_path,
#endif
    php_stream_context *context
    STREAMS_DC TSRMLS_DC)
{
    php_zstd_stream_data *self;
    int level = ZSTD_CLEVEL_DEFAULT;
    int compress;
#if ZSTD_VERSION_NUMBER >= 10400
    ZSTD_CDict *cdict = NULL;
    ZSTD_DDict *ddict = NULL;
#endif

    if (strncasecmp(STREAM_NAME, path, sizeof(STREAM_NAME)-1) == 0) {
        path += sizeof(STREAM_NAME)-1;
        if (strncmp("://", path, 3) == 0) {
            path += 3;
        }
    }

    if (php_check_open_basedir(path TSRMLS_CC)) {
        return NULL;
    }

    if (!strcmp(mode, "w") || !strcmp(mode, "wb")) {
       compress = 1;
    } else if (!strcmp(mode, "r") || !strcmp(mode, "rb")) {
       compress = 0;
    } else {
        php_error_docref(NULL TSRMLS_CC, E_ERROR, "zstd: invalid open mode");
        return NULL;
    }

    if (context) {
#if PHP_MAJOR_VERSION >= 7
        zval *tmpzval;
        zend_string *data;

        if (NULL != (tmpzval = php_stream_context_get_option(context, "zstd", "level"))) {
            level = zval_get_long(tmpzval);
        }
#if ZSTD_VERSION_NUMBER >= 10400
        if (NULL != (tmpzval = php_stream_context_get_option(context, "zstd", "dict"))) {
            data = zval_get_string(tmpzval);
            if (compress) {
                cdict = ZSTD_createCDict(ZSTR_VAL(data), ZSTR_LEN(data), level);
            } else {
                ddict = ZSTD_createDDict(ZSTR_VAL(data), ZSTR_LEN(data));
            }
            zend_string_release(data);
        }
#endif
#else
        zval **tmpzval;

        if (php_stream_context_get_option(context, "zstd", "level", &tmpzval) == SUCCESS) {
            convert_to_long_ex(tmpzval);
            level = Z_LVAL_PP(tmpzval);
        }
#if ZSTD_VERSION_NUMBER >= 10400
        if (php_stream_context_get_option(context, "zstd", "dict", &tmpzval) == SUCCESS) {
            convert_to_string(*tmpzval);
            if (compress) {
                cdict = ZSTD_createCDict(Z_STRVAL_PP(tmpzval), Z_STRLEN_PP(tmpzval), level);
            } else {
                ddict = ZSTD_createDDict(Z_STRVAL_PP(tmpzval), Z_STRLEN_PP(tmpzval));
            }
        }
#endif
#endif
    }

    if (level > ZSTD_maxCLevel()) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "zstd: compression level (%d) must be less than %d", level, ZSTD_maxCLevel());
        level = ZSTD_maxCLevel();
    }

    self = ecalloc(sizeof(*self), 1);
    self->stream = php_stream_open_wrapper(path, mode, options | REPORT_ERRORS, NULL);
    if (!self->stream) {
        efree(self);
        return NULL;
    }

    /* File */
    if (compress) {
        self->dctx = NULL;
        self->cctx = ZSTD_createCCtx();
        if (!self->cctx) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "zstd: compression context failed");
            php_stream_close(self->stream);
            efree(self);
            return NULL;
        }
#if ZSTD_VERSION_NUMBER >= 10400
        ZSTD_CCtx_reset(self->cctx, ZSTD_reset_session_only);
        ZSTD_CCtx_refCDict(self->cctx, cdict);
        ZSTD_CCtx_setParameter(self->cctx, ZSTD_c_compressionLevel, level);
#else
        ZSTD_initCStream(self->cctx, level);
#endif
        self->bufin = emalloc(self->sizein = ZSTD_CStreamInSize());
        self->bufout = emalloc(self->sizeout = ZSTD_CStreamOutSize());
        self->input.src  = self->bufin;
        self->input.pos   = 0;
        self->input.size  = 0;
        self->output.dst = self->bufout;
        self->output.pos  = 0;
        self->output.size = 0;

        return php_stream_alloc(&php_stream_zstd_write_ops, self, NULL, mode);

    } else {
        self->dctx = ZSTD_createDCtx();
        if (!self->dctx) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "zstd: compression context failed");
            php_stream_close(self->stream);
            efree(self);
            return NULL;
        }
        self->cctx = NULL;
        self->bufin = emalloc(self->sizein = ZSTD_DStreamInSize());
        self->bufout = emalloc(self->sizeout = ZSTD_DStreamOutSize());
#if ZSTD_VERSION_NUMBER >= 10400
        ZSTD_DCtx_reset(self->dctx, ZSTD_reset_session_only);
        ZSTD_DCtx_refDDict(self->dctx, ddict);
#else
        ZSTD_initDStream(self->dctx);
#endif
        self->input.src   = self->bufin;
        self->input.pos   = 0;
        self->input.size  = 0;
        self->output.dst  = self->bufout;
        self->output.pos  = 0;
        self->output.size = 0;

        return php_stream_alloc(&php_stream_zstd_read_ops, self, NULL, mode);
    }
    return NULL;
}


static php_stream_wrapper_ops zstd_stream_wops = {
    php_stream_zstd_opener,
    NULL,    /* close */
    NULL,    /* fstat */
    NULL,    /* stat */
    NULL,    /* opendir */
    STREAM_NAME,
    NULL,    /* unlink */
    NULL,    /* rename */
    NULL,    /* mkdir */
    NULL    /* rmdir */
#if PHP_VERSION_ID >= 50400
    , NULL
#endif
};


php_stream_wrapper php_stream_zstd_wrapper = {
    &zstd_stream_wops,
    NULL,
    0 /* is_url */
};

#if PHP_MAJOR_VERSION >= 7 && defined(HAVE_APCU_SUPPORT)
static int APC_SERIALIZER_NAME(zstd)(APC_SERIALIZER_ARGS)
{
    int result;
    php_serialize_data_t var_hash;
    size_t size;
    smart_str var = {0};

    PHP_VAR_SERIALIZE_INIT(var_hash);
    php_var_serialize(&var, (zval*) value, &var_hash);
    PHP_VAR_SERIALIZE_DESTROY(var_hash);
    if (var.s == NULL) {
        return 0;
    }

    size = ZSTD_compressBound(ZSTR_LEN(var.s));
    *buf = (char*) emalloc(size + 1);
    if (*buf == NULL) {
        *buf_len = 0;
        return 0;
    }

    *buf_len = ZSTD_compress(*buf, size, ZSTR_VAL(var.s), ZSTR_LEN(var.s),
                             DEFAULT_COMPRESS_LEVEL);
    if (ZSTD_isError(*buf_len) || *buf_len == 0) {
        efree(*buf);
        *buf = NULL;
        *buf_len = 0;
        result = 0;
    } else {
        result = 1;
    }

    smart_str_free(&var);

    return result;
}

static int APC_UNSERIALIZER_NAME(zstd)(APC_UNSERIALIZER_ARGS)
{
    const unsigned char* tmp;
    int result;
    php_unserialize_data_t var_hash;
    size_t var_len;
    uint64_t size;
    void *var;

    size = ZSTD_getFrameContentSize(buf, buf_len);
    if (size == ZSTD_CONTENTSIZE_ERROR
        || size == ZSTD_CONTENTSIZE_UNKNOWN) {
        ZVAL_NULL(value);
        return 0;
    }

    var = emalloc(size);
    if (!var) {
        ZVAL_NULL(value);
        return 0;
    }

    var_len = ZSTD_decompress(var, size, buf, buf_len);
    if (ZSTD_isError(var_len) || var_len == 0) {
        efree(var);
        ZVAL_NULL(value);
        return 0;
    }

    PHP_VAR_UNSERIALIZE_INIT(var_hash);
    tmp = (unsigned char*) var;
    result = php_var_unserialize(value, &tmp, var + var_len, &var_hash);
    PHP_VAR_UNSERIALIZE_DESTROY(var_hash);

    if (!result) {
        php_error_docref(NULL, E_NOTICE,
                         "Error at offset %ld of %ld bytes",
                         (zend_long) (tmp - (unsigned char*) var),
                         (zend_long) var_len);
        ZVAL_NULL(value);
        result = 0;
    } else {
        result = 1;
    }

    efree(var);

    return result;
}
#endif

ZEND_MINIT_FUNCTION(zstd)
{
    REGISTER_LONG_CONSTANT("ZSTD_COMPRESS_LEVEL_MIN",
                           1,
                           CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("ZSTD_COMPRESS_LEVEL_MAX",
                           ZSTD_maxCLevel(),
                           CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("ZSTD_COMPRESS_LEVEL_DEFAULT",
                           DEFAULT_COMPRESS_LEVEL,
                           CONST_CS | CONST_PERSISTENT);

    REGISTER_LONG_CONSTANT("LIBZSTD_VERSION_NUMBER",
                           ZSTD_VERSION_NUMBER,
                           CONST_CS | CONST_PERSISTENT);
    REGISTER_STRING_CONSTANT("LIBZSTD_VERSION_STRING",
                           ZSTD_VERSION_STRING,
                           CONST_CS | CONST_PERSISTENT);

    php_register_url_stream_wrapper(STREAM_NAME, &php_stream_zstd_wrapper TSRMLS_CC);

#if PHP_MAJOR_VERSION >= 7 && defined(HAVE_APCU_SUPPORT)
    apc_register_serializer("zstd",
                            APC_SERIALIZER_NAME(zstd),
                            APC_UNSERIALIZER_NAME(zstd),
                            NULL);
#endif

    return SUCCESS;
}

ZEND_MINFO_FUNCTION(zstd)
{
    php_info_print_table_start();
    php_info_print_table_row(2, "Zstd support", "enabled");
    php_info_print_table_row(2, "Extension Version", PHP_ZSTD_EXT_VERSION);
    php_info_print_table_row(2, "Interface Version", ZSTD_VERSION_STRING);
#if PHP_MAJOR_VERSION >= 7 && defined(HAVE_APCU_SUPPORT)
    php_info_print_table_row(2, "APCu serializer ABI", APC_SERIALIZER_ABI);
#endif
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

#if PHP_MAJOR_VERSION >= 7 && defined(HAVE_APCU_SUPPORT)
static const zend_module_dep zstd_module_deps[] = {
    ZEND_MOD_OPTIONAL("apcu")
    ZEND_MOD_END
};
#endif

zend_module_entry zstd_module_entry = {
#if PHP_MAJOR_VERSION >= 7 && defined(HAVE_APCU_SUPPORT)
    STANDARD_MODULE_HEADER_EX,
    NULL,
    zstd_module_deps,
#elif ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    "zstd",
    zstd_functions,
    ZEND_MINIT(zstd),
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
