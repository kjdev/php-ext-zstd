<?php
ob_start();
phpinfo(INFO_MODULES);
$modules = ob_get_contents();
ob_get_clean();

if (preg_match("/zstd\n([^\n]*)\n([^\n]*)\n([^\n]*)\n([^\n]*)\n/s", $modules, $info) && preg_match('/Interface Version => (.*)/', $info[0], $version)) {
    $v = explode('.', $version[1]);
    $n = ($v[0]*100*100) + ($v[1]*100) + $v[2];
    $zstd_version_number = $n;
} else {
    $zstd_version_number = 0;
}
