$SERVER_NAME = 'clipshare_server'
$CLIENT_NAME = 'clipshare_client'
$CA_NAME = 'clipshare_ca'

$files =  "ca.pfx", "ca.cer", "ca.crt", "server.pfx", "client.pfx"
foreach($file in $files)
{
    if (Test-Path $file) {
        Remove-Item $file
    }
}

$ErrorActionPreference = 'Stop'

Write-Host 'Generating keys and certificates ...'

# generate CA keys
$CA = New-SelfSignedCertificate `
    -Subject "CN=$CA_NAME" `
    -KeyExportPolicy Exportable `
    -KeySpec Signature `
    -KeyLength 4096 `
    -CertStoreLocation 'Cert:\CurrentUser\My' `
    -HashAlgorithm SHA256 `
    -NotAfter (Get-Date).AddYears(5) `
    -TextExtension @('2.5.29.19={critical}{text}CA:TRUE')

Export-PfxCertificate -Cert $CA -FilePath ca.pfx -Password (New-Object System.Security.SecureString) | out-null
Export-Certificate -Cert $CA -FilePath ca.cer | out-null
certutil -encode ca.cer ca.crt | out-null
Remove-Item ca.cer

# generate server keys
$ServerCert = New-SelfSignedCertificate `
    -Subject "CN=$SERVER_NAME" `
    -Signer $CA `
    -CertStoreLocation 'Cert:\CurrentUser\My' `
    -KeyLength 2048 `
    -KeyExportPolicy Exportable `
    -TextExtension @('2.5.29.19={text}CA:FALSE') `
    -NotAfter (Get-Date).AddYears(2) `
    -HashAlgorithm SHA256

Export-PfxCertificate -Cert $ServerCert -FilePath server.pfx -Password (New-Object System.Security.SecureString) | out-null

# generate client keys
$ClientCert = New-SelfSignedCertificate `
    -Subject "CN=$CLIENT_NAME" `
    -Signer $CA `
    -CertStoreLocation 'Cert:\CurrentUser\My'`
    -KeyLength 2048 `
    -KeyExportPolicy Exportable `
    -TextExtension @('2.5.29.19={text}CA:FALSE') `
    -NotAfter (Get-Date).AddYears(2) `
    -HashAlgorithm SHA256

Write-Host 'Generated keys and certificates.'
Write-Host

Write-Host 'Exporting client keys ...'
Write-Host 'Please enter a password that you can remember. You will need this password when importing the key on the client.'
Write-Host
$ClientPasswd = Read-Host -Prompt 'Enter Export Password: ' -AsSecureString
Export-PfxCertificate -Cert $ClientCert -FilePath client.pfx -Password $ClientPasswd | out-null
Write-Host
echo 'Exported client keys.'

echo 'Cleaning up ...'
Get-ChildItem Cert:\CurrentUser\My | Where { $_.Subject -eq "CN=$SERVER_NAME"} | foreach {Remove-Item $_.pspath }
Get-ChildItem Cert:\CurrentUser\My | Where { $_.Subject -eq "CN=$CLIENT_NAME"} | foreach {Remove-Item $_.pspath }
Get-ChildItem Cert:\CurrentUser\My | Where { $_.Subject -eq "CN=$CA_NAME"} | foreach {Remove-Item $_.pspath }

Write-Host 'Done.'
Write-Host
Write-Host '> server.pfx - The TLS key and certificate file of the server. Keep this file securely.'
Write-Host '> ca.crt     - The TLS certificate file of the CA. You need this file on both the server and the client devices.'
Write-Host
Write-Host '> client.pfx - The TLS key and certificate store file for the client. Move this to the client device.'
Write-Host
Write-Host "# the server's name is $SERVER_NAME"
Write-Host "# the client's name is $CLIENT_NAME"
Write-Host
Write-Host 'Note: If you do not plan to create more keys in the future, you may safely delete the "ca.pfx" file. Otherwise, store the "ca.pfx" file securely.'