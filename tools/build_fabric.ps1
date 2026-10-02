[CmdletBinding()]
param(
    [string]$JavaHome = 'C:\Program Files\Java\jdk-26.0.2.1',
    [switch]$Offline
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$modRoot = Join-Path $projectRoot 'minecraft-fabric-raycast'
foreach ($tool in @('java.exe', 'javac.exe')) {
    if (-not (Test-Path -LiteralPath (Join-Path $JavaHome "bin\$tool") -PathType Leaf)) {
        throw "Windows JDK 25+ required. Missing $tool in $JavaHome. Use -JavaHome with a Windows JDK."
    }
}
$previousJavaHome = $env:JAVA_HOME
$previousGradleHome = $env:GRADLE_USER_HOME
Push-Location -LiteralPath $modRoot
try {
    $env:JAVA_HOME = $JavaHome
    $env:GRADLE_USER_HOME = Join-Path $projectRoot '.gradle-user-home'
    $arguments = @('--no-daemon', '--console=plain', 'build')
    if ($Offline) { $arguments += '--offline' }
    & '.\gradlew.bat' @arguments
    if ($LASTEXITCODE -ne 0) { throw "Fabric build failed with exit code $LASTEXITCODE." }
    Get-ChildItem -LiteralPath (Join-Path $modRoot 'build\libs') -Filter '*.jar' |
        ForEach-Object { Write-Output "JAR: $($_.FullName)" }
} finally {
    $env:JAVA_HOME = $previousJavaHome
    $env:GRADLE_USER_HOME = $previousGradleHome
    Pop-Location
}
