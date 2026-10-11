$cli = 'C:\Program Files\Arduino CLI\arduino-cli.exe'
$dir = 'C:\Users\ishid\AppData\Local\Temp\cyd_build'
$sk  = '\\wsl.localhost\Ubuntu\home\ishida\cyd_life_counter_md'
for ($i = 1; $i -le 4; $i++) {
    Write-Output "=== attempt $i ==="
    & $cli upload --fqbn 'esp32:esp32:esp32:UploadSpeed=230400' --input-dir $dir -p COM4 $sk
    if ($LASTEXITCODE -eq 0) { Write-Output '=== UPLOAD OK ==='; exit 0 }
    Start-Sleep -Seconds 2
}
Write-Output '=== UPLOAD FAILED ==='
exit 1
