$ErrorActionPreference = "Stop"

$env:PATH = "C:\php\devel;C:\php\bin;C:\php\deps\bin;$env:PATH"

$task = New-Item 'task.bat' -Force
Add-Content $task 'call phpize 2>&1'
Add-Content $task "call configure --with-php-build=C:\php\deps --enable-$env:PHP_EXT --enable-debug-pack 2>&1"
Add-Content $task 'nmake /nologo 2>&1'
Add-Content $task 'exit %errorlevel%'
& "C:\php\php-sdk-$env:BIN_SDK_VER\phpsdk-$env:VS-$env:ARCH.bat" -t $task
if (-not $?) {
    throw "building failed with errorlevel $LastExitCode"
}

$dname = ''
if ($env:ARCH -eq 'x64') {
    $dname += 'x64\'
}
$dname += 'Release';
if ($env:TS -eq 'ts') {
    $dname += '_TS'
}
Copy-Item "$dname\php_$env:PHP_EXT.dll" "C:\php\bin\ext\php_$env:PHP_EXT.dll"
Copy-Item "$dname\php_$env:PHP_EXT.dll" "php_$env:PHP_EXT.dll"

$ini = New-Item "C:\php\bin\php.ini" -Force
Add-Content $ini "extension_dir=C:\php\bin\ext"
if (Test-Path 'C:\php\bin\ext\php_apcu.dll') {
    Add-Content $ini 'extension=php_apcu.dll'
}
Add-Content $ini "extension=php_openssl.dll"
Add-Content $ini "extension=php_$env:PHP_EXT.dll"
