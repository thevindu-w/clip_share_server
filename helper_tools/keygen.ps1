$SERVER_NAME = 'clipshare_server'
$CLIENT_NAME = 'clipshare_client'
$CA_NAME = 'clipshare_ca'

$files = "ca.cer", "ca.crt"
foreach($file in $files)
{
    if (Test-Path $file) {
        Remove-Item $file
    }
}

$ErrorActionPreference = 'Stop'

$reused_ca = 0
if ((Test-Path ca.pfx) -or (Test-Path ca.enc.pfx)) {
    Write-Output 'Found previously created CA file.'
    $confirm = Read-Host -Prompt 'Use it? [Y/n]'
    if ( $confirm -ine 'n' ) {
        $reused_ca = 1
    }
}

$CA = $null
$encrypted_ca = 0
if ( "$reused_ca" -eq 1 ) {
    if (Test-Path ca.enc.pfx) {
        Write-Output ''
        Write-Output 'Please enter the password for ca.enc.pfx file.'
        $CAPasswd = Read-Host -Prompt 'Enter Password' -AsSecureString
        $CA = Import-PfxCertificate -FilePath ca.enc.pfx -Password $CAPasswd -CertStoreLocation 'Cert:\CurrentUser\My'
        $encrypted_ca = 1
    } else {
        $CA = Import-PfxCertificate -FilePath ca.pfx -CertStoreLocation 'Cert:\CurrentUser\My'
    }
    Write-Output 'Exctracted CA key and certificate.'
} else {
    Write-Output 'Generating keys and certificates ...'
    # generate CA keys
    $CA = New-SelfSignedCertificate `
        -Subject "CN=$CA_NAME" `
        -KeyLength 4096 `
        -HashAlgorithm SHA256 `
        -KeyExportPolicy Exportable `
        -CertStoreLocation 'Cert:\CurrentUser\My' `
        -NotAfter (Get-Date).AddYears(5) `
        -KeySpec Signature `
        -KeyUsage None `
        -TextExtension @('2.5.29.19={critical}{text}CA=true')

    Export-PfxCertificate -Cert $CA -FilePath ca.pfx -Password (New-Object System.Security.SecureString) | out-null
}

Export-Certificate -Cert $CA -FilePath ca.cer | out-null
certutil -encode ca.cer ca.crt | out-null
Remove-Item ca.cer

$skip_server = 0
if (Test-Path server.pfx) {
    $skip_server = 1
    Write-Output ''
    Write-Output 'Found existing server.pfx.'
    $confirm = Read-Host -Prompt 'Overwrite it to create a new server key and certificate? [y/N]'
    if ( $confirm -ieq 'y' ) {
        $skip_server = 0
    }
}
if ( "$skip_server" -eq 0 ) {
    # generate server keys
    $ServerCert = New-SelfSignedCertificate `
        -Subject "CN=$SERVER_NAME" `
        -KeyLength 2048 `
        -HashAlgorithm SHA256 `
        -KeyExportPolicy Exportable `
        -CertStoreLocation 'Cert:\CurrentUser\My' `
        -NotAfter (Get-Date).AddYears(2) `
        -Signer $CA `
        -TextExtension @('2.5.29.19={critical}{text}CA=false')

    Export-PfxCertificate -Cert $ServerCert -FilePath server.pfx -Password (New-Object System.Security.SecureString) | out-null
}

$skip_client = 0
if (Test-Path client.pfx) {
    $skip_client = 1
    Write-Output ''
    Write-Output 'Found existing client.pfx.'
    $confirm = Read-Host -Prompt 'Overwrite it to create a new client key and certificate? [y/N]'
    if ( $confirm -ieq 'y' ) {
        $skip_client = 0
    }
}
if ( "$skip_client" -eq 0 ) {
    # generate client keys
    $ClientCert = New-SelfSignedCertificate `
        -Subject "CN=$CLIENT_NAME" `
        -KeyLength 2048 `
        -HashAlgorithm SHA256 `
        -KeyExportPolicy Exportable `
        -CertStoreLocation 'Cert:\CurrentUser\My'`
        -NotAfter (Get-Date).AddYears(2) `
        -Signer $CA `
        -TextExtension @('2.5.29.19={critical}{text}CA=false')

    Write-Output 'Generated keys and certificates.'
    Write-Output ''

    Write-Output 'Exporting client keys ...'
    Write-Output 'Please enter a password that you can remember. You will need this password when importing the key on the client.'
    Write-Output ''
    $ClientPasswd = Read-Host -Prompt 'Enter Export Password' -AsSecureString
    Export-PfxCertificate -Cert $ClientCert -FilePath client.pfx -Password $ClientPasswd | out-null
    Write-Output ''
    Write-Output 'Exported client keys.'
}

$ErrorActionPreference = 'SilentlyContinue'
Write-Output 'Cleaning up ...'
Get-ChildItem Cert:\CurrentUser\My | Where-Object { $_.Subject -eq "CN=$SERVER_NAME"} | ForEach-Object {Remove-Item $_.pspath }
Get-ChildItem Cert:\CurrentUser\My | Where-Object { $_.Subject -eq "CN=$CLIENT_NAME"} | ForEach-Object {Remove-Item $_.pspath }
Get-ChildItem Cert:\CurrentUser\My | Where-Object { $_.Subject -eq "CN=$CA_NAME"} | ForEach-Object {Remove-Item $_.pspath }
Get-ChildItem Cert:\CurrentUser\CA | Where-Object { $_.Subject -eq "CN=$CA_NAME"} | ForEach-Object {Remove-Item $_.pspath }
$ErrorActionPreference = 'Stop'

Write-Output 'Done.'

if ( "$reused_ca" -ne 1 ) {
    Write-Output ''
    $confirm = Read-Host -Prompt 'Do you plan to create more keys in the future and like to encrypt the CA key file to secure it? [y/N]'
    if ( $confirm -ieq 'y' ) {
        Write-Output 'Encrypting CA key ...'
        Write-Output ''
        Write-Output 'Please enter a password that you can remember. You will need this password when using the key to sign more certificates in the future.'
        Write-Output ''
        $CAPasswd = Read-Host -Prompt 'Enter Password' -AsSecureString
        Export-PfxCertificate -Cert $CA -FilePath ca.enc.pfx -Password $CAPasswd | out-null
        Remove-Item ca.pfx
        Write-Output 'Stored the encrypted CA key in "ca.enc.pfx"'
        $encrypted_ca = 1
    } else {
        Write-Output ''
        Write-Output 'The CA key is stored in "ca.pfx" without encrypting. If you do not plan to create more keys, please delete that file for security.'
    }
}

Write-Output ''
Write-Output '> server.pfx - The TLS key and certificate file of the server. Keep this file securely.'
Write-Output '> ca.crt     - The TLS certificate file of the CA. You need this file on both the server and the client devices.'
Write-Output ''
Write-Output '> client.pfx - The TLS key and certificate store file for the client. Move this to the client device.'
Write-Output ''
Write-Output "# the server's name is $SERVER_NAME"
Write-Output "# the client's name is $CLIENT_NAME"
if ( "$encrypted_ca" -ne 1 ) {
    Write-Output ''
    Write-Output 'Note: If you do not plan to create more keys in the future, you may safely delete the "ca.pfx" file. Otherwise, store the "ca.pfx" file securely.'
}
