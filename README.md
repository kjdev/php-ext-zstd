# Zstd Extension for PHP

[![Build Status](https://secure.travis-ci.org/kjdev/php-ext-zstd.png?branch=master)](https://travis-ci.org/kjdev/php-ext-zstd)

This extension allows Zstandard.

Documentation for Zstandard can be found at [» https://github.com/facebook/zstd](https://github.com/facebook/zstd).


## Build

``` bash
% git clone --recursive --depth=1 https://github.com/kjdev/php-ext-zstd.git
% phpize
% ./configure
% make
% make install
```

## Configration

zstd.ini:

```
extension=zstd.so
```

## Function

* zstd\_compress — Zstandard compression
* zstd\_uncompress — Zstandard decompression

### zstd\_compress — Zstandard compression

#### Description

string **zstd\_compress** ( string _$data_ )

Zstandard compression.

#### Pameters

* _data_

  The string to compress.

#### Return Values

Returns the compressed data or FALSE if an error occurred.


### zstd\_uncompress — Zstandard decompression

#### Description

string **zstd\_uncompress** ( string _$data_ )

Zstandard decompression.

#### Pameters

* _data_

  The compressed string.

#### Return Values

Returns the decompressed data or FALSE if an error occurred.

## Namespace

```
Namespace Zstd;

function compress( $data )
function uncompress( $data )
```

`zstd_compress` and `zstd_uncompress` function alias.


## Examples

```php
$data = zstd_compress('test');
zstd_uncompress($data);
```
