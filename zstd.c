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
#include <SAPI.h>
#include <php_ini.h>
#include <ext/standard/file.h>
#include <ext/standard/info.h>
#include <ext/standard/php_smart_string.h>
#if defined(HAVE_APCU_SUPPORT)
#include <ext/standard/php_var.h>
#include <ext/apcu/apc_serializer.h>
#include <zend_smart_str.h>
#endif
#include "php_zstd.h"

int le_state;

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

#define DEFAULT_COMPRESS_LEVEL 3

// zend_string_efree doesnt exist in PHP7.2, 20180731 is PHP 7.3
#if ZEND_MODULE_API_NO < 20180731
#define zend_string_efree(string) zend_string_free(string)
#endif

#define ZSTD_WARNING(...) \
    php_error_docref(NULL, E_WARNING, __VA_ARGS__)

#define ZSTD_IS_ERROR(result) \
    UNEXPECTED(ZSTD_isError(result))

/* One-shot functions */
ZEND_BEGIN_ARG_INFO_EX(arginfo_zstd_compress, 0, 0, 1)
    ZEND_ARG_INFO(0, data)
    ZEND_ARG_INFO(0, level)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_zstd_uncompress, 0, 0, 1)
    ZEND_ARG_INFO(0, data)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_zstd_compress_dict, 0, 0, 2)
    ZEND_ARG_INFO(0, data)
    ZEND_ARG_INFO(0, dictBuffer)
    ZEND_ARG_INFO(0, level)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_zstd_uncompress_dict, 0, 0, 2)
    ZEND_ARG_INFO(0, data)
    ZEND_ARG_INFO(0, dictBuffer)
ZEND_END_ARG_INFO()

/* Incremental functions */
ZEND_BEGIN_ARG_INFO_EX(arginfo_zstd_compress_init, 0, 0, 0)
    ZEND_ARG_INFO(0, level)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_zstd_compress_add, 0, 0, 2)
    ZEND_ARG_INFO(0, context)
    ZEND_ARG_INFO(0, data)
    ZEND_ARG_INFO(0, end)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_zstd_uncompress_init, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_zstd_uncompress_add, 0, 0, 2)
    ZEND_ARG_INFO(0, context)
    ZEND_ARG_INFO(0, data)
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 80000
#if PHP_VERSION_ID >= 80100
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_ob_zstd_handler, 0, 2, MAY_BE_STRING|MAY_BE_FALSE)
    ZEND_ARG_TYPE_INFO(0, data, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, flags, IS_LONG, 0)
ZEND_END_ARG_INFO()
#else
ZEND_BEGIN_ARG_INFO_EX(arginfo_ob_zstd_handler, 0, 0, 2)
    ZEND_ARG_INFO(0, data)
    ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()
#endif

ZEND_DECLARE_MODULE_GLOBALS(zstd);
#endif

static size_t zstd_check_compress_level(zend_long level)
{
    uint16_t maxLevel = (uint16_t) ZSTD_maxCLevel();

    if (level > maxLevel) {
        ZSTD_WARNING("compression level (" ZEND_LONG_FMT ")"
            " must be within 1..%d or smaller then 0", level, maxLevel);
      return 0;
    }

    return 1;
}

// Truncate string to given size
static zend_always_inline zend_string*
zstd_string_output_truncate(zend_string* output, size_t real_length)
{
    size_t capacity = ZSTR_LEN(output);
    size_t free_space = capacity - real_length;

    // Reallocate just when capacity and real size differs a lot
    // or the free space is bigger than 1 MB
    if (UNEXPECTED(free_space > (capacity / 8)
                   || free_space > (1024 * 1024))) {
        output = zend_string_truncate(output, real_length, 0);
    }
    ZSTR_LEN(output) = real_length;
    ZSTR_VAL(output)[real_length] = '\0';
    return output;
}

ZEND_FUNCTION(zstd_compress)
{
    zend_string *output;
    size_t size, result;
    zend_long level = DEFAULT_COMPRESS_LEVEL;

    char *input;
    size_t input_len;

#if PHP_VERSION_ID < 80000
    zval *data;
    if (zend_parse_parameters(ZEND_NUM_ARGS(),
                              "z|l", &data, &level) == FAILURE) {
      RETURN_FALSE;
    }
    if (Z_TYPE_P(data) != IS_STRING) {
      zend_error(E_WARNING, "zstd_compress(): expects parameter to be string.");
      RETURN_FALSE;
    }
    input = Z_STRVAL_P(data);
    input_len = Z_STRLEN_P(data);
#else
    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_STRING(input, input_len)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(level)
    ZEND_PARSE_PARAMETERS_END();
#endif

    if (!zstd_check_compress_level(level)) {
        RETURN_FALSE;
    }

    size = ZSTD_compressBound(input_len);
    output = zend_string_alloc(size, 0);

    result = ZSTD_compress(ZSTR_VAL(output), size, input, input_len,
                           (int)level);

    if (ZSTD_IS_ERROR(result)) {
        zend_string_efree(output);
        RETVAL_FALSE;
    }

    output = zstd_string_output_truncate(output, result);
    RETVAL_NEW_STR(output);
}

ZEND_FUNCTION(zstd_uncompress)
{
    uint64_t size;
    size_t result;
    zend_string *output;
    uint8_t streaming = 0;

    char *input;
    size_t input_len;

#if PHP_VERSION_ID < 80000
    zval *data;
    if (zend_parse_parameters(ZEND_NUM_ARGS(),
                              "z", &data) == FAILURE) {
      RETURN_FALSE;
    }
    if (Z_TYPE_P(data) != IS_STRING) {
      zend_error(E_WARNING,
                 "zstd_uncompress(): expects parameter to be string.");
      RETURN_FALSE;
    }
    input = Z_STRVAL_P(data);
    input_len = Z_STRLEN_P(data);
#else
    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STRING(input, input_len)
    ZEND_PARSE_PARAMETERS_END();
#endif

    size = ZSTD_getFrameContentSize(input, input_len);
    if (size == ZSTD_CONTENTSIZE_ERROR) {
        ZSTD_WARNING("it was not compressed by zstd");
        RETURN_FALSE;
    } else if (size == ZSTD_CONTENTSIZE_UNKNOWN) {
        streaming = 1;
        size = ZSTD_DStreamOutSize();
    }

    output = zend_string_alloc(size, 0);

    if (!streaming) {
        result = ZSTD_decompress(ZSTR_VAL(output), size,
                                 input, input_len);

        if (ZSTD_IS_ERROR(result)) {
            zend_string_efree(output);
            ZSTD_WARNING("can not decompress stream");
            RETURN_FALSE;
        }

    } else {
        ZSTD_DStream *stream;
        ZSTD_inBuffer in = { NULL, 0, 0 };
        ZSTD_outBuffer out = { NULL, 0, 0 };

        stream = ZSTD_createDStream();
        if (stream == NULL) {
            zend_string_efree(output);
            ZSTD_WARNING("can not create stream");
            RETURN_FALSE;
        }

        result = ZSTD_initDStream(stream);
        if (ZSTD_IS_ERROR(result)) {
            zend_string_efree(output);
            ZSTD_freeDStream(stream);
            ZSTD_WARNING("can not init stream");
            RETURN_FALSE;
        }

        in.src = input;
        in.size = input_len;
        in.pos = 0;

        out.dst = ZSTR_VAL(output);
        out.size = size;
        out.pos = 0;

        while (in.pos < in.size) {
            if (out.pos == out.size) {
                out.size += size;
                output = zend_string_extend(output, out.size, 0);
                out.dst = ZSTR_VAL(output);
            }

            result = ZSTD_decompressStream(stream, &out, &in);
            if (ZSTD_IS_ERROR(result)) {
                zend_string_efree(output);
                ZSTD_freeDStream(stream);
                ZSTD_WARNING("can not decompress stream");
                RETURN_FALSE;
            }

            if (result == 0) {
                break;
            }
        }

        result = out.pos;

        ZSTD_freeDStream(stream);
    }

    output = zstd_string_output_truncate(output, result);
    RETVAL_NEW_STR(output);
}

ZEND_FUNCTION(zstd_compress_dict)
{
    zend_long level = DEFAULT_COMPRESS_LEVEL;

    zend_string *output;
    char *input, *dict;
    size_t input_len, dict_len;

    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_STRING(input, input_len)
        Z_PARAM_STRING(dict, dict_len)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(level)
    ZEND_PARSE_PARAMETERS_END();

    if (!zstd_check_compress_level(level)) {
        RETURN_FALSE;
    }

    ZSTD_CCtx* const cctx = ZSTD_createCCtx();
    if (cctx == NULL) {
        ZSTD_WARNING("ZSTD_createCCtx() error");
        RETURN_FALSE;
    }
    ZSTD_CDict* const cdict = ZSTD_createCDict(dict,
                                               dict_len,
                                               (int)level);
    if (!cdict) {
        ZSTD_freeCStream(cctx);
        ZSTD_WARNING("ZSTD_createCDict() error");
        RETURN_FALSE;
    }

    size_t const cBuffSize = ZSTD_compressBound(input_len);
    output = zend_string_alloc(cBuffSize, 0);

    size_t const cSize = ZSTD_compress_usingCDict(cctx, ZSTR_VAL(output), cBuffSize,
                                                  input,
                                                  input_len,
                                                  cdict);
    if (ZSTD_IS_ERROR(cSize)) {
        ZSTD_freeCStream(cctx);
        ZSTD_freeCDict(cdict);
        zend_string_efree(output);
        ZSTD_WARNING("%s", ZSTD_getErrorName(cSize));
        RETURN_FALSE;
    }

    output = zstd_string_output_truncate(output, cSize);
    RETVAL_NEW_STR(output);

    ZSTD_freeCCtx(cctx);
    ZSTD_freeCDict(cdict);
}

ZEND_FUNCTION(zstd_uncompress_dict)
{
    char *input, *dict;
    size_t input_len, dict_len;
    zend_string *output;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STRING(input, input_len)
        Z_PARAM_STRING(dict, dict_len)
    ZEND_PARSE_PARAMETERS_END();

    unsigned long long const rSize = ZSTD_getFrameContentSize(input,
                                                              input_len);

    if (rSize == 0 || rSize == ZSTD_CONTENTSIZE_ERROR) {
        ZSTD_WARNING("it was not compressed by zstd");
        RETURN_FALSE;
    }

    ZSTD_DCtx* const dctx = ZSTD_createDCtx();
    if (dctx == NULL) {
        ZSTD_WARNING("ZSTD_createDCtx() error");
        RETURN_FALSE;
    }
    ZSTD_DDict* const ddict = ZSTD_createDDict(dict,
                                               dict_len);
    if (!ddict) {
        ZSTD_freeDStream(dctx);
        ZSTD_WARNING("ZSTD_createDDict() error");
        RETURN_FALSE;
    }

    output = zend_string_alloc(rSize, 0);

    size_t const dSize = ZSTD_decompress_usingDDict(dctx, ZSTR_VAL(output), rSize,
                                                    input,
                                                    input_len,
                                                    ddict);
    if (dSize != rSize) {
        ZSTD_freeDStream(dctx);
        ZSTD_freeDDict(ddict);
        zend_string_efree(output);
        ZSTD_WARNING("%s", ZSTD_getErrorName(dSize));
        RETURN_FALSE;
    }
    ZSTD_freeDCtx(dctx);
    ZSTD_freeDDict(ddict);

    output = zstd_string_output_truncate(output, dSize);
    RETVAL_NEW_STR(output);
}

struct _php_zstd_context {
    ZSTD_CCtx* cctx;
    ZSTD_DCtx* dctx;
    ZSTD_CDict *cdict;
    ZSTD_inBuffer input;
    ZSTD_outBuffer output;
};

static php_zstd_context* php_zstd_output_handler_context_init(void)
{
    php_zstd_context *ctx
        = (php_zstd_context *) ecalloc(1, sizeof(php_zstd_context));
    ctx->cctx = NULL;
    ctx->dctx = NULL;
    return ctx;
}

static void php_zstd_output_handler_context_free(php_zstd_context *ctx)
{
    if (ctx->cctx) {
        ZSTD_freeCCtx(ctx->cctx);
        ctx->cctx = NULL;
    }
    if (ctx->dctx) {
        ZSTD_freeDCtx(ctx->dctx);
        ctx->dctx = NULL;
    }
    if (ctx->cdict) {
        ZSTD_freeCDict(ctx->cdict);
        ctx->cdict = NULL;
    }
    if (ctx->output.dst) {
        efree(ctx->output.dst);
        ctx->output.dst = NULL;
    }
}

static void php_zstd_state_rsrc_dtor(zend_resource *res)
{
    php_zstd_context *ctx = zend_fetch_resource(res, NULL, le_state);
    php_zstd_output_handler_context_free(ctx);
}

ZEND_FUNCTION(zstd_compress_init)
{
    zend_long level = DEFAULT_COMPRESS_LEVEL;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(level)
    ZEND_PARSE_PARAMETERS_END();

    if (!zstd_check_compress_level(level)) {
        RETURN_FALSE;
    }

    php_zstd_context *ctx = php_zstd_output_handler_context_init();

    ctx->cctx = ZSTD_createCCtx();
    if (ctx->cctx == NULL) {
        efree(ctx);
        ZSTD_WARNING("ZSTD_createCCtx() error");
        RETURN_FALSE;
    }
    ctx->cdict = NULL;

    ZSTD_CCtx_reset(ctx->cctx, ZSTD_reset_session_only);
    ZSTD_CCtx_setParameter(ctx->cctx, ZSTD_c_compressionLevel, level);

    ctx->output.size = ZSTD_CStreamOutSize();
    ctx->output.dst  = emalloc(ctx->output.size);
    ctx->output.pos  = 0;

    RETURN_RES(zend_register_resource(ctx, le_state));
}

ZEND_FUNCTION(zstd_compress_add)
{
    zval *resource;
    php_zstd_context *ctx;
    char *in_buf;
    size_t in_size;
    zend_bool end = 0;
    smart_string out = {0};

    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_RESOURCE(resource)
        Z_PARAM_STRING(in_buf, in_size)
        Z_PARAM_OPTIONAL
        Z_PARAM_BOOL(end)
    ZEND_PARSE_PARAMETERS_END();

    ctx = zend_fetch_resource(Z_RES_P(resource), NULL, le_state);
    if (ctx == NULL) {
        php_error_docref(NULL, E_WARNING,
                         "ZStandard incremental compress resource failed");
        RETURN_FALSE;
    }

    ZSTD_inBuffer in = { in_buf, in_size, 0 };
    size_t res;

    do {
        ctx->output.pos = 0;
        res = ZSTD_compressStream2(ctx->cctx, &ctx->output,
                                   &in, end ? ZSTD_e_end : ZSTD_e_flush);
        if (ZSTD_isError(res)) {
            php_error_docref(NULL, E_WARNING,
                             "libzstd error %s\n", ZSTD_getErrorName(res));
            smart_string_free(&out);
            RETURN_FALSE;
        }
        smart_string_appendl(&out, ctx->output.dst, ctx->output.pos);
    } while (res > 0);

    RETVAL_STRINGL(out.c, out.len);
    smart_string_free(&out);
}

ZEND_FUNCTION(zstd_uncompress_init)
{
    php_zstd_context *ctx = php_zstd_output_handler_context_init();
    size_t result;

    ctx->dctx = ZSTD_createDCtx();
    if (ctx->dctx == NULL) {
        efree(ctx);
        ZSTD_WARNING("ZSTD_createCCtx() error");
        RETURN_FALSE;
    }
    ctx->cdict = NULL;

    ZSTD_DCtx_reset(ctx->dctx, ZSTD_reset_session_only);

    ctx->output.size = ZSTD_CStreamOutSize();
    ctx->output.dst  = emalloc(ctx->output.size);
    ctx->output.pos  = 0;

    RETURN_RES(zend_register_resource(ctx, le_state));
}

ZEND_FUNCTION(zstd_uncompress_add)
{
    zval *resource;
    php_zstd_context *ctx;
    char *in_buf;
    size_t in_size;
    zend_bool end = 0;
    smart_string out = {0};

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_RESOURCE(resource)
        Z_PARAM_STRING(in_buf, in_size)
    ZEND_PARSE_PARAMETERS_END();

    ctx = zend_fetch_resource(Z_RES_P(resource), NULL, le_state);
    if (ctx == NULL) {
        php_error_docref(NULL, E_WARNING,
                         "ZStandard incremental uncompress resource failed");
        RETURN_FALSE;
    }

    ZSTD_inBuffer in = { in_buf, in_size, 0 };
    size_t res = 1;
    uint64_t size;

    ctx->output.pos = 0;
    while (in.pos < in.size && res > 0) {
        if (ctx->output.pos == ctx->output.size) {
            ctx->output.size += size;
            ctx->output.dst = erealloc(ctx->output.dst, ctx->output.size);
        }

        res = ZSTD_decompressStream(ctx->dctx, &ctx->output, &in);
        if (ZSTD_isError(res)) {
            php_error_docref(NULL, E_WARNING,
                             "libzstd error %s\n", ZSTD_getErrorName(res));
            smart_string_free(&out);
            RETURN_FALSE;
        }

        smart_string_appendl(&out, ctx->output.dst, ctx->output.pos);
    }

    RETVAL_STRINGL(out.c, out.len);
    smart_string_free(&out);
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

static int php_zstd_decomp_close(php_stream *stream, int close_handle)
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

static int php_zstd_comp_flush_or_end(php_zstd_stream_data *self, int end)
{
    size_t res;
    int ret = 0;

    ZSTD_inBuffer in = { NULL, 0, 0 };

    /* Flush / End */
    do {
        self->output.pos  = 0;
        res = ZSTD_compressStream2(self->cctx, &self->output, &in,
                                   end ? ZSTD_e_end : ZSTD_e_flush);
        if (ZSTD_isError(res)) {
            php_error_docref(NULL, E_WARNING,
                             "libzstd error %s\n", ZSTD_getErrorName(res));
            ret = EOF;
        }
        php_stream_write(self->stream, self->output.dst, self->output.pos);
    } while (res > 0);

    return ret;
}


static int php_zstd_comp_flush(php_stream *stream)
{
    STREAM_DATA_FROM_STREAM();

    return php_zstd_comp_flush_or_end(self, 0);
}


static int php_zstd_comp_close(php_stream *stream, int close_handle)
{
    STREAM_DATA_FROM_STREAM();

    if (!self) {
        return EOF;
    }

    php_zstd_comp_flush_or_end(self, 1);

    if (close_handle) {
        if (self->stream) {
            php_stream_close(self->stream);
            self->stream = NULL;
        }
    }

    ZSTD_freeCCtx(self->cctx);
    efree(self->output.dst);
    efree(self);
    stream->abstract = NULL;

    return EOF;
}


#if PHP_VERSION_ID < 70400
static size_t php_zstd_decomp_read(php_stream *stream, char *buf, size_t count)
{
    size_t ret = 0;
#else
static ssize_t php_zstd_decomp_read(php_stream *stream, char *buf, size_t count)
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
            res = ZSTD_decompressStream(self->dctx,
                                        &self->output, &self->input);
            if (ZSTD_IS_ERROR(res)) {
                php_error_docref(NULL, E_WARNING,
                                 "libzstd error %s\n", ZSTD_getErrorName(res));
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
            self->input.size = php_stream_read(self->stream,
                                               self->bufin, self->sizein);
            if (!self->input.size) {
                /* EOF */
                count = 0;
            }
        }
    }
    return ret;
}


#if PHP_VERSION_ID < 70400
static size_t
#else
static ssize_t
#endif
php_zstd_comp_write(php_stream *stream, const char *buf, size_t count)
{
    STREAM_DATA_FROM_STREAM();

    size_t res;
    ZSTD_inBuffer in = { buf, count, 0 };

    do {
        self->output.pos = 0;
        res = ZSTD_compressStream2(self->cctx, &self->output,
                                   &in, ZSTD_e_continue);
        if (ZSTD_isError(res)) {
            php_error_docref(NULL, E_WARNING,
                             "libzstd error %s\n", ZSTD_getErrorName(res));
#if PHP_VERSION_ID >= 70400
            return -1;
#endif
        }
        php_stream_write(self->stream, self->output.dst, self->output.pos);

    } while (res > 0);

    return count;
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
    const char *path,
    const char *mode,
    int options,
    zend_string **opened_path,
    php_stream_context *context
    STREAMS_DC)
{
    php_zstd_stream_data *self;
    int level = ZSTD_CLEVEL_DEFAULT;
    int compress;
    ZSTD_CDict *cdict = NULL;
    ZSTD_DDict *ddict = NULL;

    if (strncasecmp(STREAM_NAME, path, sizeof(STREAM_NAME)-1) == 0) {
        path += sizeof(STREAM_NAME)-1;
        if (strncmp("://", path, 3) == 0) {
            path += 3;
        }
    }

    if (php_check_open_basedir(path)) {
        return NULL;
    }

    if (!strcmp(mode, "w") || !strcmp(mode, "wb")
        || !strcmp(mode, "a") || !strcmp(mode, "ab")) {
       compress = 1;
    } else if (!strcmp(mode, "r") || !strcmp(mode, "rb")) {
       compress = 0;
    } else {
        php_error_docref(NULL, E_ERROR, "zstd: invalid open mode");
        return NULL;
    }

    if (context) {
        zval *tmpzval;
        zend_string *data;

        tmpzval = php_stream_context_get_option(context, "zstd", "level");
        if (NULL != tmpzval) {
            level = zval_get_long(tmpzval);
        }
        tmpzval = php_stream_context_get_option(context, "zstd", "dict");
        if (NULL != tmpzval) {
            data = zval_get_string(tmpzval);
            if (compress) {
                cdict = ZSTD_createCDict(ZSTR_VAL(data), ZSTR_LEN(data), level);
            } else {
                ddict = ZSTD_createDDict(ZSTR_VAL(data), ZSTR_LEN(data));
            }
            zend_string_release(data);
        }
    }

    if (level > ZSTD_maxCLevel()) {
        php_error_docref(NULL, E_WARNING,
                         "zstd: compression level (%d) must be less than %d",
                         level, ZSTD_maxCLevel());
        level = ZSTD_maxCLevel();
    }

    self = ecalloc(sizeof(*self), 1);
    self->stream = php_stream_open_wrapper(path, mode,
                                           options | REPORT_ERRORS, NULL);
    if (!self->stream) {
        efree(self);
        return NULL;
    }

    /* File */
    if (compress) {
        self->dctx = NULL;
        self->cctx = ZSTD_createCCtx();
        if (!self->cctx) {
            php_error_docref(NULL, E_WARNING,
                             "zstd: compression context failed");
            php_stream_close(self->stream);
            efree(self);
            return NULL;
        }
        ZSTD_CCtx_reset(self->cctx, ZSTD_reset_session_only);
        ZSTD_CCtx_refCDict(self->cctx, cdict);
        ZSTD_CCtx_setParameter(self->cctx, ZSTD_c_compressionLevel, level);

        self->output.size = ZSTD_CStreamOutSize();
        self->output.dst  = emalloc(self->output.size);
        self->output.pos  = 0;

        return php_stream_alloc(&php_stream_zstd_write_ops, self, NULL, mode);

    } else {
        self->dctx = ZSTD_createDCtx();
        if (!self->dctx) {
            php_error_docref(NULL, E_WARNING,
                             "zstd: compression context failed");
            php_stream_close(self->stream);
            efree(self);
            return NULL;
        }
        self->cctx = NULL;
        self->bufin = emalloc(self->sizein = ZSTD_DStreamInSize());
        self->bufout = emalloc(self->sizeout = ZSTD_DStreamOutSize());
        ZSTD_DCtx_reset(self->dctx, ZSTD_reset_session_only);
        ZSTD_DCtx_refDDict(self->dctx, ddict);
        self->input.src   = self->bufin;
        self->input.pos   = 0;
        self->input.size  = 0;
        self->output.dst  = self->bufout;
        self->output.pos  = 0;
        self->output.size = 0;

        return php_stream_alloc(&php_stream_zstd_read_ops, self, NULL, mode);
    }
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
    NULL,    /* rmdir */
    NULL
};


php_stream_wrapper php_stream_zstd_wrapper = {
    &zstd_stream_wops,
    NULL,
    0 /* is_url */
};

#if defined(HAVE_APCU_SUPPORT)
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
    *buf = emalloc(size + 1);

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
    unsigned char* var;

    size = ZSTD_getFrameContentSize(buf, buf_len);
    if (size == ZSTD_CONTENTSIZE_ERROR
        || size == ZSTD_CONTENTSIZE_UNKNOWN) {
        ZVAL_NULL(value);
        return 0;
    }

    var = (unsigned char*) emalloc(size);

    var_len = ZSTD_decompress(var, size, buf, buf_len);
    if (ZSTD_isError(var_len) || var_len == 0) {
        efree(var);
        ZVAL_NULL(value);
        return 0;
    }

    PHP_VAR_UNSERIALIZE_INIT(var_hash);
    tmp = var;
    result = php_var_unserialize(value, &tmp, var + var_len, &var_hash);
    PHP_VAR_UNSERIALIZE_DESTROY(var_hash);

    if (!result) {
        php_error_docref(NULL, E_NOTICE,
                         "Error at offset %ld of %ld bytes",
                         (long) (tmp - (unsigned char*) var),
                         (long) var_len);
        ZVAL_NULL(value);
        result = 0;
    } else {
        result = 1;
    }

    efree(var);

    return result;
}
#endif

/* output handler */
#if PHP_VERSION_ID >= 80000
#define PHP_ZSTD_OUTPUT_HANDLER_NAME "zstd output compression"

static int php_zstd_output_encoding(void)
{
    zval *enc;

    if (!PHP_ZSTD_G(compression_coding)) {
        if ((Z_TYPE(PG(http_globals)[TRACK_VARS_SERVER]) == IS_ARRAY
#if PHP_VERSION_ID >= 80100
             || zend_is_auto_global(ZSTR_KNOWN(ZEND_STR_AUTOGLOBAL_SERVER)))
#else
             || zend_is_auto_global_str(ZEND_STRL("_SERVER")))
#endif
            && (enc = zend_hash_str_find(
                    Z_ARRVAL(PG(http_globals)[TRACK_VARS_SERVER]),
                    "HTTP_ACCEPT_ENCODING",
                    sizeof("HTTP_ACCEPT_ENCODING") - 1))) {
            convert_to_string(enc);
            if (strstr(Z_STRVAL_P(enc), "zstd")) {
                PHP_ZSTD_G(compression_coding) = 1;
            }
        }
    }
    return PHP_ZSTD_G(compression_coding);
}

static void
php_zstd_output_handler_load_dict(php_zstd_context *ctx, int level)
{
    php_stream *stream = NULL;
    zval *zcontext = NULL;
    php_stream_context *context = NULL;
    zend_string *contents = NULL;
    zend_long maxlen = (ssize_t) PHP_STREAM_COPY_ALL;
    char *dict = PHP_ZSTD_G(output_compression_dict);

    if (!dict || strlen(dict) <= 0) {
        return;
    }

    context = php_stream_context_from_zval(zcontext, 0);
    stream = php_stream_open_wrapper_ex(dict, "rb",
                                        REPORT_ERRORS, // | USE_PATH
                                        NULL, context);
    if (!stream) {
        ZSTD_WARNING("could not open dictionary stream: %s", dict);
        return;
    }

    if (php_stream_is(stream, PHP_STREAM_IS_STDIO)) {
        php_stream_set_option(stream, PHP_STREAM_OPTION_READ_BUFFER,
                              PHP_STREAM_BUFFER_NONE, NULL);
    }

    contents = php_stream_copy_to_mem(stream, maxlen, 0);

    if (contents) {
        ctx->cdict = ZSTD_createCDict(ZSTR_VAL(contents), ZSTR_LEN(contents),
                                      level);
        if (!ctx->cdict) {
            ZSTD_WARNING("failed to create compression dictionary: %s", dict);
        }

        zend_string(contents);
    } else {
        ZSTD_WARNING("failed to get dictionary stream: %s", dict);
    }

    php_stream_close(stream);
}

static zend_result php_zstd_output_handler_context_start(php_zstd_context *ctx)
{
    int level = PHP_ZSTD_G(output_compression_level);

    if (!zstd_check_compress_level(level) || level < 0) {
        level = ZSTD_CLEVEL_DEFAULT;
    }

    ctx->cctx = ZSTD_createCCtx();
    if (!ctx->cctx) {
        return FAILURE;
    }

    php_zstd_output_handler_load_dict(ctx, level);

    ZSTD_CCtx_reset(ctx->cctx, ZSTD_reset_session_only);
    ZSTD_CCtx_refCDict(ctx->cctx, ctx->cdict);
    ZSTD_CCtx_setParameter(ctx->cctx, ZSTD_c_compressionLevel, level);

    ctx->output.size = ZSTD_CStreamOutSize();
    ctx->output.dst = emalloc(ctx->output.size);
    ctx->output.pos = 0;

    return SUCCESS;
}

static void php_zstd_output_handler_context_dtor(void *opaq)
{
    php_zstd_context *ctx = (php_zstd_context *) opaq;

    if (ctx) {
        php_zstd_output_handler_context_free(ctx);
        efree(ctx);
    }
}

static void
php_zstd_output_handler_write(php_zstd_context *ctx,
                              php_output_context *output_context, int flags)
{
    size_t res;

    output_context->out.size = ZSTD_compressBound(output_context->in.used);
    if (output_context->out.size < ctx->output.size) {
        output_context->out.size = ctx->output.size;
    }
    output_context->out.data = emalloc(output_context->out.size);
    output_context->out.free = 1;
    output_context->out.used = 0;

    do {
        ctx->output.pos = 0;
        res = ZSTD_compressStream2(ctx->cctx, &ctx->output, &ctx->input, flags);
        if (ZSTD_isError(res)) {
            ZSTD_WARNING("zstd output handler compress error %s\n",
                         ZSTD_getErrorName(res));
        }
        memcpy(output_context->out.data + output_context->out.used,
               ctx->output.dst, ctx->output.pos);
        output_context->out.used += ctx->output.pos;
    } while (res > 0);
}

static zend_result
php_zstd_output_handler_ex(php_zstd_context *ctx,
                           php_output_context *output_context)
{
    if (output_context->op & PHP_OUTPUT_HANDLER_START) {
        /* start up */
        if (php_zstd_output_handler_context_start(ctx) != SUCCESS) {
            return FAILURE;
        }
    }

    if (output_context->op & PHP_OUTPUT_HANDLER_CLEAN) {
        /* clean */
        if (output_context->in.used) {
            ctx->input.src = output_context->in.data;
            ctx->input.size = output_context->in.used;
        } else {
            ctx->input.src = NULL;
            ctx->input.size = 0;
        }
        ctx->input.pos = 0;

        php_zstd_output_handler_write(ctx, output_context, ZSTD_e_end);

        if (output_context->op & PHP_OUTPUT_HANDLER_FINAL) {
            /* discard */
            php_zstd_output_handler_context_free(ctx);
            return SUCCESS;
        } else {
            /* restart */
            ZSTD_CCtx_reset(ctx->cctx, ZSTD_reset_session_only);
        }
    } else {
        int flags = ZSTD_e_continue;

        if (output_context->op & PHP_OUTPUT_HANDLER_FINAL) {
            flags = ZSTD_e_end;
        } else if (output_context->op & PHP_OUTPUT_HANDLER_FLUSH) {
            flags = ZSTD_e_flush;
        }

        ctx->input.src = output_context->in.data;
        ctx->input.size = output_context->in.used;
        ctx->input.pos = 0;

        php_zstd_output_handler_write(ctx, output_context, flags);

        if (output_context->op & PHP_OUTPUT_HANDLER_FINAL) {
            php_zstd_output_handler_context_free(ctx);
        }
    }

    return SUCCESS;
}

static zend_result
php_zstd_output_handler(void **handler_context,
                        php_output_context *output_context)
{
    php_zstd_context *ctx = *(php_zstd_context **) handler_context;

    if (!php_zstd_output_encoding()) {
        if ((output_context->op & PHP_OUTPUT_HANDLER_START)
            &&  (output_context->op != (PHP_OUTPUT_HANDLER_START
                                        |PHP_OUTPUT_HANDLER_CLEAN
                                        |PHP_OUTPUT_HANDLER_FINAL))) {
            sapi_add_header_ex(ZEND_STRL("Vary: Accept-Encoding"), 1, 0);
        }
        return FAILURE;
    }

    if (php_zstd_output_handler_ex(ctx, output_context) != SUCCESS) {
        return FAILURE;
    }

    if (!(output_context->op & PHP_OUTPUT_HANDLER_CLEAN)
        || ((output_context->op & PHP_OUTPUT_HANDLER_START)
            && !(output_context->op & PHP_OUTPUT_HANDLER_FINAL))) {
        int flags;
        if (php_output_handler_hook(PHP_OUTPUT_HANDLER_HOOK_GET_FLAGS,
                                    &flags) == SUCCESS) {
            if (!(flags & PHP_OUTPUT_HANDLER_STARTED)) {
                if (SG(headers_sent) || !PHP_ZSTD_G(output_compression)) {
                    return FAILURE;
                }
                sapi_add_header_ex(ZEND_STRL("Content-Encoding: zstd"), 1, 1);
                sapi_add_header_ex(ZEND_STRL("Vary: Accept-Encoding"), 1, 0);
                php_output_handler_hook(PHP_OUTPUT_HANDLER_HOOK_IMMUTABLE,
                                        NULL);
            }
        }
    }

    return SUCCESS;
}

static php_output_handler*
php_zstd_output_handler_init(const char *handler_name, size_t handler_name_len,
                             size_t chunk_size, int flags)
{
    php_output_handler *h = NULL;

    if (!PHP_ZSTD_G(output_compression)) {
        PHP_ZSTD_G(output_compression) = 1;
    }

    PHP_ZSTD_G(handler_registered) = 1;

    if ((h = php_output_handler_create_internal(handler_name, handler_name_len,
                                                php_zstd_output_handler,
                                                chunk_size, flags))) {
        php_output_handler_set_context(h,
                                       php_zstd_output_handler_context_init(),
                                       php_zstd_output_handler_context_dtor);
    }

    return h;
}

static void php_zstd_cleanup_ob_handler_mess(void)
{
    if (PHP_ZSTD_G(ob_handler)) {
        php_zstd_output_handler_context_dtor(PHP_ZSTD_G(ob_handler));
        PHP_ZSTD_G(ob_handler) = NULL;
    }
}

ZEND_FUNCTION(ob_zstd_handler)
{
    char *in_str;
    size_t in_len;
    zend_long flags = 0;
    php_output_context ctx = {0};
    int encoding;
    zend_result rv;

    if (zend_parse_parameters(ZEND_NUM_ARGS(),
                              "sl", &in_str, &in_len, &flags) != SUCCESS) {
        RETURN_THROWS();
    }

    if (!(encoding = php_zstd_output_encoding())) {
        RETURN_FALSE;
    }

    if (flags & PHP_OUTPUT_HANDLER_START) {
        sapi_add_header_ex(ZEND_STRL("Content-Encoding: zstd"), 1, 1);
        sapi_add_header_ex(ZEND_STRL("Vary: Accept-Encoding"), 1, 0);
    }

    if (!PHP_ZSTD_G(ob_handler)) {
        PHP_ZSTD_G(ob_handler) = php_zstd_output_handler_context_init();
    }

    ctx.op = flags;
    ctx.in.data = in_str;
    ctx.in.used = in_len;

    rv = php_zstd_output_handler_ex(PHP_ZSTD_G(ob_handler), &ctx);

    if (rv != SUCCESS) {
        if (ctx.out.data && ctx.out.free) {
            efree(ctx.out.data);
        }
        php_zstd_cleanup_ob_handler_mess();
        RETURN_FALSE;
    }

    if (ctx.out.data) {
        RETVAL_STRINGL(ctx.out.data, ctx.out.used);
        if (ctx.out.free) {
            efree(ctx.out.data);
        }
    } else {
        RETVAL_EMPTY_STRING();
    }
}

static zend_result
php_zstd_output_conflict_check(const char *handler_name,
                               size_t handler_name_len)
{
    if (php_output_get_level() > 0) {
        if (php_output_handler_conflict(handler_name, handler_name_len,
                                        ZEND_STRL(PHP_ZSTD_OUTPUT_HANDLER_NAME))
            ||  php_output_handler_conflict(handler_name, handler_name_len,
                                            ZEND_STRL("ob_zstd_handler"))
            ||  php_output_handler_conflict(handler_name, handler_name_len,
                                            ZEND_STRL("ob_gzhandler"))
            ||  php_output_handler_conflict(handler_name, handler_name_len,
                                            ZEND_STRL("mb_output_handler"))
            ||  php_output_handler_conflict(handler_name, handler_name_len,
                                            ZEND_STRL("URL-Rewriter"))) {
            return FAILURE;
        }
    }
    return SUCCESS;
}

static void php_zstd_output_compression_start(void)
{
    php_output_handler *h;

    switch (PHP_ZSTD_G(output_compression)) {
        case 0:
            break;
        case 1:
            /* break omitted intentionally */
        default:
            if (php_zstd_output_encoding() &&
                (h = php_zstd_output_handler_init(
                    ZEND_STRL(PHP_ZSTD_OUTPUT_HANDLER_NAME),
                    PHP_OUTPUT_HANDLER_DEFAULT_SIZE,
                    PHP_OUTPUT_HANDLER_STDFLAGS))) {
                php_output_handler_start(h);
            }
            break;
    }
}

static PHP_INI_MH(OnUpdate_zstd_output_compression)
{
    int int_value;
    zend_long *p;

    if (new_value == NULL) {
        return FAILURE;
    }

    if (zend_string_equals_literal_ci(new_value, "off")) {
        int_value = 0;
    } else if (zend_string_equals_literal_ci(new_value, "on")) {
        int_value = 1;
#if PHP_VERSION_ID >= 80200
    } else if (zend_ini_parse_quantity_warn(new_value, entry->name)) {
#else
    } else if (zend_atoi(ZSTR_VAL(new_value), ZSTR_LEN(new_value))) {
#endif
        int_value = 1;
    } else {
        int_value = 0;
    }

    if (stage == PHP_INI_STAGE_RUNTIME) {
        int status = php_output_get_status();
        if (status & PHP_OUTPUT_SENT) {
            php_error_docref("ref.outcontrol", E_WARNING,
                             "Cannot change zstd.output_compression"
                             " - headers already sent");
            return FAILURE;
        }
    }

    p = (zend_long *) ZEND_INI_GET_ADDR();
    *p = int_value;

    PHP_ZSTD_G(output_compression) = PHP_ZSTD_G(output_compression_default);

    if (stage == PHP_INI_STAGE_RUNTIME && int_value) {
        if (!php_output_handler_started(
                ZEND_STRL(PHP_ZSTD_OUTPUT_HANDLER_NAME))) {
            php_zstd_output_compression_start();
        }
    }

    return SUCCESS;
}

PHP_INI_BEGIN()
  STD_PHP_INI_BOOLEAN("zstd.output_compression", "0",
                      PHP_INI_ALL, OnUpdate_zstd_output_compression,
                      output_compression_default,
                      zend_zstd_globals, zstd_globals)
  STD_PHP_INI_ENTRY("zstd.output_compression_level", "-1",
                    PHP_INI_ALL, OnUpdateLong, output_compression_level,
                    zend_zstd_globals, zstd_globals)
  STD_PHP_INI_ENTRY("zstd.output_compression_dict", "",
                    PHP_INI_ALL, OnUpdateString, output_compression_dict,
                    zend_zstd_globals, zstd_globals)
PHP_INI_END()
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

    le_state = zend_register_list_destructors_ex(php_zstd_state_rsrc_dtor,
                                                 NULL, "zstd.state",
                                                 module_number);

    php_register_url_stream_wrapper(STREAM_NAME, &php_stream_zstd_wrapper);

#if defined(HAVE_APCU_SUPPORT)
    apc_register_serializer("zstd",
                            APC_SERIALIZER_NAME(zstd),
                            APC_UNSERIALIZER_NAME(zstd),
                            NULL);
#endif

#if PHP_VERSION_ID >= 80000
    php_output_handler_alias_register(ZEND_STRL("ob_zstd_handler"),
                                      php_zstd_output_handler_init);
    php_output_handler_conflict_register(ZEND_STRL("ob_zstd_handler"),
                                         php_zstd_output_conflict_check);
    php_output_handler_conflict_register(
        ZEND_STRL(PHP_ZSTD_OUTPUT_HANDLER_NAME),
        php_zstd_output_conflict_check);

    REGISTER_INI_ENTRIES();
#endif

    return SUCCESS;
}

ZEND_MSHUTDOWN_FUNCTION(zstd)
{
#if PHP_VERSION_ID >= 80000
    UNREGISTER_INI_ENTRIES();
#endif
    return SUCCESS;
}

ZEND_RINIT_FUNCTION(zstd)
{
#if PHP_VERSION_ID >= 80000
    PHP_ZSTD_G(compression_coding) = 0;

    if (!PHP_ZSTD_G(handler_registered)) {
        PHP_ZSTD_G(output_compression) = PHP_ZSTD_G(output_compression_default);

        php_zstd_output_compression_start();
    }
#endif
    return SUCCESS;
}

ZEND_RSHUTDOWN_FUNCTION(zstd)
{
#if PHP_VERSION_ID >= 80000
    php_zstd_cleanup_ob_handler_mess();

    PHP_ZSTD_G(handler_registered) = 0;
#endif
    return SUCCESS;
}

ZEND_MINFO_FUNCTION(zstd)
{
    php_info_print_table_start();
    php_info_print_table_row(2, "Zstd support", "enabled");
    php_info_print_table_row(2, "Extension Version", PHP_ZSTD_VERSION);
    php_info_print_table_row(2, "Interface Version", ZSTD_VERSION_STRING);
#if defined(HAVE_APCU_SUPPORT)
    php_info_print_table_row(2, "APCu serializer ABI", APC_SERIALIZER_ABI);
#endif
    php_info_print_table_end();

#if PHP_VERSION_ID >= 80000
    DISPLAY_INI_ENTRIES();
#endif
}

#if PHP_VERSION_ID >= 80000
ZEND_GINIT_FUNCTION(zstd)
{
#if defined(COMPILE_DL_ZSTD) && defined(ZTS)
    ZEND_TSRMLS_CACHE_UPDATE();
#endif
    zstd_globals->ob_handler = NULL;
    zstd_globals->handler_registered = 0;
}
#endif

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

    ZEND_FE(zstd_compress_init, arginfo_zstd_compress_init)
    ZEND_FE(zstd_compress_add, arginfo_zstd_compress_add)
    ZEND_FE(zstd_uncompress_init, arginfo_zstd_uncompress_init)
    ZEND_FE(zstd_uncompress_add, arginfo_zstd_uncompress_add)

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

    ZEND_NS_FALIAS(PHP_ZSTD_NS, compress_init,
                   zstd_compress_init, arginfo_zstd_compress_init)
    ZEND_NS_FALIAS(PHP_ZSTD_NS, compress_add,
                   zstd_compress_add, arginfo_zstd_compress_add)
    ZEND_NS_FALIAS(PHP_ZSTD_NS, uncompress_init,
                   zstd_uncompress_init, arginfo_zstd_uncompress_init)
    ZEND_NS_FALIAS(PHP_ZSTD_NS, uncompress_add,
                   zstd_uncompress_add, arginfo_zstd_uncompress_add)

#if PHP_VERSION_ID >= 80000
    ZEND_FE(ob_zstd_handler, arginfo_ob_zstd_handler)
#endif

    {NULL, NULL, NULL}
};

#if defined(HAVE_APCU_SUPPORT)
static const zend_module_dep zstd_module_deps[] = {
    ZEND_MOD_OPTIONAL("apcu")
    ZEND_MOD_END
};
#endif

zend_module_entry zstd_module_entry = {
#if defined(HAVE_APCU_SUPPORT)
    STANDARD_MODULE_HEADER_EX,
    NULL,
    zstd_module_deps,
#else
    STANDARD_MODULE_HEADER,
#endif
    "zstd",
    zstd_functions,
    ZEND_MINIT(zstd),
    ZEND_MSHUTDOWN(zstd),
    ZEND_RINIT(zstd),
    ZEND_RSHUTDOWN(zstd),
    ZEND_MINFO(zstd),
    PHP_ZSTD_VERSION,
#if PHP_VERSION_ID >= 80000
    PHP_MODULE_GLOBALS(zstd),
    PHP_GINIT(zstd),
    NULL,
    NULL,
    STANDARD_MODULE_PROPERTIES_EX
#else
    STANDARD_MODULE_PROPERTIES
#endif
};

#ifdef COMPILE_DL_ZSTD
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(zstd)
#endif
