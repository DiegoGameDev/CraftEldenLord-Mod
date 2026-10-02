[CmdletBinding()]
param(
    [string]$MinecraftId = 'minecraft:stone',
    [string]$Dimension = 'minecraft:overworld',
    [double]$X = 128,
    [double]$Y = 64,
    [double]$Z = -32,
    [string]$OutputPath
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    $OutputPath = Join-Path $projectRoot 'runtime-state\minecraft_selection.json'
}
if (-not [System.IO.Path]::IsPathRooted($OutputPath)) {
    throw "OutputPath must be absolute: $OutputPath"
}
$directory = Split-Path -Parent $OutputPath
New-Item -ItemType Directory -Force -Path $directory | Out-Null
$temporary = Join-Path $directory ([System.IO.Path]::GetFileName($OutputPath) + '.tmp')
$payload = [ordered]@{
    schema_version = 1
    minecraft_id = $MinecraftId
    dimension = $Dimension
    position = [ordered]@{
        x = $X
        y = $Y
        z = $Z
    }
}
$json = $payload | ConvertTo-Json -Depth 4 -Compress
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($temporary, $json, $utf8NoBom)
Move-Item -LiteralPath $temporary -Destination $OutputPath -Force
Write-Output "Selection written atomically: $OutputPath"
Write-Output $json
