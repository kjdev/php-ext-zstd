# Zstd Extension for PHP

This extension allows Zstd.

Documentation for Zstd can be found at [» https://github.com/Cyan4973/zstd](https://github.com/Cyan4973/zstd).

## Build

``` bash
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

* zstd\_compress — Zstd compression
* zstd\_uncompress — Zstd decompression

### zstd\_compress — Zstd compression

#### Description

string **zstd\_compress** ( string _$data_ )

Zstd compression.

#### Pameters

* _data_

  The string to compress.

#### Return Values

Returns the compressed data or FALSE if an error occurred.


### zstd\_uncompress — Zstd decompression

#### Description

string **zstd\_uncompress** ( string _$data_ )

Zstd decompression.

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
