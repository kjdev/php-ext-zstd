# Zstd Extension for PHP

[![Build Status](https://secure.travis-ci.org/kjdev/php-ext-zstd.png?branch=master)](https://travis-ci.org/kjdev/php-ext-zstd)

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

## Distribution binary packages

### Fedora / CentOS / RHEL

RPM packages of this extension are available in [» Remi's RPM repository](https://rpms.remirepo.net/) and are named **php-zstd**.


## Configration

zstd.ini:

```
extension=zstd.so
```

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

> Alias: zstd\_dempress\_dict,
> zstd\_unmpress\_usingcdict, zstd\_decompress\_usingcdict

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

## Examples

```php
$data = zstd_compress('test');
zstd_uncompress($data);
```
