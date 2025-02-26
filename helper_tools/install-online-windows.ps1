$ErrorActionPreference = 'Stop'

if (([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Error 'This installer must not be run as administrator.'
    Write-Error 'Please run without administrator rights.'
    Write-Error 'Installation aborted.'
    exit 1
}

$VERSION_DEFAULT =
$VERSION = "$VERSION_DEFAULT"
if (-Not "$VERSION") {
    $VERSION = Read-Host 'Enter version (ex: 3.2.0)'
}
$VERSION = ([regex]'[0-9]+\.[0-9]+\.[0-9]+').Matches("$VERSION")[0]
if (-Not "$VERSION") {
    Write-Error 'Invalid version'
    exit 1
}

Write-Output "You are installing ClipShare version $VERSION"
$confirm = Read-Host 'Proceed? [y/N]'
if ("$confirm" -ne "y") {
    Write-Error 'Aborted.'
    exit 0
}

if ([System.Environment]::Is64BitOperatingSystem) {
    $ARCH = 'x86_64'
} else {
    $ARCH = 'x86'
}

$filename="clip_share_server-$VERSION-windows-$ARCH.zip"
$url="https://github.com/thevindu-w/clip_share_server/releases/download/v$VERSION/$filename"

$suffix=0
$tmpdir="clipsharetmp"
while (Test-Path "$tmpdir") {
    if ($suffix -gt 1000) {
        Write-Error 'Creating temporary directory failed'
        exit 1
    }
    $suffix++
    $tmpdir="clipsharetmp_$suffix"
}
New-Item -Name "$tmpdir" -Type Directory | Out-Null
Set-Location "$tmpdir"

[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
Invoke-WebRequest -Uri "$url" -OutFile "$filename" -MaximumRedirection 2

Expand-Archive -Path "$filename"
Set-Location clip_share*
.\install-windows.bat

Set-Location ..\..
Remove-Item -Recurse "$tmpdir"