param()
$ErrorActionPreference='Stop'
$boot=Split-Path -Parent $PSScriptRoot
$app=Join-Path $boot '..\RL78_F24_CAN_Transfer'
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$installation=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$installation){throw 'MSVC x64 tools required for developer-only register/protocol tests.'}
$bootstrap=Join-Path $installation 'Common7\Tools\VsDevCmd.bat'
$lines=& cmd /d /c ('call "'+$bootstrap+'" -no_logo -arch=amd64 >nul && set')
if($LASTEXITCODE -ne 0){throw 'MSVC environment setup failed.'}
foreach($line in $lines){if($line -match '^([^=]+)=(.*)$'){[Environment]::SetEnvironmentVariable($Matches[1],$Matches[2],'Process')}}
$output=Join-Path $boot 'tmp\tests\transfer'
New-Item -ItemType Directory -Path $output -Force | Out-Null
Push-Location -LiteralPath $output
try {
    foreach($trace in @(1,0)) {
        $arguments=@('/nologo','/TC','/Za','/W4','/WX',('/I'+(Join-Path $PSScriptRoot 'tests\transfer_mock')),
            ('/I'+(Join-Path $boot 'hal\device')),('/I'+$boot),('/I'+$app),
            (Join-Path $PSScriptRoot 'tests\transfer_diagnostics_tests.c'),('/Fe:transfer_trace'+$trace+'.exe'))
        if(!$trace){$arguments+='/DTRANSFER_TEST_NO_TRACE'}
        & cl @arguments
        if($LASTEXITCODE -ne 0){throw 'Transfer production-code test build failed.'}
        & (Join-Path $output ('transfer_trace'+$trace+'.exe'))
        if($LASTEXITCODE -ne 0){throw 'Transfer production-code test failed.'}
        Write-Host "PASS: Transfer UART trace=$trace model."
    }
} finally { Pop-Location }
