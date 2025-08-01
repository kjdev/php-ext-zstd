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
#if PHP_VERSION_ID < 70200
#include <ext/standard/php_smart_string.h>
#else
#include "Zend/zend_smart_string.h"
#endif
#if defined(HAVE_APCU_SUPPORT)
#include <ext/standard/php_var.h>
#include <ext/apcu/apc_serializer.h>
#include <zend_smart_str.h>
#endif
#include <Zend/zend_API.h>
#include <Zend/zend_interfaces.h>
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

// zend_string_efree doesnt exist in PHP7.2, 20180731 is PHP 7.3
#if ZEND_MODULE_API_NO < 20180731
#define zend_string_efree(string) zend_string_free(string)
#endif

#define ZSTD_WARNING(...) \
    php_error_docref(NULL, E_WARNING, __VA_ARGS__)

#define ZSTD_IS_ERROR(result) \
    UNEXPECTED(ZSTD_isError(result))

zend_class_entry *zstd_context_ptr;
static const zend_function_entry zstd_context_methods[] = {
    ZEND_FE_END
};

struct _php_zstd_context {
    ZSTD_CCtx* cctx;
    ZSTD_DCtx* dctx;
    ZSTD_CDict *cdict;
    ZSTD_DDict *ddict;
    ZSTD_inBuffer input;
    ZSTD_outBuffer output;
    zend_object std;
};

/* Zstd Compress/UnCompress Context */
static php_zstd_context *php_zstd_context_from_obj(zend_object *obj)
{
    return (php_zstd_context *)
        ((char *)(obj) - XtOffsetOf(php_zstd_context, std));
}

#define PHP_ZSTD_CONTEXT_OBJ_INIT_OF_CLASS(ce) \
  object_init_ex(return_value, ce); \
  php_zstd_context *ctx = php_zstd_context_from_obj(Z_OBJ_P(return_value)); \
  php_zstd_context_init(ctx);

static void php_zstd_context_init(php_zstd_context *ctx)
{
    ctx->cctx = NULL;
    ctx->dctx = NULL;
    ctx->cdict = NULL;
    ctx->ddict = NULL;
    ctx->input.src = NULL;
    ctx->input.size = 0;
    ctx->input.pos = 0;
    ctx->output.dst = NULL;
    ctx->output.size = 0;
    ctx->output.pos = 0;
}

static void php_zstd_context_free(php_zstd_context *ctx)
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
    if (ctx->ddict) {
        ZSTD_freeDDict(ctx->ddict);
        ctx->ddict = NULL;
    }
    if (ctx->output.dst) {
        efree(ctx->output.dst);
        ctx->output.dst = NULL;
    }
}

static void php_zstd_context_free_obj(zend_object *object)
{
    php_zstd_context *intern = php_zstd_context_from_obj(object);
    php_zstd_context_free(intern);
    zend_object_std_dtor(&intern->std);
}

static zend_object *
php_zstd_context_create_object(zend_class_entry *class_type,
                               zend_object_handlers *handlers)
{
    php_zstd_context *intern;
#if PHP_VERSION_ID >= 80000
    intern = zend_object_alloc(sizeof(php_zstd_context), class_type);
#else
    intern = ecalloc(1,
                     sizeof(php_zstd_context)
                     + zend_object_properties_size(class_type));
#endif
    zend_object_std_init(&intern->std, class_type);
    object_properties_init(&intern->std, class_type);
    intern->std.handlers = handlers;

    return &intern->std;
}

static int
php_zstd_context_create_compress(php_zstd_context *ctx,
                                 zend_long level, zend_string *dict)
{
    ctx->cctx = ZSTD_createCCtx();
    if (ctx->cctx == NULL) {
        ZSTD_WARNING("failed to create compress context");
        return FAILURE;
    }

    ZSTD_CCtx_reset(ctx->cctx, ZSTD_reset_session_only);
    ZSTD_CCtx_setParameter(ctx->cctx, ZSTD_c_compressionLevel, level);

    if (dict) {
        ctx->cdict = ZSTD_createCDict(ZSTR_VAL(dict), ZSTR_LEN(dict),
                                      (int)level);
        if (!ctx->cdict) {
            ZSTD_WARNING("failed to load dictionary");
            return FAILURE;
        }

        ZSTD_CCtx_refCDict(ctx->cctx, ctx->cdict);
    }

    ctx->output.size = ZSTD_CStreamOutSize();
    ctx->output.dst = emalloc(ctx->output.size);
    ctx->output.pos = 0;

    return SUCCESS;
}

static int
php_zstd_context_create_decompress(php_zstd_context *ctx, zend_string *dict)
{
    ctx->dctx = ZSTD_createDCtx();
    if (ctx->dctx == NULL) {
        ZSTD_WARNING("failed to prepare uncompression");
        return FAILURE;
    }

    ZSTD_DCtx_reset(ctx->dctx, ZSTD_reset_session_only);

    if (dict) {
        ctx->ddict = ZSTD_createDDict(ZSTR_VAL(dict), ZSTR_LEN(dict));
        if (!ctx->ddict) {
            ZSTD_WARNING("failed to load dictionary");
            return FAILURE;
        }

        ZSTD_DCtx_refDDict(ctx->dctx, ctx->ddict);
    }

    ctx->output.size = ZSTD_DStreamOutSize();
    ctx->output.dst = emalloc(ctx->output.size);
    ctx->output.pos = 0;

    return SUCCESS;
}

/* Zstd Compress Context */
zend_class_entry *php_zstd_compress_context_ce;
static zend_object_handlers php_zstd_compress_context_object_handlers;

static zend_object *
php_zstd_compress_context_create_object(zend_class_entry *class_type)
{
    return php_zstd_context_create_object(
        class_type,
        &php_zstd_compress_context_object_handlers);
}

static zend_function *
php_zstd_compress_context_get_constructor(zend_object *object)
{
    zend_throw_error(NULL,
                     "Cannot directly construct Zstd\\Compress\\Context, "
                     "use zstd_compress_init() instead");
    return NULL;
}

static zend_class_entry *php_zstd_compress_context_register_class(void)
{
    zend_class_entry ce, *class_entry;

    INIT_NS_CLASS_ENTRY(ce, "Zstd\\Compress", "Context", NULL);
#if PHP_VERSION_ID >= 80000
    class_entry = zend_register_internal_class_ex(&ce, NULL);
#if PHP_VERSION_ID >= 80100
    class_entry->ce_flags |= ZEND_ACC_FINAL|ZEND_ACC_NO_DYNAMIC_PROPERTIES|ZEND_ACC_NOT_SERIALIZABLE;
#else
    class_entry->ce_flags |= ZEND_ACC_FINAL|ZEND_ACC_NO_DYNAMIC_PROPERTIES;
#endif
#else
    ce.create_object = php_zstd_compress_context_create_object;
    class_entry = zend_register_internal_class(&ce);
    class_entry->ce_flags |= ZEND_ACC_FINAL;
#endif

    return class_entry;
}

/* Zstd UnCompress Context */
zend_class_entry *php_zstd_uncompress_context_ce;
static zend_object_handlers php_zstd_uncompress_context_object_handlers;

static zend_object *
php_zstd_uncompress_context_create_object(zend_class_entry *class_type)
{
    return php_zstd_context_create_object(
        class_type,
        &php_zstd_uncompress_context_object_handlers);
}

static zend_function *
php_zstd_uncompress_context_get_constructor(zend_object *object)
{
    zend_throw_error(NULL,
                     "Cannot directly construct Zstd\\UnCompress\\Context, "
                     "use zstd_uncompress_init() instead");
    return NULL;
}

static zend_class_entry *php_zstd_uncompress_context_register_class(void)
{
    zend_class_entry ce, *class_entry;

    INIT_NS_CLASS_ENTRY(ce, "Zstd\\UnCompress", "Context", NULL);
#if PHP_VERSION_ID >= 80000
    class_entry = zend_register_internal_class_ex(&ce, NULL);
#if PHP_VERSION_ID >= 80100
    class_entry->ce_flags |= ZEND_ACC_FINAL|ZEND_ACC_NO_DYNAMIC_PROPERTIES|ZEND_ACC_NOT_SERIALIZABLE;
#else
    class_entry->ce_flags |= ZEND_ACC_FINAL|ZEND_ACC_NO_DYNAMIC_PROPERTIES;
#endif
#else
    ce.create_object = php_zstd_uncompress_context_create_object;
    class_entry = zend_register_internal_class(&ce);
    class_entry->ce_flags |= ZEND_ACC_FINAL;
#endif

    return class_entry;
}

static php_zstd_context* php_zstd_output_handler_context_init(void)
{
    php_zstd_context *ctx
        = (php_zstd_context *) ecalloc(1, sizeof(php_zstd_context));
    ctx->cctx = NULL;
    ctx->dctx = NULL;
    return ctx;
}

#define php_zstd_output_handler_context_free(ctx) php_zstd_context_free(ctx)

/* One-shot functions */
#if PHP_VERSION_ID >= 80000
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_zstd_compress, 0, 1, MAY_BE_STRING|MAY_BE_FALSE)
    ZEND_ARG_TYPE_INFO(0, data, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, level, IS_LONG, 0, "ZSTD_COMPRESS_LEVEL_DEFAULT")
#else
ZEND_BEGIN_ARG_INFO_EX(arginfo_zstd_compress, 0, 0, 1)
    ZEND_ARG_INFO(0, data)
    ZEND_ARG_INFO(0, level)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 80000
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_zstd_uncompress, 0, 1, MAY_BE_STRING|MAY_BE_FALSE)
    ZEND_ARG_TYPE_INFO(0, data, IS_STRING, 0)
#else
ZEND_BEGIN_ARG_INFO_EX(arginfo_zstd_uncompress, 0, 0, 1)
    ZEND_ARG_INFO(0, data)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 80000
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_zstd_compress_dict, 0, 2, MAY_BE_STRING|MAY_BE_FALSE)
    ZEND_ARG_TYPE_INFO(0, data, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, dict, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, level, IS_LONG, 0, "ZSTD_COMPRESS_LEVEL_DEFAULT")
#else
ZEND_BEGIN_ARG_INFO_EX(arginfo_zstd_compress_dict, 0, 0, 2)
    ZEND_ARG_INFO(0, data)
    ZEND_ARG_INFO(0, dict)
    ZEND_ARG_INFO(0, level)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 80000
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_zstd_uncompress_dict, 0, 2, MAY_BE_STRING|MAY_BE_FALSE)
    ZEND_ARG_TYPE_INFO(0, data, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, dict, IS_STRING, 0)
#else
ZEND_BEGIN_ARG_INFO_EX(arginfo_zstd_uncompress_dict, 0, 0, 2)
    ZEND_ARG_INFO(0, data)
    ZEND_ARG_INFO(0, dict)
#endif
ZEND_END_ARG_INFO()

/* Incremental functions */
#if PHP_VERSION_ID >= 80000
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_TYPE_MASK_EX(arginfo_zstd_compress_init, 0, 0, Zstd\\Compress\\Context, MAY_BE_FALSE)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, level, IS_LONG, 0, "ZSTD_COMPRESS_LEVEL_DEFAULT")
#else
ZEND_BEGIN_ARG_INFO_EX(arginfo_zstd_compress_init, 0, 0, 0)
    ZEND_ARG_INFO(0, level)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 80000
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_zstd_compress_add, 0, 2, MAY_BE_STRING|MAY_BE_FALSE)
    ZEND_ARG_OBJ_INFO(0, context, Zstd\\Compress\\Context, 0)
    ZEND_ARG_TYPE_INFO(0, data, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, end, _IS_BOOL, 0, "false")
#else
ZEND_BEGIN_ARG_INFO_EX(arginfo_zstd_compress_add, 0, 0, 2)
    ZEND_ARG_INFO(0, context)
    ZEND_ARG_INFO(0, data)
    ZEND_ARG_INFO(0, end)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 80000
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_TYPE_MASK_EX(arginfo_zstd_uncompress_init, 0, 0, Zstd\\UnCompress\\Context, MAY_BE_FALSE)
#else
ZEND_BEGIN_ARG_INFO_EX(arginfo_zstd_uncompress_init, 0, 0, 0)
#endif
ZEND_END_ARG_INFO()

#if PHP_VERSION_ID >= 80000
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_zstd_uncompress_add, 0, 2, MAY_BE_STRING|MAY_BE_FALSE)
    ZEND_ARG_OBJ_INFO(0, context, Zstd\\UnCompress\\Context, 0)
    ZEND_ARG_TYPE_INFO(0, data, IS_STRING, 0)
#else
ZEND_BEGIN_ARG_INFO_EX(arginfo_zstd_uncompress_add, 0, 0, 2)
    ZEND_ARG_INFO(0, context)
    ZEND_ARG_INFO(0, data)
#endif
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

ZEND_FUNCTION(zstd_compress)
{
    size_t result;
    smart_string out = { 0 };
    zend_long level = ZSTD_CLEVEL_DEFAULT;
    zend_string *input;
    php_zstd_context ctx;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_STR(input)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(level)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    if (!zstd_check_compress_level(level)) {
        RETURN_FALSE;
    }

    php_zstd_context_init(&ctx);
    if (php_zstd_context_create_compress(&ctx, level, NULL) != SUCCESS) {
        php_zstd_context_free(&ctx);
        RETURN_FALSE;
    }

    ctx.input.src = ZSTR_VAL(input);
    ctx.input.size = ZSTR_LEN(input);
    ctx.input.pos = 0;

    do {
        ctx.output.pos = 0;
        result = ZSTD_compressStream2(ctx.cctx, &ctx.output,
                                      &ctx.input, ZSTD_e_end);
        if (ZSTD_isError(result)) {
            ZSTD_WARNING("%s", ZSTD_getErrorName(result));
            smart_string_free(&out);
            php_zstd_context_free(&ctx);
            RETURN_FALSE;
        }
        smart_string_appendl(&out, ctx.output.dst, ctx.output.pos);
    } while (result > 0);

    RETVAL_STRINGL(out.c, out.len);
    smart_string_free(&out);

    php_zstd_context_free(&ctx);
}

ZEND_FUNCTION(zstd_uncompress)
{
    size_t chunk, result;
    uint64_t size;
    smart_string out = { 0 };
    zend_string *input;
    php_zstd_context ctx;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(input)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    size = ZSTD_getFrameContentSize(ZSTR_VAL(input), ZSTR_LEN(input));
    if (size == ZSTD_CONTENTSIZE_ERROR) {
        ZSTD_WARNING("it was not compressed by zstd");
        RETURN_FALSE;
    } else if (size == ZSTD_CONTENTSIZE_UNKNOWN) {
        size = ZSTD_DStreamOutSize();
    }

    php_zstd_context_init(&ctx);
    if (php_zstd_context_create_decompress(&ctx, NULL) != SUCCESS) {
        php_zstd_context_free(&ctx);
        RETURN_FALSE;
    }

    chunk = ZSTD_DStreamOutSize();

    ctx.input.src = ZSTR_VAL(input);
    ctx.input.size = ZSTR_LEN(input);
    ctx.input.pos = 0;

    ctx.output.dst = emalloc(size);
    ctx.output.size = size;
    ctx.output.pos = 0;

    while (ctx.input.pos < ctx.input.size) {
        if (ctx.output.pos == ctx.output.size) {
            ctx.output.size += chunk;
            ctx.output.dst = erealloc(ctx.output.dst, ctx.output.size);
        }

        ctx.output.pos = 0;
        result = ZSTD_decompressStream(ctx.dctx, &ctx.output, &ctx.input);
        if (ZSTD_IS_ERROR(result)) {
            smart_string_free(&out);
            php_zstd_context_free(&ctx);
            ZSTD_WARNING("%s", ZSTD_getErrorName(result));
            RETURN_FALSE;
        }

        smart_string_appendl(&out, ctx.output.dst, ctx.output.pos);

        if (result == 0) {
            break;
        }
    }

    RETVAL_STRINGL(out.c, out.len);
    smart_string_free(&out);

    php_zstd_context_free(&ctx);
}

ZEND_FUNCTION(zstd_compress_dict)
{
    size_t result;
    smart_string out = { 0 };
    zend_long level = ZSTD_CLEVEL_DEFAULT;
    zend_string *input, *dict;
    php_zstd_context ctx;

    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_STR(input)
        Z_PARAM_STR(dict)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(level)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    if (!zstd_check_compress_level(level)) {
        RETURN_FALSE;
    }

    php_zstd_context_init(&ctx);
    if (php_zstd_context_create_compress(&ctx, level, dict) != SUCCESS) {
        php_zstd_context_free(&ctx);
        RETURN_FALSE;
    }

    ctx.input.src = ZSTR_VAL(input);
    ctx.input.size = ZSTR_LEN(input);
    ctx.input.pos = 0;

    do {
        ctx.output.pos = 0;
        result = ZSTD_compressStream2(ctx.cctx, &ctx.output,
                                      &ctx.input, ZSTD_e_end);
        if (ZSTD_isError(result)) {
            ZSTD_WARNING("%s", ZSTD_getErrorName(result));
            smart_string_free(&out);
            php_zstd_context_free(&ctx);
            RETURN_FALSE;
        }
        smart_string_appendl(&out, ctx.output.dst, ctx.output.pos);
    } while (result > 0);

    RETVAL_STRINGL(out.c, out.len);
    smart_string_free(&out);

    php_zstd_context_free(&ctx);
}

ZEND_FUNCTION(zstd_uncompress_dict)
{
    size_t chunk, result;
    unsigned long long size;
    smart_string out = { 0 };
    zend_string *input, *dict;
    php_zstd_context ctx;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STR(input)
        Z_PARAM_STR(dict)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    size = ZSTD_getFrameContentSize(ZSTR_VAL(input), ZSTR_LEN(input));
    if (size == 0) {
        RETURN_EMPTY_STRING();
    } else if (size == ZSTD_CONTENTSIZE_ERROR) {
        ZSTD_WARNING("it was not compressed by zstd");
        RETURN_FALSE;
    } else if (size == ZSTD_CONTENTSIZE_UNKNOWN) {
        size = ZSTD_DStreamOutSize();
    }

    php_zstd_context_init(&ctx);
    if (php_zstd_context_create_decompress(&ctx, dict) != SUCCESS) {
        php_zstd_context_free(&ctx);
        RETURN_FALSE;
    }

    chunk = ZSTD_DStreamOutSize();

    ctx.input.src = ZSTR_VAL(input);
    ctx.input.size = ZSTR_LEN(input);
    ctx.input.pos = 0;

    ctx.output.dst = emalloc(size);
    ctx.output.size = size;
    ctx.output.pos = 0;

    while (ctx.input.pos < ctx.input.size) {
        if (ctx.output.pos == ctx.output.size) {
            ctx.output.size += chunk;
            ctx.output.dst = erealloc(ctx.output.dst, ctx.output.size);
        }

        ctx.output.pos = 0;
        result = ZSTD_decompressStream(ctx.dctx, &ctx.output, &ctx.input);
        if (ZSTD_IS_ERROR(result)) {
            smart_string_free(&out);
            php_zstd_context_free(&ctx);
            ZSTD_WARNING("%s", ZSTD_getErrorName(result));
            RETURN_FALSE;
        }

        smart_string_appendl(&out, ctx.output.dst, ctx.output.pos);

        if (result == 0) {
            break;
        }
    }

    RETVAL_STRINGL(out.c, out.len);
    smart_string_free(&out);

    php_zstd_context_free(&ctx);
}

ZEND_FUNCTION(zstd_compress_init)
{
    zend_long level = ZSTD_CLEVEL_DEFAULT;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(level)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    if (!zstd_check_compress_level(level)) {
        RETURN_FALSE;
    }

    PHP_ZSTD_CONTEXT_OBJ_INIT_OF_CLASS(php_zstd_compress_context_ce);

    if (php_zstd_context_create_compress(ctx, level, NULL) != SUCCESS) {
        zval_ptr_dtor(return_value);
        RETURN_FALSE;
    }
}

ZEND_FUNCTION(zstd_compress_add)
{
    php_zstd_context *ctx;
    zend_string *input;
    zend_bool end = 0;
    smart_string out = {0};
#if PHP_VERSION_ID >= 80000
    zend_object *obj;
#else
    zval *obj;
#endif

    ZEND_PARSE_PARAMETERS_START(2, 3)
#if PHP_VERSION_ID >= 80000
        Z_PARAM_OBJ_OF_CLASS(obj, php_zstd_compress_context_ce)
#else
        Z_PARAM_OBJECT_OF_CLASS(obj, php_zstd_compress_context_ce)
#endif
        Z_PARAM_STR(input)
        Z_PARAM_OPTIONAL
        Z_PARAM_BOOL(end)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

#if PHP_VERSION_ID >= 80000
    ctx = php_zstd_context_from_obj(obj);
#else
    ctx = php_zstd_context_from_obj(Z_OBJ_P(obj));
#endif
    if (ctx == NULL || ctx->cctx == NULL) {
        ZSTD_WARNING("failed to prepare incremental compress");
        RETURN_FALSE;
    }

    ZSTD_inBuffer in = { ZSTR_VAL(input), ZSTR_LEN(input), 0 };
    size_t res;

    do {
        ctx->output.pos = 0;
        res = ZSTD_compressStream2(ctx->cctx, &ctx->output,
                                   &in, end ? ZSTD_e_end : ZSTD_e_flush);
        if (ZSTD_isError(res)) {
            ZSTD_WARNING("%s", ZSTD_getErrorName(res));
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
    PHP_ZSTD_CONTEXT_OBJ_INIT_OF_CLASS(php_zstd_uncompress_context_ce);

    if (php_zstd_context_create_decompress(ctx, NULL) != SUCCESS) {
        zval_ptr_dtor(return_value);
        RETURN_FALSE;
    }
}

ZEND_FUNCTION(zstd_uncompress_add)
{
    zend_object *context;
    php_zstd_context *ctx;
    zend_string *input;
    smart_string out = {0};
#if PHP_VERSION_ID >= 80000
    zend_object *obj;
#else
    zval *obj;
#endif

    ZEND_PARSE_PARAMETERS_START(2, 2)
#if PHP_VERSION_ID >= 80000
        Z_PARAM_OBJ_OF_CLASS(obj, php_zstd_uncompress_context_ce)
#else
        Z_PARAM_OBJECT_OF_CLASS(obj, php_zstd_uncompress_context_ce)
#endif
        Z_PARAM_STR(input)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

#if PHP_VERSION_ID >= 80000
    ctx = php_zstd_context_from_obj(obj);
#else
    ctx = php_zstd_context_from_obj(Z_OBJ_P(obj));
#endif
    if (ctx == NULL || ctx->dctx == NULL) {
        ZSTD_WARNING("failed to prepare incremental uncompress");
        RETURN_FALSE;
    }

    ZSTD_inBuffer in = { ZSTR_VAL(input), ZSTR_LEN(input), 0 };
    size_t res = 1;
    const size_t grow = ZSTD_DStreamOutSize();

    while (in.pos < in.size && res > 0) {
        if (ctx->output.pos == ctx->output.size) {
            ctx->output.size += grow;
            ctx->output.dst = erealloc(ctx->output.dst, ctx->output.size);
        }

        ctx->output.pos = 0;
        res = ZSTD_decompressStream(ctx->dctx, &ctx->output, &in);
        if (ZSTD_isError(res)) {
            ZSTD_WARNING("%s", ZSTD_getErrorName(res));
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
    php_zstd_context ctx;
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

    php_zstd_context_free(&self->ctx);

    efree(self->bufin);
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
        self->ctx.output.pos  = 0;
        res = ZSTD_compressStream2(self->ctx.cctx, &self->ctx.output, &in,
                                   end ? ZSTD_e_end : ZSTD_e_flush);
        if (ZSTD_isError(res)) {
            ZSTD_WARNING("zstd: %s", ZSTD_getErrorName(res));
            ret = EOF;
        }
        php_stream_write(self->stream,
                         self->ctx.output.dst, self->ctx.output.pos);
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

    php_zstd_context_free(&self->ctx);

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
        x = self->ctx.output.size - self->ctx.output.pos;
        /* enough available */
        if (x >= count) {
            memcpy(buf, self->bufout + self->ctx.output.pos, count);
            self->ctx.output.pos += count;
            ret += count;
            return ret;
        }
        /* take remaining from out  */
        if (x) {
            memcpy(buf, self->bufout + self->ctx.output.pos, x);
            self->ctx.output.pos += x;
            ret += x;
            buf += x;
            count -= x;
        }
        /* decompress */
        if (self->ctx.input.pos < self->ctx.input.size) {
            /* for zstd */
            self->ctx.output.pos = 0;
            self->ctx.output.size = self->sizeout;
            res = ZSTD_decompressStream(self->ctx.dctx,
                                        &self->ctx.output, &self->ctx.input);
            if (ZSTD_IS_ERROR(res)) {
                ZSTD_WARNING("zstd: %s", ZSTD_getErrorName(res));
#if PHP_VERSION_ID >= 70400
                return -1;
#endif
            }
            /* for us */
            self->ctx.output.size = self->ctx.output.pos;
            self->ctx.output.pos = 0;
        }  else {
            /* read */
            self->ctx.input.pos = 0;
            self->ctx.input.size = php_stream_read(self->stream,
                                                   self->bufin, self->sizein);
            if (!self->ctx.input.size) {
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
        self->ctx.output.pos = 0;
        res = ZSTD_compressStream2(self->ctx.cctx, &self->ctx.output,
                                   &in, ZSTD_e_continue);
        if (ZSTD_isError(res)) {
            ZSTD_WARNING("zstd: %s", ZSTD_getErrorName(res));
#if PHP_VERSION_ID >= 70400
            return -1;
#endif
        }
        php_stream_write(self->stream,
                         self->ctx.output.dst, self->ctx.output.pos);

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
    zend_string *dict = NULL;

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

        tmpzval = php_stream_context_get_option(context, "zstd", "level");
        if (NULL != tmpzval) {
            level = zval_get_long(tmpzval);
        }
        tmpzval = php_stream_context_get_option(context, "zstd", "dict");
        if (NULL != tmpzval) {
            dict = zval_get_string(tmpzval);
        }
    }

    if (level > ZSTD_maxCLevel()) {
        ZSTD_WARNING("zstd: compression level (%d) must be less than %d",
                     level, ZSTD_maxCLevel());
        level = ZSTD_maxCLevel();
    }

    self = ecalloc(sizeof(*self), 1);
    self->stream = php_stream_open_wrapper(path, mode,
                                           options | REPORT_ERRORS, NULL);
    if (!self->stream) {
        efree(self);
        if (dict) {
            zend_string_release(dict);
        }
        return NULL;
    }

    php_zstd_context_init(&self->ctx);

    /* File */
    if (compress) {
        if (php_zstd_context_create_compress(&self->ctx,
                                             level, dict) != SUCCESS) {
            ZSTD_WARNING("zstd: compression context failed");
            php_stream_close(self->stream);
            efree(self);
            if (dict) {
                zend_string_release(dict);
            }
            return NULL;
        }

        if (dict) {
            zend_string_release(dict);
        }

        return php_stream_alloc(&php_stream_zstd_write_ops, self, NULL, mode);

    } else {
        if (php_zstd_context_create_decompress(&self->ctx, dict) != SUCCESS) {
            ZSTD_WARNING("zstd: compression context failed");
            php_stream_close(self->stream);
            efree(self);
            if (dict) {
                zend_string_release(dict);
            }
            return NULL;
        }
        self->bufin = emalloc(self->sizein = ZSTD_DStreamInSize());
        self->bufout = self->ctx.output.dst;
        self->sizeout = self->ctx.output.size;
        self->ctx.input.src = self->bufin;
        self->ctx.input.pos = 0;
        self->ctx.input.size = 0;
        self->ctx.output.size = 0;

        if (dict) {
            zend_string_release(dict);
        }

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
                             ZSTD_CLEVEL_DEFAULT);
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

static zend_string*
php_zstd_output_handler_load_dict(php_zstd_context *ctx)
{
    php_stream *stream = NULL;
    zval *zcontext = NULL;
    php_stream_context *context = NULL;
    zend_long maxlen = (ssize_t) PHP_STREAM_COPY_ALL;
    char *dict = PHP_ZSTD_G(output_compression_dict);

    if (!dict || strlen(dict) <= 0) {
        return NULL;
    }

    context = php_stream_context_from_zval(zcontext, 0);
    stream = php_stream_open_wrapper_ex(dict, "rb",
                                        REPORT_ERRORS, // | USE_PATH
                                        NULL, context);
    if (!stream) {
        ZSTD_WARNING("could not open dictionary stream: %s", dict);
        return NULL;
    }

    if (php_stream_is(stream, PHP_STREAM_IS_STDIO)) {
        php_stream_set_option(stream, PHP_STREAM_OPTION_READ_BUFFER,
                              PHP_STREAM_BUFFER_NONE, NULL);
    }

    zend_string *data = php_stream_copy_to_mem(stream, maxlen, 0);

    php_stream_close(stream);

    return data;
}

static zend_result php_zstd_output_handler_context_start(php_zstd_context *ctx)
{
    int level = PHP_ZSTD_G(output_compression_level);

    if (!zstd_check_compress_level(level) || level < 0) {
        level = ZSTD_CLEVEL_DEFAULT;
    }

    zend_string *dict = php_zstd_output_handler_load_dict(ctx);

    if (php_zstd_context_create_compress(ctx, level, dict) != SUCCESS) {
        return FAILURE;
    }

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
                           ZSTD_CLEVEL_DEFAULT,
                           CONST_CS | CONST_PERSISTENT);

    REGISTER_LONG_CONSTANT("ZSTD_VERSION_NUMBER",
                           ZSTD_VERSION_NUMBER,
                           CONST_CS | CONST_PERSISTENT);
    REGISTER_STRING_CONSTANT("LIBZSTD_VERSION_STRING",
                           ZSTD_VERSION_STRING,
                           CONST_CS | CONST_PERSISTENT);

    php_register_url_stream_wrapper(STREAM_NAME, &php_stream_zstd_wrapper);

    php_zstd_compress_context_ce
        = php_zstd_compress_context_register_class();
#if PHP_VERSION_ID >= 80000
   php_zstd_compress_context_ce->create_object
       = php_zstd_compress_context_create_object;
#if PHP_VERSION_ID >= 80300
   php_zstd_compress_context_ce->default_object_handlers
       = &php_zstd_compress_context_object_handlers;
#endif
#if PHP_VERSION_ID < 80100
   php_zstd_compress_context_ce->serialize = zend_class_serialize_deny;
   php_zstd_compress_context_ce->unserialize = zend_class_unserialize_deny;
#endif
#endif
    memcpy(&php_zstd_compress_context_object_handlers,
           &std_object_handlers, sizeof(zend_object_handlers));
    php_zstd_compress_context_object_handlers.offset
        = XtOffsetOf(php_zstd_context, std);
    php_zstd_compress_context_object_handlers.free_obj
        = php_zstd_context_free_obj;
    php_zstd_compress_context_object_handlers.get_constructor
        = php_zstd_compress_context_get_constructor;
    php_zstd_compress_context_object_handlers.clone_obj = NULL;
#if PHP_VERSION_ID >= 80000
    php_zstd_compress_context_object_handlers.compare
        = zend_objects_not_comparable;
#endif

    php_zstd_uncompress_context_ce
        = php_zstd_uncompress_context_register_class();
#if PHP_VERSION_ID >= 80000
    php_zstd_uncompress_context_ce->create_object
        = php_zstd_uncompress_context_create_object;
#if PHP_VERSION_ID >= 80300
    php_zstd_uncompress_context_ce->default_object_handlers
        = &php_zstd_uncompress_context_object_handlers;
#endif
#if PHP_VERSION_ID < 80100
    php_zstd_uncompress_context_ce->serialize = zend_class_serialize_deny;
    php_zstd_uncompress_context_ce->unserialize = zend_class_unserialize_deny;
#endif
#endif
    memcpy(&php_zstd_uncompress_context_object_handlers,
           &std_object_handlers, sizeof(zend_object_handlers));
    php_zstd_uncompress_context_object_handlers.offset
        = XtOffsetOf(php_zstd_context, std);
    php_zstd_uncompress_context_object_handlers.free_obj
        = php_zstd_context_free_obj;
    php_zstd_uncompress_context_object_handlers.get_constructor
        = php_zstd_uncompress_context_get_constructor;
    php_zstd_uncompress_context_object_handlers.clone_obj = NULL;
#if PHP_VERSION_ID >= 80000
    php_zstd_uncompress_context_object_handlers.compare
        = zend_objects_not_comparable;
#endif

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
