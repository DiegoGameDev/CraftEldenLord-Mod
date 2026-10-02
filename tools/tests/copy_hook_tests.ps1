[CmdletBinding()]
param([Parameter(Mandatory = $true)][string]$DllPath)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$copyScript = Join-Path $projectRoot 'tools\copy_hook_to_game.ps1'
$scratch = Join-Path $projectRoot "elden-ring-hooker\build\test-output\copy-$([guid]::NewGuid().ToString('N'))"
$null = New-Item -ItemType Directory -Path $scratch
# A marker file exercises destination checks; it is never executed.
$null = New-Item -ItemType File -Path (Join-Path $scratch 'eldenring.exe')
$mods = Join-Path $scratch 'mods'
$destination = Join-Path $mods 'MinecraftEldenBridge.dll'
& $copyScript -GameDirectory $scratch -DllPath $DllPath -WhatIf
if (Test-Path -LiteralPath $mods) { throw 'WhatIf modified the destination.' }
& $copyScript -GameDirectory $scratch -DllPath $DllPath
if ((Get-FileHash -LiteralPath $DllPath).Hash -ne (Get-FileHash -LiteralPath $destination).Hash) {
    throw 'Copied DLL hash mismatch.'
}
$rejected = $false
try { & $copyScript -GameDirectory $scratch -DllPath $DllPath } catch { $rejected = $true }
if (-not $rejected) { throw 'Existing destination was not protected.' }
& $copyScript -GameDirectory $scratch -DllPath $DllPath -Overwrite
$rejected = $false
try { & $copyScript -GameDirectory $mods -DllPath $DllPath -WhatIf } catch { $rejected = $true }
if (-not $rejected) { throw 'Directory without eldenring.exe was accepted.' }
Write-Output 'Copy script: WhatIf, exact copy, overwrite guard and invalid destination passed.'
