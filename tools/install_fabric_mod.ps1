[CmdletBinding(SupportsShouldProcess)]
param(
    [string]$InstanceDirectory = 'C:\Users\DiegoMogger\AppData\Roaming\PrismLauncher\instances\EldenCraftLord Instance',
    [string]$JarPath,
    [string]$FabricApiJar = 'C:\Users\DiegoMogger\Downloads\install mods\elden e mine mod dependences\fabric-api-0.161.0+26.3.jar'
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem

function Read-ModMetadata([string]$Path) {
    $archive = [IO.Compression.ZipFile]::OpenRead($Path)
    try {
        $entry = $archive.GetEntry('fabric.mod.json')
        if ($null -eq $entry) { return $null }
        $reader = [IO.StreamReader]::new($entry.Open())
        try { return ($reader.ReadToEnd() | ConvertFrom-Json) } finally { $reader.Dispose() }
    } finally { $archive.Dispose() }
}

$projectRoot = Split-Path -Parent $PSScriptRoot
if (-not $JarPath) {
    $JarPath = Join-Path $projectRoot 'minecraft-fabric-raycast\build\libs\minecraft-elden-bridge-raycast-0.2.0.jar'
}
$pack = Get-Content -Raw -LiteralPath (Join-Path $InstanceDirectory 'mmc-pack.json') | ConvertFrom-Json
$minecraft = @($pack.components | Where-Object uid -eq 'net.minecraft')
$loader = @($pack.components | Where-Object uid -eq 'net.fabricmc.fabric-loader')
if ($minecraft.Count -ne 1 -or $minecraft[0].version -ne '26.3' -or
    $loader.Count -ne 1 -or [version]$loader[0].version -lt [version]'0.19.5') {
    throw 'Expected a Minecraft 26.3 instance with Fabric Loader >= 0.19.5. Instance was not changed.'
}
$mod = Read-ModMetadata $JarPath
if ($mod.id -ne 'minecraft_elden_bridge' -or $mod.environment -ne 'client' -or
    $mod.depends.minecraft -ne '26.3') { throw 'Not the expected client-side bridge JAR for 26.3.' }

$minecraftDirectory = Join-Path $InstanceDirectory 'minecraft'
if (-not (Test-Path -LiteralPath $minecraftDirectory -PathType Container)) {
    throw "Minecraft directory not found: $minecraftDirectory"
}
$modsDirectory = Join-Path $minecraftDirectory 'mods'
$destination = Join-Path $modsDirectory 'MinecraftEldenBridge-raycast.jar'
$hasApi = $false
if (Test-Path -LiteralPath $modsDirectory) {
    foreach ($existing in Get-ChildItem -LiteralPath $modsDirectory -Filter '*.jar') {
        $metadata = Read-ModMetadata $existing.FullName
        if ($metadata.id -eq 'minecraft_elden_bridge' -and $existing.FullName -ne $destination) {
            throw "Another bridge JAR exists: $($existing.FullName). Resolve duplicate versions manually."
        }
        if ($existing.FullName -eq $destination -and $metadata.id -ne 'minecraft_elden_bridge') {
            throw "Destination is occupied by an unrelated JAR: $destination"
        }
        if ($metadata.id -eq 'fabric-api') {
            if ($metadata.version -ne '0.161.0+26.3') {
                throw "Existing Fabric API $($metadata.version) differs from the validated version. Review it manually; no dependencies changed."
            }
            $hasApi = $true
        }
    }
}
if (-not $hasApi) {
    $api = Read-ModMetadata $FabricApiJar
    if ($api.id -ne 'fabric-api' -or $api.version -ne '0.161.0+26.3') { throw 'Unexpected Fabric API JAR.' }
    $apiDestination = Join-Path $modsDirectory (Split-Path -Leaf $FabricApiJar)
    if (Test-Path -LiteralPath $apiDestination) { throw "Dependency destination already exists: $apiDestination" }
}
if ($PSCmdlet.ShouldProcess($modsDirectory, 'Install bridge JAR and add Fabric API only if absent')) {
    New-Item -ItemType Directory -Force -Path $modsDirectory | Out-Null
    if (-not $hasApi) { Copy-Item -LiteralPath $FabricApiJar -Destination $apiDestination }
    Copy-Item -LiteralPath $JarPath -Destination $destination -Force
    Write-Output "Installed: $destination"
    Write-Output 'Fabric Loader is configured by Prism. No instance settings or existing dependencies were changed.'
}
