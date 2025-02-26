$SERVER_NAME = 'clipshare_server'
$CLIENT_NAME = 'clipshare_client'
$CA_NAME = 'clipshare_ca'

$files = "ca.pfx", "ca.cer", "ca.crt", "server.pfx", "client.pfx"
foreach($file in $files)
{
    if (Test-Path $file) {
        Remove-Item $file
    }
}

$ErrorActionPreference = 'Stop'

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
Export-Certificate -Cert $CA -FilePath ca.cer | out-null
certutil -encode ca.cer ca.crt | out-null
Remove-Item ca.cer

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
$ClientPasswd = Read-Host -Prompt 'Enter Export Password: ' -AsSecureString
Export-PfxCertificate -Cert $ClientCert -FilePath client.pfx -Password $ClientPasswd | out-null
Write-Output ''
Write-Output 'Exported client keys.'

Write-Output 'Cleaning up ...'
Get-ChildItem Cert:\CurrentUser\My | Where-Object { $_.Subject -eq "CN=$SERVER_NAME"} | ForEach-Object {Remove-Item $_.pspath }
Get-ChildItem Cert:\CurrentUser\My | Where-Object { $_.Subject -eq "CN=$CLIENT_NAME"} | ForEach-Object {Remove-Item $_.pspath }
Get-ChildItem Cert:\CurrentUser\My | Where-Object { $_.Subject -eq "CN=$CA_NAME"} | ForEach-Object {Remove-Item $_.pspath }

Write-Output 'Done.'
Write-Output ''
Write-Output '> server.pfx - The TLS key and certificate file of the server. Keep this file securely.'
Write-Output '> ca.crt     - The TLS certificate file of the CA. You need this file on both the server and the client devices.'
Write-Output ''
Write-Output '> client.pfx - The TLS key and certificate store file for the client. Move this to the client device.'
Write-Output ''
Write-Output "# the server's name is $SERVER_NAME"
Write-Output "# the client's name is $CLIENT_NAME"
Write-Output ''
Write-Output 'Note: If you do not plan to create more keys in the future, you may safely delete the "ca.pfx" file. Otherwise, store the "ca.pfx" file securely.'