<?php

namespace {

  class ZstdContext {}

  function zstd_compress(string $data, int $level = 3): string|false {}

  function zstd_uncompress(string $data): string|false {}

  function zstd_compress_dict(string $data, string $dict, int $level = DEFAULT_COMPRESS_LEVEL): string|false {}

  function zstd_uncompress_dict(string $data, string $dict): string|false {}

  function zstd_compress_init(int $level= 3): ZstdContext|false {}

  function zstd_compress_add(ZstdContext $context, string $data, bool $end = false): string|false {}

  function zstd_uncompress_init(): ZstdContext|false {}

  function zstd_uncompress_add(ZstdContext $context, string $data): string|false {}

}

namespace Zstd {

  class ZstdContext {}

  function compress(string $data, int $level = 3): string|false {}

  function uncompress(string $data): string|false {}

  function compress_dict(string $data, string $dict, int $level = 3): string|false {}

  function uncompress_dict(string $data, string $dict): string|false {}

  function compress_init(int $level= 3): ZstdContext|false {}

  function compress_add(ZstdContext $context, string $data, bool $end = false): string|false {}

  function uncompress_init(): ZstdContext|false {}

  function uncompress_add(ZstdContext $context, string $data): string|false {}

}
