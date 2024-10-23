$ErrorActionPreference = "Stop"

$env:PATH = "C:\php\devel;C:\php\bin;C:\php\deps\bin;$env:PATH"

$env:TEST_PHP_EXECUTABLE = "C:\php\bin\php.exe"
& $env:TEST_PHP_EXECUTABLE 'run-tests.php' --color --show-diff tests
if (-not $?) {
    throw "testing failed with errorlevel $LastExitCode"
}
