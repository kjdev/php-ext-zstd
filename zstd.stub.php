<?php

namespace {

  /**
   * @var int
   */
   const ZSTD_COMPRESS_LEVEL_MIN = 1;

  /**
   * @var int
   */
   const ZSTD_COMPRESS_LEVEL_MAX = UNKNOWN;

  /**
   * @var int
   * @cvalue ZSTD_CLEVEL_DEFAULT
   */
   const ZSTD_COMPRESS_LEVEL_DEFAULT = UNKNOWN;

  /**
   * @var int
   * @cvalue ZSTD_VERSION_NUMBER
   */
   const ZSTD_VERSION_NUMBER = UNKNOWN;

  /**
   * @var string
   * @cvalue ZSTD_VERSION_TEXT
   */
   const ZSTD_VERSION_TEXT = UNKNOWN;

  function zstd_compress(string $data, int $level = ZSTD_COMPRESS_LEVEL_DEFAULT): string|false {}

  function zstd_uncompress(string $data): string|false {}

  function zstd_compress_dict(string $data, string $dict, int $level = ZSTD_COMPRESS_LEVEL_DEFAULT): string|false {}

  function zstd_uncompress_dict(string $data, string $dict): string|false {}

  function zstd_compress_init(int $level = ZSTD_COMPRESS_LEVEL_DEFAULT): Zstd\Compress\Context|false {}

  function zstd_compress_add(Zstd\Compress\Context $context, string $data, bool $end = false): string|false {}

  function zstd_uncompress_init(): Zstd\UnCompress\Context|false {}

  function zstd_uncompress_add(Zstd\UnCompress\Context $context, string $data): string|false {}

}

namespace Zstd {

  function compress(string $data, int $level = ZSTD_COMPRESS_LEVEL_DEFAULT): string|false {}

  function uncompress(string $data): string|false {}

  function compress_dict(string $data, string $dict, int $level = ZSTD_COMPRESS_LEVEL_DEFAULT): string|false {}

  function uncompress_dict(string $data, string $dict): string|false {}

  function compress_init(int $level = ZSTD_COMPRESS_LEVEL_DEFAULT): Compress\Context|false {}

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
