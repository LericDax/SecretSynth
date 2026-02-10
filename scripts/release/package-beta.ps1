[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('windows', 'macos')]
    [string]$Platform,

    [Parameter(Mandatory = $true)]
    [string]$Installer,

    [Parameter(Mandatory = $true)]
    [string]$Vst3,

    [Parameter(Mandatory = $false)]
    [string]$Au,

    [Parameter(Mandatory = $false)]
    [string]$OutDir = 'dist',

    [Parameter(Mandatory = $false)]
    [string]$BuildMeta
)

$ErrorActionPreference = 'Stop'

$ScriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Resolve-Path (Join-Path $ScriptRoot '..\..')
$PluginIdentityFile = Join-Path $RepoRoot 'cmake\PluginIdentity.cmake'
$StagingRoot = Join-Path $RepoRoot 'build\release-staging'

if (-not (Test-Path $PluginIdentityFile)) {
    throw "Plugin identity file not found: $PluginIdentityFile"
}

$identityContent = Get-Content -Raw $PluginIdentityFile
$versionMatch = [regex]::Match($identityContent, 'set\(SECRET_SYNTH_VERSION\s+"(?<version>\d+\.\d+\.\d+)"\)')
if (-not $versionMatch.Success) {
    throw "Could not parse SECRET_SYNTH_VERSION in $PluginIdentityFile"
}

$versionTag = "v$($versionMatch.Groups['version'].Value)"
if ($BuildMeta) {
    $versionTag = "$versionTag+$BuildMeta"
}

if (-not (Test-Path $Installer)) { throw "Installer not found: $Installer" }
if (-not (Test-Path $Vst3)) { throw "VST3 bundle not found: $Vst3" }
if ($Au -and -not (Test-Path $Au)) { throw "AU component not found: $Au" }

$installerExt = if ($Platform -eq 'windows') { 'msi' } else { 'pkg' }
$archiveBaseName = "SecretSynth-beta-$versionTag-$Platform"
$resolvedOutDir = Join-Path $RepoRoot $OutDir
$stageDir = Join-Path $StagingRoot $archiveBaseName
$archivePath = Join-Path $resolvedOutDir "$archiveBaseName.zip"

New-Item -ItemType Directory -Force -Path $resolvedOutDir | Out-Null
New-Item -ItemType Directory -Force -Path $StagingRoot | Out-Null
if (Test-Path $stageDir) {
    Remove-Item -Recurse -Force $stageDir
}
New-Item -ItemType Directory -Force -Path $stageDir | Out-Null

Copy-Item -Path $Installer -Destination (Join-Path $stageDir "SecretSynth-installer.$installerExt") -Recurse -Force
Copy-Item -Path $Vst3 -Destination (Join-Path $stageDir 'SecretSynth.vst3') -Recurse -Force
if ($Au) {
    Copy-Item -Path $Au -Destination (Join-Path $stageDir 'SecretSynth.component') -Recurse -Force
}

if (Test-Path $archivePath) {
    Remove-Item -Force $archivePath
}

Compress-Archive -Path $stageDir -DestinationPath $archivePath -CompressionLevel Optimal
Write-Host "Created $archivePath"
