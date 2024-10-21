$ErrorActionPreference = 'Stop'

$VERSION_DEFAULT =
$global:VERSION = "$VERSION_DEFAULT"
if (-Not "$VERSION") {
    $VERSION = Read-Host 'Enter version (ex: 3.2.0)'
}
$VERSION = ([regex]'[0-9]+\.[0-9]+\.[0-9]+').Matches("$VERSION")[0]
if (-Not "$VERSION") {
    Write-Host 'Invalid version'
    exit 1
}

Write-Host "You are installing ClipShare version $VERSION"
$confirm = Read-Host 'Proceed? [y/N]'
if ("$confirm" -ne "y") {
    Write-Host 'Aborted.'
    exit 0
}

$global:filename="clip_share_server-$VERSION-windows64.zip"
$global:url="https://github.com/thevindu-w/clip_share_server/releases/download/v$VERSION/$filename"

$global:suffix=0
$global:tmpdir="clipsharetmp"
while (Test-Path "$tmpdir") {
    if ($suffix -gt 1000) {
        Write-Host 'Creating temporary directory failed'
        exit 1
    }
    $suffix++
    $tmpdir="clipsharetmp_$suffix"
}
New-Item -Name "$tmpdir" -Type Directory | Out-Null
cd "$tmpdir"

[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
Invoke-WebRequest -Uri "$url" -OutFile "$filename" -MaximumRedirection 2

Expand-Archive -Path "$filename"
cd clip_share*
.\install-windows.bat

cd ..\..
Remove-Item -Recurse "$tmpdir"