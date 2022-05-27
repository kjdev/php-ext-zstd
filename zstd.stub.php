<?php

namespace {

  function zstd_compress(string $data, int $level = 3): string|false {}

  function zstd_uncompress(string $data): string|false {}

  function zstd_compress_dict(string $data, string $dict, int $level = DEFAULT_COMPRESS_LEVEL): string|false {}

  function zstd_uncompress_dict(string $data, string $dict): string|false {}

}

namespace Zstd {

  function compress(string $data, int $level = 3): string|false {}

  function uncompress(string $data): string|false {}

  function compress_dict(string $data, string $dict, int $level = 3): string|false {}

  function uncompress_dict(string $data, string $dict): string|false {}

}
