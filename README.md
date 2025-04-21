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
zstd.output\_compression\_level | -1      | PHP\_INI\_ALL
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
    Specify a value between 0 to 22.
    The default value of -1 uses internally defined values (3).

* zstd.output\_compression\_dict _string_

    Specifies the path to the compressed dictionary file to be
    used by the output handler.

## Constant

Name                           | Description
-------------------------------| -----------
ZSTD\_COMPRESS\_LEVEL\_MIN     | Minimal compress level value
ZSTD\_COMPRESS\_LEVEL\_MAX     | Maximal compress level value
ZSTD\_COMPRESS\_LEVEL\_DEFAULT | Default compress level value
LIBZSTD\_VERSION\_NUMBER       | libzstd version number
LIBZSTD\_VERSION\_STRING       | libzstd version string

## Function

* zstd\_compress — Zstandard compression
* zstd\_uncompress — Zstandard decompression
* zstd\_compress\_dict — Zstandard compression using a digested dictionary
* zstd\_uncompress\_dict — Zstandard decompression using a digested dictionary
* zstd\_compress_\init — Initialize an incremental compress context
* zstd\_compress\_add — Incrementally compress data
* zstd\_uncompress_\init — Initialize an incremental uncompress context
* zstd\_uncompress\_add — Incrementally uncompress data


### zstd\_compress — Zstandard compression

#### Description

string **zstd\_compress** ( string _$data_ [, int _$level_ = 3 ] )

Zstandard compression.

#### Parameters

* _data_

  The string to compress.

* _level_

  The level of compression (1-22).
  (Defaults to 3)

  A value smaller than 0 means a faster compression level.
  (Zstandard library 1.3.4 or later)

#### Return Values

Returns the compressed data or FALSE if an error occurred.


### zstd\_uncompress — Zstandard decompression

#### Description

string **zstd\_uncompress** ( string _$data_ )

Zstandard decompression.

> Alias: zstd\_decompress

#### Parameters

* _data_

  The compressed string.

#### Return Values

Returns the decompressed data or FALSE if an error occurred.


### zstd\_compress\_dict — Zstandard compression using a digested dictionary

#### Description

string **zstd\_compress\_dict** ( string _$data_ , string _$dict_ [, int _$level_ = 3 ])

Zstandard compression using a digested dictionary.

> Alias: zstd\_compress\_usingcdict

#### Parameters

* _data_

  The string to compress.

* _dict_

  The Dictionary data.

* _level_

  The level of compression (1-22).
  (Defaults to 3)

#### Return Values

Returns the compressed data or FALSE if an error occurred.


### zstd\_uncompress\_dict — Zstandard decompression using a digested dictionary

#### Description

string **zstd\_uncompress\_dict** ( string _$data_ , string _$dict_ )

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


### zstd\_compress\_init — Initialize an incremental compress context

#### Description

resource **zstd\_compress\_init** ( [ int _$level_ = ZSTD_COMPRESS_LEVEL_DEFAULT ] )

Initialize an incremental compress context

#### Parameters

* _level_

  The higher the level, the slower the compression. (Defaults to `ZSTD_COMPRESS_LEVEL_DEFAULT`)

#### Return Values

Returns a zstd context resource (zstd.state) on success, or FALSE on failure


### zstd\_compress\_add — Incrementally compress data

#### Description

string **zstd\_compress\_add** ( resource _$context_, string _$data_ [, bool _$end_ = false ] )

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


### zstd\_uncompress\_init — Initialize an incremental uncompress context

#### Description

resource **zstd\_uncompress\_init** ( void )

Initialize an incremental uncompress context

#### Return Values

Returns a zstd context resource (zstd.state) on success, or FALSE on failure


### zstd\_uncompress\_add — Incrementally uncompress data

#### Description

string **zstd\_uncompress\_add** ( resource _$context_, string _$data_ )

Incrementally uncompress data

#### Parameters

* _context_

  A context created with `zstd_uncompress_init()`.

* _data_

  A chunk of compressed data.

#### Return Values

Returns a chunk of uncompressed data, or FALSE on failure.


## Namespace

```
Namespace Zstd;

function compress( $data [, $level = 3 ] )
function uncompress( $data )
function compress_dict ( $data, $dict )
function uncompress_dict ( $data, $dict )
function compress_init ( [ $level = 3 ] )
function compress_add ( $context, $data [, $end = false ] )
function uncompress_init ()
function uncompress_add ( $context, $data )

```

`zstd_compress`, `zstd_uncompress`, `zstd_compress_dict`,
`zstd_uncompress_dict`, `zstd_compress_init`, `zstd_compress_add`,
`zstd_uncompress_init` and `zstd_uncompress_add` function alias.

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
        ],
    ],
);

file_put_contents("compress.zstd:///path/to/data.zstd", $data, context: $context);
readfile("compress.zstd:///path/to/data.zstd", context: $context);
```
