param(
    [Parameter(Mandatory = $true)][string]$OutputDirectory,
    [string]$GbdkRoot = $env:GBDK_HOME
)
$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($GbdkRoot)) { throw 'Set GBDK_HOME or pass -GbdkRoot.' }
$compiler = Join-Path $GbdkRoot 'bin/lcc.exe'
if (-not (Test-Path -LiteralPath $compiler)) { throw 'GBDK lcc.exe was not found.' }
$outputRoot = [IO.Path]::GetFullPath($OutputDirectory)
foreach ($language in @('zh', 'en')) {
    $source = Join-Path $PSScriptRoot ('source/menu-' + $language)
    $build = Join-Path $outputRoot $language
    New-Item -ItemType Directory -Path $build -Force | Out-Null
    $entry = if ($language -eq 'zh') { 'main_zh.c' } else { 'main.c' }
    $define = if ($language -eq 'zh') { '-DMENU_BUILD_ZH=1' } else { '-DMENU_BUILD_ZH=0' }
    Push-Location $source
    try {
        foreach ($unit in @(@($entry, 'main.o'), @('ass/sound.c', 'sound.o'), @('common.c', 'common.o'))) {
            & $compiler '-DUSE_SFR_FOR_REG' $define '-c' '-o' (Join-Path $build $unit[1]) $unit[0]
            if ($LASTEXITCODE -ne 0) { throw ('Compilation failed: ' + $unit[0]) }
        }
        & $compiler '-DUSE_SFR_FOR_REG' '-o' (Join-Path $outputRoot ('menu_' + $language + '.gb')) (Join-Path $build 'main.o') (Join-Path $build 'sound.o') (Join-Path $build 'common.o')
        if ($LASTEXITCODE -ne 0) { throw ('Linking failed: ' + $language) }
    } finally { Pop-Location }
}
