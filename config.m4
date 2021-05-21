dnl config.m4 for extension zstd

dnl Check PHP version:
AC_MSG_CHECKING(PHP version)
if test ! -z "$phpincludedir"; then
    PHP_VERSION=`grep 'PHP_VERSION ' $phpincludedir/main/php_version.h | sed -e 's/.*"\([[0-9\.]]*\)".*/\1/g' 2>/dev/null`
elif test ! -z "$PHP_CONFIG"; then
    PHP_VERSION=`$PHP_CONFIG --version 2>/dev/null`
fi

if test x"$PHP_VERSION" = "x"; then
    AC_MSG_WARN([none])
else
    PHP_MAJOR_VERSION=`echo $PHP_VERSION | sed -e 's/\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\).*/\1/g' 2>/dev/null`
    PHP_MINOR_VERSION=`echo $PHP_VERSION | sed -e 's/\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\).*/\2/g' 2>/dev/null`
    PHP_RELEASE_VERSION=`echo $PHP_VERSION | sed -e 's/\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\).*/\3/g' 2>/dev/null`
    AC_MSG_RESULT([$PHP_VERSION])
fi

if test $PHP_MAJOR_VERSION -lt 5; then
    AC_MSG_ERROR([need at least PHP 5 or newer])
fi

PHP_ARG_ENABLE(zstd, whether to enable zstd support,
[  --enable-zstd           Enable zstd support])

PHP_ARG_WITH(libzstd, whether to use system zstd library,
[  --with-libzstd           Use system zstd library], no, no)

if test "$PHP_ZSTD" != "no"; then

  if test "$PHP_LIBZSTD" != "no"; then
    AC_PATH_PROG(PKG_CONFIG, pkg-config, no)

    AC_MSG_CHECKING(for libzstd)
    if test -x "$PKG_CONFIG" && $PKG_CONFIG --exists libzstd; then
      if $PKG_CONFIG libzstd --atleast-version 1; then
        LIBZSTD_CFLAGS=`$PKG_CONFIG libzstd --cflags`
        LIBZSTD_LIBDIR=`$PKG_CONFIG libzstd --libs`
        LIBZSTD_VERSON=`$PKG_CONFIG libzstd --modversion`
        AC_MSG_RESULT(from pkgconfig: version $LIBZSTD_VERSON)
      else
        AC_MSG_ERROR(system libzstd is too old)
      fi
    else
      AC_MSG_ERROR(pkg-config not found)
    fi
    PHP_EVAL_LIBLINE($LIBZSTD_LIBDIR, ZSTD_SHARED_LIBADD)
    PHP_EVAL_INCLINE($LIBZSTD_CFLAGS)
  else
    ZSTD_COMMON_SOURCES="
      zstd/lib/common/debug.c
      zstd/lib/common/entropy_common.c
      zstd/lib/common/error_private.c
      zstd/lib/common/fse_decompress.c
      zstd/lib/common/pool.c
      zstd/lib/common/threading.c
      zstd/lib/common/xxhash.c
      zstd/lib/common/zstd_common.c
    "
    ZSTD_COMPRESS_SOURCES="
      zstd/lib/compress/fse_compress.c
      zstd/lib/compress/hist.c
      zstd/lib/compress/huf_compress.c
      zstd/lib/compress/zstd_compress.c
      zstd/lib/compress/zstd_compress_literals.c
      zstd/lib/compress/zstd_compress_sequences.c
      zstd/lib/compress/zstd_compress_superblock.c
      zstd/lib/compress/zstd_double_fast.c
      zstd/lib/compress/zstd_fast.c
      zstd/lib/compress/zstd_lazy.c
      zstd/lib/compress/zstd_ldm.c
      zstd/lib/compress/zstd_opt.c
      zstd/lib/compress/zstdmt_compress.c
    "
    ZSTD_DECOMPRESS_SOURCES="
      zstd/lib/decompress/huf_decompress.c
      zstd/lib/decompress/zstd_ddict.c
      zstd/lib/decompress/zstd_decompress.c
      zstd/lib/decompress/zstd_decompress_block.c
    "

    PHP_ADD_INCLUDE(PHP_EXT_SRCDIR()/zstd/lib/common)
    PHP_ADD_INCLUDE(PHP_EXT_SRCDIR()/zstd/lib)
  fi
  PHP_NEW_EXTENSION(zstd, zstd.c $ZSTD_COMMON_SOURCES $ZSTD_COMPRESS_SOURCES $ZSTD_DECOMPRESS_SOURCES, $ext_shared)
  PHP_SUBST(ZSTD_SHARED_LIBADD)

  if test "$PHP_LIBZSTD" = "no"; then
    PHP_ADD_BUILD_DIR($ext_builddir/zstd/lib/common)
    PHP_ADD_BUILD_DIR($ext_builddir/zstd/lib/compress)
    PHP_ADD_BUILD_DIR($ext_builddir/zstd/lib/decompress)
  fi

  ifdef([PHP_INSTALL_HEADERS],
  [
    PHP_INSTALL_HEADERS([ext/zstd/], [php_zstd.h])
  ], [
    PHP_ADD_MAKEFILE_FRAGMENT
  ])
fi

dnl APCu
AC_MSG_CHECKING([for APCu includes])
if test -f "$phpincludedir/ext/apcu/apc_serializer.h"; then
  apc_inc_path="$phpincludedir"
  AC_MSG_RESULT([APCu in $apc_inc_path])
  AC_DEFINE(HAVE_APCU_SUPPORT, 1, [Whether to enable APCu support])
else
  AC_MSG_RESULT([not found])
fi

dnl coverage
PHP_ARG_ENABLE(coverage, whether to enable coverage support,
[  --enable-coverage       Enable coverage support], no, no)

if test "$PHP_COVERAGE" != "no"; then
    EXTRA_CFLAGS="--coverage"
    PHP_SUBST(EXTRA_CFLAGS)
fi
