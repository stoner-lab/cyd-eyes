# Waits for the CH340 port to appear, opens it and dumps the ESP32 boot log.
# A brownout reset shows up as "Brownout detector was triggered" or
# "rst:0xf (RTCWDT_BROWN_OUT_RESET)" in the first lines after plugging in.
# Usage: powershell -ExecutionPolicy Bypass -File serial_probe.ps1 [-DurationSec 180]

param([int]$DurationSec = 180)

$log = 'C:\Users\duy\.cline\data\workspaces\chat\cyd-pet\serial_probe.log'
"probe start $(Get-Date -Format 'HH:mm:ss')" | Set-Content -LiteralPath $log

function Get-CydPort {
    $dev = Get-PnpDevice -PresentOnly -ErrorAction SilentlyContinue |
           Where-Object { $_.Class -eq 'Ports' -and $_.FriendlyName -match 'CH340' } |
           Select-Object -First 1
    if (-not $dev) { return $null }
    $m = [regex]::Match($dev.FriendlyName, '\((COM\d+)\)')
    if ($m.Success) { return $m.Groups[1].Value }
    return $null
}

$end = (Get-Date).AddSeconds($DurationSec)
$attempt = 0

while ((Get-Date) -lt $end) {
    $port = Get-CydPort
    if (-not $port) {
        Start-Sleep -Milliseconds 400
        continue
    }

    $attempt++
    "open $(Get-Date -Format 'HH:mm:ss') $port attempt $attempt" | Add-Content -LiteralPath $log
    $sp = $null
    try {
        $sp = New-Object System.IO.Ports.SerialPort $port, 115200, 'None', 8, 'One'
        $sp.ReadTimeout = 800
        $sp.DtrEnable = $false
        $sp.RtsEnable = $false
        $sp.Open()
        $stopAt = (Get-Date).AddSeconds(6)
        $lines = 0
        while ((Get-Date) -lt $stopAt -and $lines -lt 60) {
            try {
                $line = $sp.ReadLine()
                if ($line) {
                    $line.Trim() | Add-Content -LiteralPath $log
                    $lines++
                }
            } catch { }
        }
        "closed $(Get-Date -Format 'HH:mm:ss') lines=$lines" | Add-Content -LiteralPath $log
    } catch {
        "open failed: $($_.Exception.Message)" | Add-Content -LiteralPath $log
    } finally {
        if ($sp) {
            try { if ($sp.IsOpen) { $sp.Close() } } catch { }
            $sp.Dispose()
        }
    }
    Start-Sleep -Seconds 1
}

"probe end $(Get-Date -Format 'HH:mm:ss')" | Add-Content -LiteralPath $log
