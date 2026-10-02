[CmdletBinding(SupportsShouldProcess = $true, ConfirmImpact = 'Medium')]
param(
    [string]$GameDirectory = 'D:\SteamLibrary\steamapps\common\ELDEN RING\Game',
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Release',
    [string]$DllPath,
    [switch]$Overwrite
)
$ErrorActionPreference = 'Stop'
if (-not [System.IO.Path]::IsPathRooted($GameDirectory)) {
    throw 'GameDirectory must be an absolute path to the folder containing eldenring.exe.'
}
$game = (Resolve-Path -LiteralPath $GameDirectory).Path
if (-not (Test-Path -LiteralPath (Join-Path $game 'eldenring.exe') -PathType Leaf)) {
    throw "eldenring.exe not found in $game"
}
if ([string]::IsNullOrWhiteSpace($DllPath)) {
    $projectRoot = Split-Path -Parent $PSScriptRoot
    $DllPath = Join-Path $projectRoot "elden-ring-hooker\build\$Configuration\MinecraftEldenBridge.dll"
}
$source = (Resolve-Path -LiteralPath $DllPath).Path
if (-not (Test-Path -LiteralPath $source -PathType Leaf)) { throw 'DLL source is not a file.' }
$mods = Join-Path $game 'mods'
$destination = Join-Path $mods 'MinecraftEldenBridge.dll'
if ((Test-Path -LiteralPath $destination) -and -not $Overwrite) {
    throw 'Destination already exists. Use -Overwrite explicitly after closing the game.'
}
if ($PSCmdlet.ShouldProcess($destination, "Copy DLL from $source")) {
    $null = New-Item -ItemType Directory -Path $mods -Force
    Copy-Item -LiteralPath $source -Destination $destination -Force:$Overwrite
    Write-Output "Copied: $destination"
}
