# Zstd Extension for PHP

[![Linux](https://github.com/kjdev/php-ext-zstd/workflows/Linux/badge.svg?branch=master)](https://github.com/kjdev/php-ext-zstd/actions?query=workflow%3ALinux+branch%3Amaster)
[![Windows](https://github.com/kjdev/php-ext-zstd/workflows/Windows/badge.svg?branch=master)](https://github.com/kjdev/php-ext-zstd/actions?query=workflow%3AWindows+branch%3Amaster)

This extension allows Zstandard.

Documentation for Zstandard can be found at [» https://github.com/facebook/zstd](https://github.com/facebook/zstd).


## Build from sources

``` bash
% git clone --recursive --depth=1 https://github.com/kjdev/php-ext-zstd.git
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

Fedora users can install the [» php-zstd](https://apps.fedoraproject.org/packages/php-zstd) package from official repository.

``` bash
dnf install php-zstd
```

### CentOS / RHEL

CentOS / RHEL (and other clones) users can install the [» php-zstd](https://apps.fedoraproject.org/packages/php-zstd) package from [» EPEL](https://fedoraproject.org/wiki/EPEL) repository.

``` bash
yum install php-zstd
```

Other RPM packages of this extension, for other PHP versions, are available in [» Remi's RPM repository](https://rpms.remirepo.net/).


## Configration

php.ini:

```
extension=zstd.so
```

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

### zstd\_compress — Zstandard compression

#### Description

string **zstd\_compress** ( string _$data_ [, int _$level_ = 3 ] )

Zstandard compression.

#### Pameters

* _data_

  The string to compress.

* _level_

  The level of compression (1-22).
  (Defaults to 3, 0 for no compression)

  A value smaller than 0 means a faster compression level.
  (Zstandard library 1.3.4 or later)

#### Return Values

Returns the compressed data or FALSE if an error occurred.


### zstd\_uncompress — Zstandard decompression

#### Description

string **zstd\_uncompress** ( string _$data_ )

Zstandard decompression.

> Alias: zstd\_decompress

#### Pameters

* _data_

  The compressed string.

#### Return Values

Returns the decompressed data or FALSE if an error occurred.


### zstd\_compress\_dict — Zstandard compression using a digested dictionary

#### Description

string **zstd\_compress\_dict** ( string _$data_ , string _$dict_ )

Zstandard compression using a digested dictionary.

> Alias: zstd\_compress\_usingcdict

#### Pameters

* _data_

  The string to compress.

* _dict_

  The Dictionary data.

#### Return Values

Returns the compressed data or FALSE if an error occurred.


### zstd\_uncompress\_dict — Zstandard decompression using a digested dictionary

#### Description

string **zstd\_uncompress\_dict** ( string _$data_ , string _$dict_ )

Zstandard decompression using a digested dictionary.

> Alias: zstd\_decompress\_dict,
> zstd\_uncompress\_usingcdict, zstd\_decompress\_usingcdict

#### Pameters

* _data_

  The compressed string.

* _dict_

  The Dictionary data.

#### Return Values

Returns the decompressed data or FALSE if an error occurred.


## Namespace

```
Namespace Zstd;

function compress( $data [, $level = 3 ] )
function uncompress( $data )
function compress_dict ( $data, $dict )
function uncompress_dict ( $data, $dict )
```

`zstd_compress`, `zstd_uncompress`, `zstd_compress_dict` and
`zstd_uncompress_dict` function alias.

## Streams

Zstd compression and uncompression are available using the
`compress.zstd://` stream prefix.

## Examples

```php
// Using functions
$data = zstd_compress('test');
zstd_uncompress($data);

// Using namespaced functions
$data = \Zstd\compress('test');
\Zstd\uncompress($data);

// Using streams
file_put_contents("compress.zstd:///patch/to/data.zstd", $data);
readfile("compress.zstd:///patch/to/data.zstd");
```
