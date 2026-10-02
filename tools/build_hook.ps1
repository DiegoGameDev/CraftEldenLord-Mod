[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Release',
    [switch]$SkipTests
)
$ErrorActionPreference = 'Stop'
function Invoke-BuildTool {
    param([string]$Executable, [string[]]$ToolArguments)
    if ($PSVersionTable.PSVersion.Major -ge 7) {
        # Rebuild the child environment to collapse PATH/Path duplicates from some hosts.
        $start = [System.Diagnostics.ProcessStartInfo]::new()
        $start.FileName = $Executable
        $start.UseShellExecute = $false
        foreach ($argument in $ToolArguments) { $start.ArgumentList.Add($argument) }
        $environment = [Environment]::GetEnvironmentVariables()
        $start.Environment.Clear()
        foreach ($key in $environment.Keys) { $start.Environment[$key] = $environment[$key] }
        $process = [System.Diagnostics.Process]::Start($start)
        try {
            $process.WaitForExit()
            if ($process.ExitCode -ne 0) { throw "$Executable failed with exit code $($process.ExitCode)." }
        } finally {
            $process.Dispose()
        }
    } else {
        & $Executable @ToolArguments
        if ($LASTEXITCODE -ne 0) { throw "$Executable failed with exit code $LASTEXITCODE." }
    }
}
$projectRoot = Split-Path -Parent $PSScriptRoot
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswhere -PathType Leaf)) {
    throw 'Visual Studio Installer not found. Install the Desktop development with C++ workload.'
}
$installation = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -format json | ConvertFrom-Json
if (-not $installation) { throw 'MSVC x64 tools not found.' }
$installation = @($installation)[0]
$bundledCmake = Join-Path $installation.installationPath 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
if (Test-Path -LiteralPath $bundledCmake -PathType Leaf) {
    $cmake = $bundledCmake
} else {
    $cmake = (Get-Command cmake -ErrorAction Stop).Source
}
$major = ([version]$installation.installationVersion).Major
$generator = switch ($major) {
    17 { 'Visual Studio 17 2022' }
    18 { 'Visual Studio 18 2026' }
    default { throw "Unsupported Visual Studio version $major; configure CMake manually." }
}
$source = Join-Path $projectRoot 'elden-ring-hooker'
$build = Join-Path $source 'build'
Invoke-BuildTool $cmake @('-S', $source, '-B', $build, '-G', $generator, '-A', 'x64',
    "-DCMAKE_GENERATOR_INSTANCE=$($installation.installationPath)", '-DBUILD_TESTING=ON')
Invoke-BuildTool $cmake @('--build', $build, '--config', $Configuration, '--parallel', '1')
if (-not $SkipTests) {
    $ctest = Join-Path (Split-Path -Parent $cmake) 'ctest.exe'
    Invoke-BuildTool $ctest @('--test-dir', $build, '-C', $Configuration, '--output-on-failure')
}
Write-Output "DLL: $(Join-Path $build "$Configuration\MinecraftEldenBridge.dll")"
