<?php

namespace {

  function zstd_compress(string $data, int $level = 3): string|false {}

  function zstd_uncompress(string $data): string|false {}

  function zstd_compress_dict(string $data, string $dict, int $level = DEFAULT_COMPRESS_LEVEL): string|false {}

  function zstd_uncompress_dict(string $data, string $dict): string|false {}

  /**
   * @return resource|false
   */
  function zstd_compress_init(int $level= 3) {}

  /**
   * @param resource $context
   */
  function zstd_compress_add($context, string $data, bool $end = false): string|false {}

  /**
   * @return resource|false
   */
  function zstd_uncompress_init() {}

  /**
   * @param resource $context
   */
  function zstd_uncompress_add($context, string $data): string|false {}

}

namespace Zstd {

  function compress(string $data, int $level = 3): string|false {}

  function uncompress(string $data): string|false {}

  function compress_dict(string $data, string $dict, int $level = 3): string|false {}

  function uncompress_dict(string $data, string $dict): string|false {}

  /**
   * @return resource|false
   */
  function compress_init(int $level= 3) {}

  /**
   * @param resource $context
   */
  function compress_add($context, string $data, bool $end = false): string|false {}

  /**
   * @return resource|false
   */
  function uncompress_init() {}

  /**
   * @param resource $context
   */
  function uncompress_add($context, string $data): string|false {}

}
