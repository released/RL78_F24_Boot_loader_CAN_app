param([string]$ToolProject='', [switch]$CModels, [switch]$TransferModels)
$ErrorActionPreference='Stop'
& (Join-Path $PSScriptRoot 'tools\test_device.ps1')
if($TransferModels){& (Join-Path $PSScriptRoot 'tools\test_transfer.ps1')}
if(!$CModels -and !$ToolProject){return}
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$installation=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $installation){throw 'MSVC x64 tools required for 32-bit-long host tests.'}
$bootstrap=Join-Path $installation 'Common7\Tools\VsDevCmd.bat'
$environmentLines=& cmd /d /c ('call "'+$bootstrap+'" -no_logo -arch=amd64 >nul && set')
if($LASTEXITCODE -ne 0){throw 'MSVC environment setup failed.'}
foreach($line in $environmentLines){if($line -match '^([^=]+)=(.*)$'){[Environment]::SetEnvironmentVariable($Matches[1],$Matches[2],'Process')}}
$output=Join-Path $PSScriptRoot 'tmp\tests'
New-Item -ItemType Directory -Path $output -Force | Out-Null
$arguments=@('/nologo','/std:c++17','/W4','/WX','/EHsc',('/I'+$PSScriptRoot),('/Fe'+(Join-Path $output 'device_contract_tests.exe')))
$sources=@((Join-Path $PSScriptRoot 'tools\tests\device_contract_tests.cpp'),
    (Join-Path $PSScriptRoot 'hal\image\image_metadata.c'),(Join-Path $PSScriptRoot 'hal\image\image_metadata_profile.c'))
if($ToolProject){
    $core=Join-Path (Resolve-Path -LiteralPath $ToolProject).Path 'src\core'
    $arguments+=@('/DF24_TEST_TOOL',('/I'+$core))
    $sources+=@(Join-Path $core 'firmware_update_profile.cpp')
}
Push-Location -LiteralPath $output
try {
    & cl @arguments @sources
    if($LASTEXITCODE -ne 0){throw 'C/C++ RAM helper test build failed.'}
    & (Join-Path $output 'device_contract_tests.exe') (Join-Path $PSScriptRoot 'DefaultBuild\RL78_F24_Boot_loader_CAN_app_update.bin')
    if($LASTEXITCODE -ne 0){throw 'C/C++ RAM helper tests failed.'}
} finally { Pop-Location }
