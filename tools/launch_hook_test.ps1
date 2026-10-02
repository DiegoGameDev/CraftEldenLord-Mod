[CmdletBinding()]
param(
    [string]$GameDirectory = 'D:\SteamLibrary\steamapps\common\ELDEN RING\Game',
    [string]$ModEngineDirectory = 'C:\Users\DiegoMogger\Downloads\install mods\elden e mine mod dependences\mod wngine 2',
    [switch]$CheckOnly
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$launcher = Join-Path $ModEngineDirectory 'modengine2_launcher.exe'
$engine = Join-Path $ModEngineDirectory 'modengine2\bin\modengine2.dll'
$gameExe = Join-Path $GameDirectory 'eldenring.exe'
$config = Join-Path $projectRoot 'config\modengine2_hook_test.toml'
$dll = Join-Path $projectRoot 'elden-ring-hooker\build\Release\MinecraftEldenBridge.dll'
$exampleSelection = Join-Path $projectRoot 'shared-formats\example_selection.json'
$stateDirectory = Join-Path $projectRoot 'runtime-state'
$selection = Join-Path $stateDirectory 'minecraft_selection.json'
$logDirectory = Join-Path $projectRoot 'runtime-logs\game'
foreach ($file in @($launcher, $engine, $gameExe, $config, $dll, $exampleSelection)) {
    if (-not (Test-Path -LiteralPath $file -PathType Leaf)) { throw "Required file not found: $file" }
}
New-Item -ItemType Directory -Force -Path $stateDirectory | Out-Null
if (-not (Test-Path -LiteralPath $selection -PathType Leaf)) {
    Copy-Item -LiteralPath $exampleSelection -Destination $selection
}
Write-Output "Game: $gameExe"
Write-Output "Config: $config"
Write-Output "Selection: $selection"
Write-Output "Log: $(Join-Path $logDirectory 'MinecraftEldenBridge.log')"
Write-Output 'Polling: 333 ms (about 20 frames at 60 FPS)'
if ($CheckOnly) {
    Write-Output 'Paths verified. Game was not launched.'
    return
}
if (Get-Process -Name eldenring -ErrorAction SilentlyContinue) {
    throw 'Close Elden Ring before starting this test.'
}
$previousSelection = $env:MEB_SELECTION_JSON
$previousLog = $env:MEB_LOG_DIRECTORY
$previousPoll = $env:MEB_SELECTION_POLL_MS
Push-Location -LiteralPath $ModEngineDirectory
try {
    $env:MEB_SELECTION_JSON = $selection
    $env:MEB_LOG_DIRECTORY = $logDirectory
    $env:MEB_SELECTION_POLL_MS = '333'
    Write-Output "Launching test at $(Get-Date -Format o). Check for new timestamps in the log."
    & $launcher -t er -p $gameExe -c $config
    if ($LASTEXITCODE -ne 0) { throw "Mod Engine 2 launcher exited with code $LASTEXITCODE." }
    Write-Output 'Launcher returned successfully; confirm initialization in the hook log.'
} finally {
    $env:MEB_SELECTION_JSON = $previousSelection
    $env:MEB_LOG_DIRECTORY = $previousLog
    $env:MEB_SELECTION_POLL_MS = $previousPoll
    Pop-Location
}
