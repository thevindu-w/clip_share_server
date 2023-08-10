Add-Type -AssemblyName System.Windows.Forms
$i2 = [System.Drawing.Image]::Fromfile("$PWD\image.png")
[Windows.Forms.Clipboard]::SetImage($i2)