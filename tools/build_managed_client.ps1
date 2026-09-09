param(
    [string]$CMake = (Join-Path $PSScriptRoot '../../cod2/.venv/Scripts/cmake.exe'),
    [Parameter(Mandatory=$true)][string]$ClientIwd
)
$ErrorActionPreference = 'Stop'
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$buildRoot = Join-Path $repoRoot 'build/win-Managed'
$compilerRoot = Join-Path $repoRoot 'tools/mingw/bin'
$ninja = Join-Path (Split-Path $CMake) 'ninja.exe'
$ClientIwd = (Resolve-Path -LiteralPath $ClientIwd).Path
$previousPath = $env:PATH
try {
    $env:PATH = "$compilerRoot;$(Split-Path $CMake);$previousPath"
    & $CMake -S $repoRoot -B $buildRoot -G Ninja -DCMAKE_BUILD_TYPE=Release `
        -DREFORGED_MANAGED_CLIENT=ON "-DREFORGED_CLIENT_IWD=$ClientIwd" "-DCMAKE_C_COMPILER=$compilerRoot/gcc.exe" `
        "-DCMAKE_CXX_COMPILER=$compilerRoot/g++.exe" "-DCMAKE_ASM_NASM_COMPILER=$compilerRoot/nasm.exe" `
        "-DCMAKE_RC_COMPILER=$compilerRoot/windres.exe" "-DCMAKE_MAKE_PROGRAM=$ninja"
    if ($LASTEXITCODE -ne 0) { throw 'Managed CMake configuration failed' }
    foreach ($pass in 1..2) {
        & $CMake --build $buildRoot --config Release --parallel 4
        if ($LASTEXITCODE -ne 0) { throw "Managed build pass $pass failed" }
    }
    $dll = Join-Path $buildRoot 'managed/mss32.build.dll'
    $iwd = $ClientIwd
    $policy = [ordered]@{
        schemaVersion = 1
        updateOwner = 'reforged-launcher'
        upstreamTelemetry = $false
        embeddedIwd = 'verify-only'
        dllSha256 = (Get-FileHash -LiteralPath $dll -Algorithm SHA256).Hash.ToLowerInvariant()
        iwdSha256 = (Get-FileHash -LiteralPath $iwd -Algorithm SHA256).Hash.ToLowerInvariant()
    }
    [IO.File]::WriteAllText((Join-Path $buildRoot 'managed/reforged-client-policy.json'),
        ($policy | ConvertTo-Json) + "`n", [Text.UTF8Encoding]::new($false))
    Write-Output "Managed artifact: $dll"
    Write-Output "DLL SHA-256: $($policy.dllSha256)"
} finally {
    $env:PATH = $previousPath
}