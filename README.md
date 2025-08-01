# Zstd Extension for PHP

[![Linux](https://github.com/kjdev/php-ext-zstd/actions/workflows/linux.yaml/badge.svg?branch=master)](https://github.com/kjdev/php-ext-zstd/actions/workflows/linux.yaml)
[![Windows](https://github.com/kjdev/php-ext-zstd/actions/workflows/windows.yaml/badge.svg?branch=master)](https://github.com/kjdev/php-ext-zstd/actions/workflows/windows.yaml)

This extension allows Zstandard.

Documentation for Zstandard can be found at [» https://github.com/facebook/zstd](https://github.com/facebook/zstd).


## Build from sources

``` bash
% git clone --recursive --depth=1 https://github.com/kjdev/php-ext-zstd.git
% cd php-ext-zstd
% phpize
% ./configure
% make
% make install
```

To use the system library

``` bash
% ./configure --with-libzstd
```

> minimum system libzstd library version to 1.4.0

Install from [pecl](https://pecl.php.net/package/zstd):

``` bash
% pecl install zstd
```

## Distribution binary packages

### Fedora

Fedora users can install the [» php-zstd](https://packages.fedoraproject.org/pkgs/php-zstd/php-zstd/) package from official repository.

``` bash
dnf install php-zstd
```

### CentOS / RHEL

CentOS / RHEL (and other clones) users can install the [» php-zstd](https://packages.fedoraproject.org/pkgs/php-zstd/php-zstd/) package from [» EPEL](https://fedoraproject.org/wiki/EPEL) repository.

``` bash
yum install php-zstd
```

Other RPM packages of this extension, for other PHP versions, are available in [» Remi's RPM repository](https://rpms.remirepo.net/).


## Configration

php.ini:

```
extension=zstd.so
```

### Output handler option

Name                            | Default | Changeable
------------------------------- | ------- | ----------
zstd.output\_compression        | 0       | PHP\_INI\_ALL
zstd.output\_compression\_level | 3       | PHP\_INI\_ALL
zstd.output\_compression\_dict  | ""      | PHP\_INI\_ALL

* zstd.output\_compression _boolean_/_integer_

    Whether to transparently compress pages.
    If this option is set to "On" in php.ini or the Apache configuration,
    pages are compressed if the browser sends an
    "Accept-Encoding: zstd" header.
    "Content-Encoding: zstd" and "Vary: Accept-Encoding" headers are added to
    the output. In runtime, it can be set only before sending any output.

* zstd.output\_compression\_level _integer_

    Compression level used for transparent output compression.
    Specify a value between 1 to 22.
    The default value of `ZSTD_COMPRESS_LEVEL_DEFAULT` (3).

* zstd.output\_compression\_dict _string_

    Specifies the path to the compressed dictionary file to be
    used by the output handler.

## Constant

Name                           | Description
-------------------------------| -----------
ZSTD\_COMPRESS\_LEVEL\_MIN     | Minimal compress level value
ZSTD\_COMPRESS\_LEVEL\_MAX     | Maximal compress level value
ZSTD\_COMPRESS\_LEVEL\_DEFAULT | Default compress level value
ZSTD\_VERSION\_NUMBER          | libzstd version number
ZSTD\_VERSION\_TEXT            | libzstd version string

## Function

* zstd\_compress — Zstandard compression
* zstd\_uncompress — Zstandard decompression
* zstd\_compress\_dict — Zstandard compression using a digested dictionary
* zstd\_uncompress\_dict — Zstandard decompression using a digested dictionary
* zstd\_compress\_init — Initialize an incremental compress context
* zstd\_compress\_add — Incrementally compress data
* zstd\_uncompress\_init — Initialize an incremental uncompress context
* zstd\_uncompress\_add — Incrementally uncompress data

---
### zstd\_compress — Zstandard compression

#### Description

``` php
zstd_compress ( string $data, int $level = ZSTD_COMPRESS_LEVEL_DEFAULT, ?string $dict = null ): string|false
```

Zstandard compression.

#### Parameters

* _data_

  The string to compress.

* _level_

  The level of compression (e.g. 1-22).
  (Defaults to `ZSTD_COMPRESS_LEVEL_DEFAULT`)

  A value smaller than 0 means a faster compression level.
  (Zstandard library 1.3.4 or later)

* _dict_

  The Dictionary data.

#### Return Values

Returns the compressed data or FALSE if an error occurred.

---
### zstd\_uncompress — Zstandard decompression

#### Description

``` php
zstd_uncompress ( string $data, ?string $dict = null ): string|false
```

Zstandard decompression.

> Alias: zstd\_decompress

#### Parameters

* _data_

  The compressed string.

* _dict_

  The Dictionary data.

#### Return Values

Returns the decompressed data or FALSE if an error occurred.

---
### zstd\_compress\_dict — Zstandard compression using a digested dictionary

> deprecated: use zstd\_compress() insted

#### Description

``` php
zstd_compress_dict ( string $data , string $dict, int $level = ZSTD_COMPRESS_LEVEL_DEFAULT ): string|false
```

Zstandard compression using a digested dictionary.

> Alias: zstd\_compress\_usingcdict

#### Parameters

* _data_

  The string to compress.

* _dict_

  The Dictionary data.

* _level_

  The level of compression (e.g. 1-22).
  (Defaults to `ZSTD_COMPRESS_LEVEL_DEFAULT`)

#### Return Values

Returns the compressed data or FALSE if an error occurred.

---
### zstd\_uncompress\_dict — Zstandard decompression using a digested dictionary

> deprecated: use zstd\_uncompress() insted

#### Description

``` php
zstd_uncompress_dict ( string $data , string $dict ): string|false
```

Zstandard decompression using a digested dictionary.

> Alias: zstd\_decompress\_dict,
> zstd\_uncompress\_usingcdict, zstd\_decompress\_usingcdict

#### Parameters

* _data_

  The compressed string.

* _dict_

  The Dictionary data.

#### Return Values

Returns the decompressed data or FALSE if an error occurred.

---
### zstd\_compress\_init — Initialize an incremental compress context

#### Description

``` php
zstd_compress_init ( int $level = ZSTD_COMPRESS_LEVEL_DEFAULT, ?string $dict = null ): Zstd\Compress\Context|false
```

Initialize an incremental compress context

#### Parameters

* _level_

  The higher the level, the slower the compression.
  (Defaults to `ZSTD_COMPRESS_LEVEL_DEFAULT`)

* _dict_

  The Dictionary data.

#### Return Values

Returns a zstd context instance on success, or FALSE on failure

---
### zstd\_compress\_add — Incrementally compress data

#### Description

``` php
zstd_compress_add ( Zstd\Compress\Context $context, string $data, bool $end = false ): string|false
```

Incrementally compress data

#### Parameters

* _context_

  A context created with `zstd_compress_init()`.

* _data_

  A chunk of data to compress.

* _end_

  Set to true to terminate with the last chunk of data.

#### Return Values

Returns a chunk of compressed data, or FALSE on failure.

---
### zstd\_uncompress\_init — Initialize an incremental uncompress context

#### Description

``` php
zstd_uncompress_init ( ?string $dict = null ): Zstd\UnCompress\Context|false
```

Initialize an incremental uncompress context

#### Parameters

* _dict_

  The Dictionary data.

#### Return Values

Returns a zstd context instance on success, or FALSE on failure

---
### zstd\_uncompress\_add — Incrementally uncompress data

#### Description

``` php
zstd_uncompress_add ( Zstd\UnCompress\Context $context, string $data ): string|false
```

Incrementally uncompress data

#### Parameters

* _context_

  A context created with `zstd_uncompress_init()`.

* _data_

  A chunk of compressed data.

#### Return Values

Returns a chunk of uncompressed data, or FALSE on failure.


## Namespace

``` php
Namespace Zstd;

function compress( string $data, int $level = ZSTD_COMPRESS_LEVEL_DEFAULT ): string|false {}
function uncompress( string $data ): string|false {}
function compress_dict ( string $data, string $dict, int $level = ZSTD_COMPRESS_LEVEL_DEFAULT ): string|false {}
function uncompress_dict ( string $data, string $dict ): string|false {}
function compress_init ( int $level = ZSTD_COMPRESS_LEVEL_DEFAULT ): \Zstd\Compress\Context|false {}
function compress_add ( \Zstd\Compress\Context $context, string $data, bool $end = false ): string|false {}
function uncompress_init (): \Zstd\UnCompress\Context|false {}
function uncompress_add ( \Zstd\UnCompress\Context $context, string $data ): string|false
```

`zstd_compress`, `zstd_uncompress`, `zstd_compress_dict`,
`zstd_uncompress_dict`, `zstd_compress_init`, `zstd_compress_add`,
`zstd_uncompress_init` and `zstd_uncompress_add` function aliases.

## Streams

Zstd compression and decompression are available using the
`compress.zstd://` stream prefix.

## Output handler

``` php
ini_set('zstd.output_compression', 'On');
// OR
// ob_start('ob_zstd_handler');

echo ...;
```

> "Accept-Encoding: zstd" must be specified.

## Examples

```php
// Using functions
$data = zstd_compress('test');
zstd_uncompress($data);

// Using namespaced functions
$data = \Zstd\compress('test');
\Zstd\uncompress($data);

// Using streams
file_put_contents("compress.zstd:///path/to/data.zstd", $data);
readfile("compress.zstd:///path/to/data.zstd");

// Providing level of compression, when using streams 
$context = stream_context_create([
    'zstd' => [
            'level' => ZSTD_COMPRESS_LEVEL_MIN,
            // 'dict' => $dict,
        ],
    ],
);

file_put_contents("compress.zstd:///path/to/data.zstd", $data, context: $context);
readfile("compress.zstd:///path/to/data.zstd", context: $context);
```
