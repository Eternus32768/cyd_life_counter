param([string]$Port = 'COM4', [int]$Seconds = 8)
$sp = New-Object System.IO.Ports.SerialPort($Port, 115200, 'None', 8, 'One')
$sp.ReadTimeout = 500
$sp.DtrEnable = $false
$sp.RtsEnable = $false
$sp.Open()
# ESP32 自動リセット回路: RTS->EN, DTR->IO0。DTRをfalseのままRTSをパルスして通常起動させる。
$sp.RtsEnable = $true
Start-Sleep -Milliseconds 150
$sp.RtsEnable = $false
$deadline = (Get-Date).AddSeconds($Seconds)
$buffer = ''
while ((Get-Date) -lt $deadline) {
    try { $buffer += $sp.ReadExisting() } catch {}
    Start-Sleep -Milliseconds 100
}
$sp.Close()
Write-Output '----- SERIAL BEGIN -----'
Write-Output $buffer
Write-Output '----- SERIAL END -----'
