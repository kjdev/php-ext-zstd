$ErrorActionPreference = "Stop"

$env:PATH = "C:\php\devel;C:\php\bin;C:\php\deps\bin;$env:PATH"

# Toolset
# $installerDir = Join-Path "${env:ProgramFiles(x86)}\Microsoft Visual Studio" 'Installer'
# $vswherePath = Join-Path $installerDir 'vswhere.exe'
# if (-not (Test-Path $vswherePath)) {
#     if (-not (Test-Path $installerDir)) {
#         New-Item -Path $installerDir -ItemType Directory -Force | Out-Null
#     }
#     $vsWhereUrl = 'https://github.com/microsoft/vswhere/releases/latest/download/vswhere.exe'
#     Invoke-WebRequest -Uri $vsWhereUrl -OutFile $vswherePath -UseBasicParsing
# }
# if($null -eq (Get-Command vswhere -ErrorAction SilentlyContinue)) {
#     throw "vswhere is not available"
# }
$MSVCDirectory = vswhere -latest -products * -find "VC\Tools\MSVC"
$selectedToolset = $null
$minor = $null
foreach ($toolset in (Get-ChildItem $MSVCDirectory)) {
    $toolsetMajorVersion, $toolsetMinorVersion = $toolset.Name.split(".")[0,1]
    $major = 14
    switch ($env:VS) {
        'vs17' { $minorMin = 30; $minorMax = $null; break }
        'vs16' { $minorMin = 20; $minorMax = 29; break }
        Default { throw "VS version is not available" }
    }
    $majorVersionCheck = [int]$major -eq [int]$toolsetMajorVersion
    $minorLowerBoundCheck = [int]$toolsetMinorVersion -ge [int]$minorMin
    $minorUpperBoundCheck = ($null -eq $minorMax) -or ([int]$toolsetMinorVersion -le [int]$minorMax)
    if ($majorVersionCheck -and $minorLowerBoundCheck -and $minorUpperBoundCheck) {
        if($null -eq $minor -or [int]$toolsetMinorVersion -gt [int]$minor) {
            $selectedToolset = $toolset.Name.Trim()
            $minor = $toolsetMinorVersion
        }
    }
}
if (-not $selectedToolset) {
    throw "toolset not available"
}

$task = New-Item 'task.bat' -Force
Add-Content $task 'call phpize 2>&1'
Add-Content $task "call configure --with-php-build=C:\php\deps --enable-$env:PHP_EXT --enable-debug-pack 2>&1"
Add-Content $task 'nmake /nologo 2>&1'
Add-Content $task 'exit %errorlevel%'
& "C:\php\php-sdk-$env:BIN_SDK_VER\phpsdk-starter.bat" -c $env:VS -a $env:ARCH -s $selectedToolset -t $task
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
