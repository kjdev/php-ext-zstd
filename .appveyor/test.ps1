$ErrorActionPreference = "Stop"

Set-Location C:\projects\zstd

$env:TEST_PHP_EXECUTABLE = "$env:PHP_PATH\php.exe"
& $env:TEST_PHP_EXECUTABLE 'run-tests.php' --show-diff tests
if (-not $?) {
    throw "testing failed with errorlevel $LastExitCode"
}
