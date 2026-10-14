# Build the MPIR proxy, check that its export table matches the DLL already
# in the repo, and run mpz_mul / mpz_add through every variant this CPU can
# execute. Used by .github/workflows/build-proxy.yml.
$ErrorActionPreference = "Stop"

function Import-VcVars([string]$VcVars) {
    $bat = Join-Path $env:TEMP "mpir-proxy-vcvars.cmd"
    @(
        "@echo off",
        "call `"$VcVars`" >nul",
        "if errorlevel 1 exit /b 1",
        "set"
    ) | Set-Content -Path $bat -Encoding ascii
    $pairs = cmd /c $bat
    if ($LASTEXITCODE -ne 0) {
        throw "vcvars failed: $VcVars"
    }
    foreach ($line in $pairs) {
        $eq = $line.IndexOf("=")
        if ($eq -lt 1) { continue }
        [System.Environment]::SetEnvironmentVariable(
            $line.Substring(0, $eq),
            $line.Substring($eq + 1),
            "Process")
    }
}

function Convert-RvaToOffset([byte[]]$Bytes, [int]$SectionTable, [int]$SectionCount, [uint32]$Rva, [string]$Path) {
    for ($i = 0; $i -lt $SectionCount; $i++) {
        $s = $SectionTable + ($i * 40)
        $vsz = [BitConverter]::ToUInt32($Bytes, $s + 8)
        $va = [BitConverter]::ToUInt32($Bytes, $s + 12)
        $rawSz = [BitConverter]::ToUInt32($Bytes, $s + 16)
        $raw = [BitConverter]::ToUInt32($Bytes, $s + 20)
        $span = [Math]::Max($vsz, $rawSz)
        if ($Rva -ge $va -and $Rva -lt ($va + $span)) {
            return [int]($raw + ($Rva - $va))
        }
    }
    throw "RVA $Rva is outside every section of $Path"
}

function Get-ExportPairs([string]$Dll) {
    $path = (Resolve-Path $Dll).Path
    $bytes = [System.IO.File]::ReadAllBytes($path)
    $pe = [BitConverter]::ToInt32($bytes, 0x3C)
    $sig = [System.Text.Encoding]::ASCII.GetString($bytes, $pe, 4)
    if ($sig -ne "PE`0`0") {
        throw "not a PE file: $path"
    }
    $coff = $pe + 4
    $nSections = [BitConverter]::ToUInt16($bytes, $coff + 2)
    $optSize = [BitConverter]::ToUInt16($bytes, $coff + 16)
    $opt = $coff + 20
    $magic = [BitConverter]::ToUInt16($bytes, $opt)
    if ($magic -ne 0x20B) {
        throw "expected a PE32+ DLL: $path"
    }
    $exportRva = [BitConverter]::ToUInt32($bytes, $opt + 112)
    if ($exportRva -eq 0) {
        throw "no export directory: $path"
    }
    $sec = $opt + $optSize
    $exp = Convert-RvaToOffset $bytes $sec $nSections $exportRva $path
    $ordinalBase = [BitConverter]::ToUInt32($bytes, $exp + 16)
    $nameCount = [BitConverter]::ToUInt32($bytes, $exp + 24)
    $namesOff = Convert-RvaToOffset $bytes $sec $nSections ([BitConverter]::ToUInt32($bytes, $exp + 32)) $path
    $ordsOff = Convert-RvaToOffset $bytes $sec $nSections ([BitConverter]::ToUInt32($bytes, $exp + 36)) $path
    $pairs = New-Object System.Collections.Generic.List[string]
    for ($i = 0; $i -lt $nameCount; $i++) {
        $nameOff = Convert-RvaToOffset $bytes $sec $nSections ([BitConverter]::ToUInt32($bytes, $namesOff + (4 * $i))) $path
        $end = $nameOff
        while ($bytes[$end] -ne 0) { $end++ }
        $name = [System.Text.Encoding]::ASCII.GetString($bytes, $nameOff, $end - $nameOff)
        $ordinal = $ordinalBase + [BitConverter]::ToUInt16($bytes, $ordsOff + (2 * $i))
        $pairs.Add(("{0:D4} {1}" -f $ordinal, $name))
    }
    return @($pairs.ToArray() | Sort-Object)
}

$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    throw "vswhere.exe not found"
}

$install = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $install) {
    throw "Visual C++ toolset is not installed"
}

# The wildcard path matches the PlatformToolsets directory itself. The toolset
# id (v142, v143, ...) is the directory inside it.
$toolsetNames = @(Get-ChildItem -Path "$install\MSBuild\Microsoft\VC" -Recurse -Directory -ErrorAction SilentlyContinue |
    Where-Object { $_.Parent.Name -eq "PlatformToolsets" -and $_.FullName -match '\\x64\\PlatformToolsets\\' -and $_.Name -match '^v\d+$' } |
    Select-Object -ExpandProperty Name -Unique)
Write-Host "available toolsets: $($toolsetNames -join ', ')"
if ($toolsetNames.Count -eq 0) {
    throw "No x64 platform toolset found under $install"
}
if ($toolsetNames -contains "v142") {
    $toolset = "v142"
} else {
    $toolset = $toolsetNames | Sort-Object | Select-Object -Last 1
}

$sdkRoot = Join-Path ${env:ProgramFiles(x86)} "Windows Kits\10\Include"
$sdk = Get-ChildItem $sdkRoot -Directory -ErrorAction SilentlyContinue |
    Where-Object { $_.Name -match '^10\.\d+\.\d+\.\d+$' } |
    Sort-Object Name |
    Select-Object -Last 1 -ExpandProperty Name

$msbuild = & $vswhere -latest -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe" | Select-Object -First 1
if (-not $msbuild) {
    throw "MSBuild.exe not found"
}

$vcvars = Join-Path $install "VC\Auxiliary\Build\vcvars64.bat"
Import-VcVars $vcvars

$msbuildArgs = @(
    "proxy\mpir.vcxproj",
    "/p:Configuration=Release",
    "/p:Platform=x64",
    "/p:PlatformToolset=$toolset",
    "/m",
    "/v:m"
)
if ($sdk) {
    $msbuildArgs += "/p:WindowsTargetPlatformVersion=$sdk"
}

Write-Host "msbuild: $msbuild"
Write-Host "toolset: $toolset"
Write-Host "sdk: $sdk"
& $msbuild @msbuildArgs
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$built = Get-ChildItem -Path proxy -Recurse -Filter "mpir.dll" |
    Where-Object { $_.FullName -match '\\x64\\Release\\' } |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1
if (-not $built) {
    throw "Release x64 mpir.dll was not produced"
}
$pdbPath = Join-Path $built.DirectoryName ($built.BaseName + ".pdb")
if (-not (Test-Path $pdbPath)) {
    throw "PDB missing next to $($built.FullName)"
}

$oldExports = @(Get-ExportPairs (Join-Path $root "mpir.dll"))
$newExports = @(Get-ExportPairs $built.FullName)
Write-Host "export count: old=$($oldExports.Count) new=$($newExports.Count)"
$diff = Compare-Object -ReferenceObject $oldExports -DifferenceObject $newExports
if ($diff) {
    $diff | Format-Table -AutoSize | Out-String | Write-Host
    throw "Export ordinals or names differ from the committed mpir.dll. mpir.lib and mpir.exp must stay unchanged."
}
if ($newExports.Count -ne 594) {
    throw "Expected 594 exports, found $($newExports.Count)"
}

& cl /nologo /O2 /W3 /WX /Fo:proxy\ /Fe:proxy\smoke.exe proxy\smoke.c
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$run = Join-Path $root "proxy\testrun"
if (Test-Path $run) {
    Remove-Item $run -Recurse -Force
}
New-Item -ItemType Directory -Path $run | Out-Null
Copy-Item $built.FullName (Join-Path $run "mpir.dll")
Copy-Item (Join-Path $root "mpir_*.dll") $run
Copy-Item (Join-Path $root "proxy\smoke.exe") $run

$log = Join-Path $root "smoke-log.txt"
if (Test-Path $log) {
    Remove-Item $log
}
Push-Location $run
try {
    cmd /c "smoke.exe --plan > plan.txt 2>&1"
    if ($LASTEXITCODE -ne 0) {
        Get-Content plan.txt
        throw "smoke --plan failed"
    }
    Get-Content plan.txt | Tee-Object -FilePath $log | Out-Host

    $autoArch = $null
    $runs = @()
    foreach ($line in Get-Content plan.txt) {
        if ($line -match '^AUTO (\S+)$') {
            $autoArch = $Matches[1]
        } elseif ($line -match '^RUN (.+)$') {
            $runs += $Matches[1].Trim()
        }
    }
    $runs = @($runs)
    if (-not $autoArch) {
        throw "smoke --plan did not print AUTO <arch> from the shared table"
    }
    if ($runs -notcontains "auto") {
        throw "smoke --plan produced no RUN auto line"
    }

    $env:MPIR_PROXY_DEBUG = "1"
    foreach ($name in $runs) {
        $expect = $name
        if ($name -eq "auto") {
            Remove-Item Env:MPIR_PROXY_ARCH -ErrorAction SilentlyContinue
            $expect = $autoArch
        } else {
            $env:MPIR_PROXY_ARCH = $name
        }
        Add-Content -Path $log -Value "===== $name ====="
        Write-Host "===== $name ====="
        cmd /c "smoke.exe > section.txt 2>&1"
        $code = $LASTEXITCODE
        Get-Content section.txt | Tee-Object -FilePath $log -Append | Out-Host
        $text = Get-Content section.txt -Raw
        if ($code -ne 0) {
            throw "smoke failed for $name (exit $code)"
        }
        if ($text -notmatch "(?m)mpir proxy: selected $([regex]::Escape($expect))\s*$") {
            throw "proxy loaded a different arch than $expect for $name"
        }
        if ($text -notmatch "(?m)^ok\s*$") {
            throw "smoke did not print ok for $name"
        }
    }
} finally {
    Pop-Location
}

$artifactDir = Join-Path $root "artifacts"
if (Test-Path $artifactDir) {
    Remove-Item $artifactDir -Recurse -Force
}
New-Item -ItemType Directory -Path $artifactDir | Out-Null
Copy-Item $built.FullName (Join-Path $artifactDir "mpir.dll")
Copy-Item $pdbPath (Join-Path $artifactDir "mpir.pdb")
Copy-Item $log (Join-Path $artifactDir "smoke-log.txt")

$info = @(
    "commit=$env:GITHUB_SHA",
    "runner_image=$env:ImageVersion",
    "toolset=$toolset",
    "sdk=$sdk",
    "msbuild=$msbuild",
    "built_dll=$($built.FullName)"
)
$info | Set-Content (Join-Path $artifactDir "build-info.txt")
$info | Write-Host
Write-Host "proxy build ok"
