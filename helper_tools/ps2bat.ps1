$code = Get-Command .\install-online-windows.ps1 | Select -ExpandProperty ScriptBlock

$encoded = [convert]::ToBase64String([Text.Encoding]::Unicode.GetBytes($code))
Write-Output '@ECHO OFF' | Out-File -Encoding ASCII install.bat
Write-Output "powershell.exe -ExecutionPolicy bypass -Encoded $encoded" | Out-File -Append -Encoding ASCII install.bat