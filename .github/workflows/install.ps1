$ErrorActionPreference = "Stop"

if (-not (Test-Path 'C:\php')) {
    [void](New-Item 'C:\php' -ItemType 'directory')
}

# PHP SDK
$bname = "php-sdk-$env:BIN_SDK_VER.zip"
if (-not (Test-Path C:\php\$bname)) {
    echo "Download: https://github.com/php/php-sdk-binary-tools/archive/$bname"
    Invoke-WebRequest "https://github.com/php/php-sdk-binary-tools/archive/$bname" -OutFile "C:\php\$bname"
}
$dname0 = "php-sdk-binary-tools-php-sdk-$env:BIN_SDK_VER"
$dname1 = "php-sdk-$env:BIN_SDK_VER"
if (-not (Test-Path "C:\php\$dname1")) {
    Expand-Archive "C:\php\$bname" "C:\php"
    Move-Item "C:\php\$dname0" "C:\php\$dname1"
}

# PHP releases
if (-not (Test-Path C:\php\releases.json)) {
    echo "Download: https://windows.php.net/downloads/releases/releases.json"
    Invoke-WebRequest "https://windows.php.net/downloads/releases/releases.json" -OutFile "C:\php\releases.json"
}
$php_version = (Get-Content -Path "C:\php\releases.json" | ConvertFrom-Json | ForEach-Object {
    if ($_."$env:PHP_VER") {
        return $_."$env:PHP_VER".version
    } else {
        return "$env:PHP_VER"
    }
})

# PHP devel pack: "C:\php\devel"
$ts_part = ''
if ('nts' -eq $env:TS) {
    $ts_part = '-nts'
}
$bname = "php-devel-pack-$php_version$ts_part-Win32-$env:VS-$env:ARCH.zip"
if (-not (Test-Path "C:\php\$bname")) {
    try {
        echo "Download: https://windows.php.net/downloads/releases/$bname"
        Invoke-WebRequest "https://windows.php.net/downloads/releases/$bname" -OutFile "C:\php\$bname"
    } catch [System.Net.WebException] {
        echo "Downlaod: https://windows.php.net/downloads/releases/archives/$bname"
        Invoke-WebRequest "https://windows.php.net/downloads/releases/archives/$bname" -OutFile "C:\php\$bname"
    }
}
$dname = "php-$php_version-devel-$env:VS-$env:ARCH"
if (-not (Test-Path "C:\php\devel")) {
    Expand-Archive "C:\php\$bname" 'C:\php'
    if (-not (Test-Path "C:\php\$dname")) {
        $php_normalize_version = $php_version.Split("-")[0]
        $dname = "php-$php_normalize_version-devel-$env:VS-$env:ARCH"
    }
    if (-not (Test-Path "C:\php\devel")) {
        Move-Item "C:\php\$dname" "C:\php\devel"
    }
}

# PHP binary: "C:\php\bin"
$bname = "php-$php_version$ts_part-Win32-$env:VS-$env:ARCH.zip"
if (-not (Test-Path "C:\php\$bname")) {
    try {
        echo "Download: https://windows.php.net/downloads/releases/$bname"
        Invoke-WebRequest "https://windows.php.net/downloads/releases/$bname" -OutFile "C:\php\$bname"
    } catch [System.Net.WebException] {
        echo "Download: https://windows.php.net/downloads/releases/archives/$bname"
        Invoke-WebRequest "https://windows.php.net/downloads/releases/archives/$bname" -OutFile "C:\php\$bname"
    }
}
if (-not (Test-Path "C:\php\bin")) {
    Expand-Archive "C:\php\$bname" "C:\php\bin"
}

# library dependency: "C:\php\deps"
if ("$env:DEP" -ne "") {
    $bname = "$env:DEP-$env:VS-$env:ARCH.zip"
    if (-not (Test-Path "C:\php\$bname")) {
        echo "Download: https://windows.php.net/downloads/pecl/deps/$bname"
        Invoke-WebRequest "https://windows.php.net/downloads/pecl/deps/$bname" -OutFile "C:\php\$bname"
        Expand-Archive "C:\php\$bname" "C:\php\deps"
    }
}

# PECL apuc
if (('' -ne $env:PECL_APCU) -and ('8.2' -ne $env:PHP_VER) -and ('8.3' -ne $env:PHP_VER)) {
    $apcu_version = '5.1.21'

    $ts_part = 'ts'
    if ('nts' -eq $env:TS) {
        $ts_part = 'nts'
    }

    $bname = "php_apcu-$apcu_version-$env:PHP_VER-$ts_part-$env:VS-$env:ARCH.zip"

    echo "Download: https://windows.php.net/downloads/pecl/releases/apcu/$apcu_version/$bname"
    Invoke-WebRequest "https://windows.php.net/downloads/pecl/releases/apcu/$apcu_version/$bname" -OutFile "C:\php\$bname"
    Expand-Archive "C:\php\$bname" 'C:\php\bin\ext'

    $bname = "v$apcu_version.zip"

    echo "Download: https://github.com/krakjoe/apcu/archive/refs/tags/$bname"
    Invoke-WebRequest "https://github.com/krakjoe/apcu/archive/refs/tags/$bname" -OutFile "C:\php\$bname"
    Expand-Archive "C:\php\$bname" 'C:\php'

    if (-not (Test-Path 'C:\php\devel\include\ext\apcu')) {
        [void](New-Item 'C:\php\devel\include\ext\apcu' -ItemType 'directory')
    }

    Move-Item "C:\php\apcu-$apcu_version\php_apc.h" 'C:\php\devel\include\ext\apcu\php_apc.h'
    Move-Item "C:\php\apcu-$apcu_version\apc.h" 'C:\php\devel\include\ext\apcu\apc.h'
    Move-Item "C:\php\apcu-$apcu_version\apc_api.h" 'C:\php\devel\include\ext\apcu\apc_api.h'
    Move-Item "C:\php\apcu-$apcu_version\apc_cache.h" 'C:\php\devel\include\ext\apcu\apc_cache.h'
    Move-Item "C:\php\apcu-$apcu_version\apc_globals.h" 'C:\php\devel\include\ext\apcu\apc_globals.h'
    Move-Item "C:\php\apcu-$apcu_version\apc_iterator.h" 'C:\php\devel\include\ext\apcu\apc_iterator.h'
    Move-Item "C:\php\apcu-$apcu_version\apc_lock.h" 'C:\php\devel\include\ext\apcu\apc_lock.h'
    Move-Item "C:\php\apcu-$apcu_version\apc_mutex.h" 'C:\php\devel\include\ext\apcu\apc_mutex.h'
    Move-Item "C:\php\apcu-$apcu_version\apc_sma.h" 'C:\php\devel\include\ext\apcu\apc_sma.h'
    Move-Item "C:\php\apcu-$apcu_version\apc_serializer.h" 'C:\php\devel\include\ext\apcu\apc_serializer.h'
    Move-Item "C:\php\apcu-$apcu_version\apc_stack.h" 'C:\php\devel\include\ext\apcu\apc_stack.h'
    Move-Item "C:\php\apcu-$apcu_version\apc_windows_srwlock_kernel.h" 'C:\php\devel\include\ext\apcu\apc_windows_srwlock_kernel.h'
    Move-Item "C:\php\apcu-$apcu_version\apc_arginfo.h" 'C:\php\devel\include\ext\apcu\apc_arginfo.h'
    Move-Item "C:\php\apcu-$apcu_version\php_apc_legacy_arginfo.h" 'C:\php\devel\include\ext\apcu\php_apc_legacy_arginfo.h'
}
