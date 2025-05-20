<?php

namespace {

  function zstd_compress(string $data, int $level = 3): string|false {}

  function zstd_uncompress(string $data): string|false {}

  function zstd_compress_dict(string $data, string $dict, int $level = DEFAULT_COMPRESS_LEVEL): string|false {}

  function zstd_uncompress_dict(string $data, string $dict): string|false {}

  function zstd_compress_init(int $level = 3): Zstd\Compress\Context|false {}

  function zstd_compress_add(Zstd\Compress\Context $context, string $data, bool $end = false): string|false {}

  function zstd_uncompress_init(): Zstd\UnCompress\Context|false {}

  function zstd_uncompress_add(Zstd\UnCompress\Context $context, string $data): string|false {}

}

namespace Zstd {

  function compress(string $data, int $level = 3): string|false {}

  function uncompress(string $data): string|false {}

  function compress_dict(string $data, string $dict, int $level = 3): string|false {}

  function uncompress_dict(string $data, string $dict): string|false {}

  function compress_init(int $level = 3): Compress\Context|false {}

  function compress_add(Compress\Context $context, string $data, bool $end = false): string|false {}

  function uncompress_init(): UnCompress\Context|false {}

  function uncompress_add(UnCompress\Context $context, string $data): string|false {}

}

namespace Zstd\Compress {

  final class Context
  {
  }

}

namespace Zstd\UnCompress {

  final class Context
  {
  }

}
