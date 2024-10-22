<?php
if (!extension_loaded('zstd')) die('skip need ext/zstd');
if (PHP_VERSION_ID < 80000) die('skip requires PHP 8.0+');
if (false === stristr(PHP_SAPI, 'cgi')) die('skip need sapi/cgi');
