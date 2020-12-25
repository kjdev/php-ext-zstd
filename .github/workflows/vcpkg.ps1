$ErrorActionPreference = "Stop"

if (-not (Test-Path 'C:\php\deps')) {
    [void](New-Item 'C:\php\deps' -ItemType 'directory')
}

$package = "$env:VCPKG_LIBRARY"
$arch = "$env:ARCH-windows"

vcpkg install "${package}:${arch}"
if (-not $?) {
    throw "installing failed with errorlevel $LastExitCode"
}

Copy-Item "C:\vcpkg\installed\$arch\bin" "C:\php\deps" -Recurse -Force
Copy-Item "C:\vcpkg\installed\$arch\include" "C:\php\deps" -Recurse -Force
Copy-Item "C:\vcpkg\installed\$arch\lib" "C:\php\deps" -Recurse -Force
