$ErrorActionPreference = 'Stop'

$MarkerText = 'IU-REL-02 build_release.ps1 owned workspace'
$ProjectRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$SourceRoot = $ProjectRoot

if ($ProjectRoot -match '[^\x00-\x7F]') {
    if (-not $env:ComSpec) {
        throw 'The project path contains non-ASCII characters and cmd.exe is unavailable for an 8.3 path lookup. Move or clone the repository to an ASCII-only path.'
    }

    $shortPathCommand = 'for %I in ("{0}") do @echo %~sI' -f $ProjectRoot
    $shortPathOutput = & $env:ComSpec /d /c $shortPathCommand
    if ($LASTEXITCODE -ne 0 -or -not $shortPathOutput) {
        throw 'Could not resolve an ASCII-safe 8.3 path. Move or clone the repository to an ASCII-only path.'
    }

    $shortRoot = ([string]($shortPathOutput | Select-Object -Last 1)).Trim()
    if (-not $shortRoot -or $shortRoot -match '[^\x00-\x7F]' -or -not (Test-Path -LiteralPath $shortRoot -PathType Container)) {
        throw 'This volume has no usable ASCII-safe 8.3 path. Move or clone the repository to an ASCII-only path.'
    }

    $SourceRoot = [System.IO.Path]::GetFullPath($shortRoot)
}

function Get-RequiredToolPath {
    param([string]$Name)

    $command = Get-Command -Name $Name -CommandType Application -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if (-not $command) {
        throw "Required tool '$Name' was not found on PATH. Configure the matching Qt MinGW toolchain, CMake, and Ninja, then run this script again."
    }

    return (Get-Item -LiteralPath $command.Source).FullName
}

$CMakePath = Get-RequiredToolPath 'cmake.exe'
$CTestPath = Get-RequiredToolPath 'ctest.exe'
$NinjaPath = Get-RequiredToolPath 'ninja.exe'
$CompilerPath = Get-RequiredToolPath 'g++.exe'
$WindeployqtPath = Get-RequiredToolPath 'windeployqt.exe'
$QtBin = Split-Path -Parent $WindeployqtPath
$QtPrefix = Split-Path -Parent $QtBin
$CompilerBin = Split-Path -Parent $CompilerPath

$qtConfig = Join-Path $QtPrefix 'lib\cmake\Qt6\Qt6Config.cmake'
if (-not (Test-Path -LiteralPath $qtConfig -PathType Leaf)) {
    throw "The Qt prefix derived from windeployqt is not a usable Qt installation: $QtPrefix"
}

$compilerTarget = (& $CompilerPath -dumpmachine | Out-String).Trim()
if ($LASTEXITCODE -ne 0 -or $compilerTarget -notmatch 'mingw') {
    throw "The detected g++ is not a MinGW compiler: $CompilerPath"
}

$deployVersion = (& $WindeployqtPath --version | Out-String).Trim()
if ($LASTEXITCODE -ne 0) {
    throw "Could not query the detected windeployqt: $WindeployqtPath"
}

$BuildRoot = [System.IO.Path]::GetFullPath((Join-Path $SourceRoot 'build'))
$WorkRoot = [System.IO.Path]::GetFullPath((Join-Path $BuildRoot 'iu-rel-02-release'))
if ([System.IO.Path]::GetDirectoryName($WorkRoot) -ne $BuildRoot) {
    throw 'Refusing to use a release work directory outside the designated build location.'
}

$MarkerPath = Join-Path $WorkRoot '.build_release_owned'
if (Test-Path -LiteralPath $WorkRoot -PathType Container) {
    $workItem = Get-Item -LiteralPath $WorkRoot -Force
    if (($workItem.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Refusing to refresh a redirected release work directory: $WorkRoot"
    }
    if (-not (Test-Path -LiteralPath $MarkerPath -PathType Leaf) -or
        (Get-Content -LiteralPath $MarkerPath -Raw).Trim() -ne $MarkerText) {
        throw "Refusing to refresh an unmarked directory: $WorkRoot"
    }

    if (Test-Path -LiteralPath (Join-Path $WorkRoot 'previous-release')) {
        throw 'A previous release backup needs manual inspection before this script can run again.'
    }

    foreach ($ownedName in @('build', 'package-staging')) {
        $targetPath = [System.IO.Path]::GetFullPath((Join-Path $WorkRoot $ownedName))
        if (Test-Path -LiteralPath $targetPath) {
            if ([System.IO.Path]::GetDirectoryName($targetPath) -ne $WorkRoot) {
                throw "Refusing to remove an unexpected build-workspace path: $targetPath"
            }
            $targetItem = Get-Item -LiteralPath $targetPath -Force
            if (($targetItem.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw "Refusing to remove a redirected build-workspace path: $targetPath"
            }
            Remove-Item -LiteralPath $targetPath -Recurse -Force
        }
    }
}
else {
    New-Item -ItemType Directory -Path $WorkRoot | Out-Null
    Set-Content -LiteralPath $MarkerPath -Value $MarkerText -Encoding ASCII
}

$BuildDir = Join-Path $WorkRoot 'build'
$StagingDir = Join-Path $WorkRoot 'package-staging'
$RequiredData = @(
    'students.txt',
    'administrators.txt',
    'records.txt',
    'diaries.txt',
    'operation_logs.csv'
)
$RequiredRuntimeFiles = @(
    '校园雷锋日记.exe',
    'Qt6Core.dll',
    'Qt6Gui.dll',
    'Qt6Widgets.dll',
    'platforms\qwindows.dll',
    'libstdc++-6.dll',
    'libgcc_s_seh-1.dll',
    'libwinpthread-1.dll'
)

$SourceDataDir = Join-Path $SourceRoot 'data'
$SourceDataHashes = @{}
foreach ($name in $RequiredData) {
    $sourceFile = Join-Path $SourceDataDir $name
    if (-not (Test-Path -LiteralPath $sourceFile -PathType Leaf)) {
        throw "Required source demo data file is missing: data\$name"
    }
    $SourceDataHashes[$name] = (Get-FileHash -LiteralPath $sourceFile -Algorithm SHA256).Hash
}

Write-Host "CMake:       $CMakePath"
Write-Host "CTest:       $CTestPath"
Write-Host "Ninja:       $NinjaPath"
Write-Host "g++:         $CompilerPath ($compilerTarget)"
Write-Host "windeployqt: $WindeployqtPath ($deployVersion)"
if ($SourceRoot -eq $ProjectRoot) {
    Write-Host 'Source path: normal ASCII-safe path'
}
else {
    Write-Host "Source path: 8.3 short path $SourceRoot"
}

Write-Host '[1/5] Fresh Release configure'
& $CMakePath -S $SourceRoot -B $BuildDir -G Ninja "-DCMAKE_MAKE_PROGRAM=$NinjaPath" "-DCMAKE_CXX_COMPILER=$CompilerPath" "-DCMAKE_PREFIX_PATH=$QtPrefix" '-DCMAKE_BUILD_TYPE=Release'
if ($LASTEXITCODE -ne 0) {
    throw "CMake configure failed with exit code $LASTEXITCODE."
}

Write-Host '[2/5] Build Qt, Console, and Core tests'
& $CMakePath --build $BuildDir --target leifeng_qt leifeng_console core_behavior_tests --parallel
if ($LASTEXITCODE -ne 0) {
    throw "Build failed with exit code $LASTEXITCODE."
}

Write-Host '[3/5] Run CTest'
& $CTestPath --test-dir $BuildDir --output-on-failure
if ($LASTEXITCODE -ne 0) {
    throw "CTest failed with exit code $LASTEXITCODE."
}

Write-Host '[4/5] Deploy and verify a staging package'
$BuiltExe = Join-Path $BuildDir 'leifeng_qt.exe'
if (-not (Test-Path -LiteralPath $BuiltExe -PathType Leaf)) {
    throw "The Release Qt executable was not generated: $BuiltExe"
}

New-Item -ItemType Directory -Path $StagingDir | Out-Null
$StagingExe = Join-Path $StagingDir '校园雷锋日记.exe'
Copy-Item -LiteralPath $BuiltExe -Destination $StagingExe

$OriginalPath = $env:PATH
try {
    $env:PATH = "$QtBin;$CompilerBin;$OriginalPath"
    & $WindeployqtPath --release --compiler-runtime --dir $StagingDir $StagingExe
    if ($LASTEXITCODE -ne 0) {
        throw "windeployqt failed with exit code $LASTEXITCODE."
    }
}
finally {
    $env:PATH = $OriginalPath
}

$StagingDataDir = Join-Path $StagingDir 'data'
New-Item -ItemType Directory -Path $StagingDataDir | Out-Null
foreach ($name in $RequiredData) {
    Copy-Item -LiteralPath (Join-Path $SourceDataDir $name) -Destination (Join-Path $StagingDataDir $name)
}

function Assert-Package {
    param(
        [string]$PackageDir,
        [string[]]$RuntimeFiles,
        [string[]]$DataFiles,
        [hashtable]$ExpectedDataHashes
    )

    foreach ($relativePath in $RuntimeFiles) {
        $requiredPath = Join-Path $PackageDir $relativePath
        if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf)) {
            throw "Required release runtime file is missing: $relativePath"
        }
    }

    $packageDataDir = Join-Path $PackageDir 'data'
    $actualDataNames = @(Get-ChildItem -LiteralPath $packageDataDir -File -Force | ForEach-Object Name | Sort-Object)
    $expectedDataNames = @($DataFiles | Sort-Object)
    if (Compare-Object -ReferenceObject $expectedDataNames -DifferenceObject $actualDataNames) {
        throw 'The release data directory does not contain exactly the five required demo files.'
    }

    foreach ($name in $DataFiles) {
        $dataPath = Join-Path $packageDataDir $name
        $actualHash = (Get-FileHash -LiteralPath $dataPath -Algorithm SHA256).Hash
        if ($actualHash -ne $ExpectedDataHashes[$name]) {
            throw "Release data does not match repository demo data: data\$name"
        }
    }
}

Assert-Package -PackageDir $StagingDir -RuntimeFiles $RequiredRuntimeFiles -DataFiles $RequiredData -ExpectedDataHashes $SourceDataHashes

foreach ($name in $RequiredData) {
    $currentHash = (Get-FileHash -LiteralPath (Join-Path $SourceDataDir $name) -Algorithm SHA256).Hash
    if ($currentHash -ne $SourceDataHashes[$name]) {
        throw "Repository source data changed during the release workflow: data\$name"
    }
}

Write-Host '[5/5] Replace release only after staging verification'
$FinalRelease = [System.IO.Path]::GetFullPath((Join-Path $SourceRoot 'release'))
if ([System.IO.Path]::GetDirectoryName($FinalRelease) -ne $SourceRoot -or
    [System.IO.Path]::GetFileName($FinalRelease) -ne 'release') {
    throw 'Refusing to replace a release directory outside the repository root.'
}
if (Test-Path -LiteralPath $FinalRelease) {
    $releaseItem = Get-Item -LiteralPath $FinalRelease -Force
    if (($releaseItem.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Refusing to replace a redirected release directory: $FinalRelease"
    }
}

$PreviousRelease = Join-Path $WorkRoot 'previous-release'
if (Test-Path -LiteralPath $PreviousRelease) {
    throw "Unexpected previous-release backup already exists: $PreviousRelease"
}

$HadPreviousRelease = Test-Path -LiteralPath $FinalRelease -PathType Container
if ($HadPreviousRelease) {
    Move-Item -LiteralPath $FinalRelease -Destination $PreviousRelease
}

try {
    Move-Item -LiteralPath $StagingDir -Destination $FinalRelease
    Assert-Package -PackageDir $FinalRelease -RuntimeFiles $RequiredRuntimeFiles -DataFiles $RequiredData -ExpectedDataHashes $SourceDataHashes
}
catch {
    if (Test-Path -LiteralPath $FinalRelease -PathType Container) {
        if (-not (Test-Path -LiteralPath $StagingDir)) {
            Move-Item -LiteralPath $FinalRelease -Destination $StagingDir
        }
        else {
            Remove-Item -LiteralPath $FinalRelease -Recurse -Force
        }
    }
    if ($HadPreviousRelease -and (Test-Path -LiteralPath $PreviousRelease -PathType Container)) {
        Move-Item -LiteralPath $PreviousRelease -Destination $FinalRelease
    }
    throw
}

if ($HadPreviousRelease) {
    Remove-Item -LiteralPath $PreviousRelease -Recurse -Force
}

foreach ($name in $RequiredData) {
    $currentHash = (Get-FileHash -LiteralPath (Join-Path $SourceDataDir $name) -Algorithm SHA256).Hash
    if ($currentHash -ne $SourceDataHashes[$name]) {
        throw "Repository source data changed during the release workflow: data\$name"
    }
}

$exePath = Join-Path $FinalRelease '校园雷锋日记.exe'
$exeHash = (Get-FileHash -LiteralPath $exePath -Algorithm SHA256).Hash
$exeSize = (Get-Item -LiteralPath $exePath).Length
Write-Host "Release: $FinalRelease"
Write-Host "Executable size: $exeSize bytes"
Write-Host "Executable SHA-256: $exeHash"
Write-Host 'Release data SHA-256:'
foreach ($name in $RequiredData) {
    $releaseHash = (Get-FileHash -LiteralPath (Join-Path $FinalRelease "data\$name") -Algorithm SHA256).Hash
    Write-Host "  $name $releaseHash (matches source)"
}
Write-Host 'Release package generated. Runtime smoke testing is performed on a disposable copy.'
