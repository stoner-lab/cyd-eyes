# Continuous serial logger for touch calibration.
# Appends every line from the CH340 port to touch_calib.log while it runs.
# Usage: powershell -ExecutionPolicy Bypass -File serial_log.ps1 [-DurationSec 150]

param([int]$DurationSec = 150)

$log = 'C:\Users\duy\.cline\data\workspaces\chat\cyd-pet\touch_calib.log'
"log start $(Get-Date -Format 'HH:mm:ss')" | Set-Content -LiteralPath $log

$end = (Get-Date).AddSeconds($DurationSec)
$sp = $null

while ((Get-Date) -lt $end) {
    $dev = Get-PnpDevice -PresentOnly -ErrorAction SilentlyContinue |
           Where-Object { $_.InstanceId -like '*VID_1A86*' } |
           Select-Object -First 1
    if (-not $dev) { Start-Sleep -Milliseconds 500; continue }

    $m = [regex]::Match($dev.FriendlyName, '\((COM\d+)\)')
    if (-not $m.Success) { Start-Sleep -Milliseconds 500; continue }

    try {
        if (-not $sp) {
            $sp = New-Object System.IO.Ports.SerialPort $m.Groups[1].Value, 115200, 'None', 8, 'One'
            $sp.ReadTimeout = 700
            $sp.DtrEnable = $false
            $sp.RtsEnable = $false
            $sp.Open()
            "port open $(Get-Date -Format 'HH:mm:ss') $($m.Groups[1].Value)" | Add-Content -LiteralPath $log
        }
        try {
            $line = $sp.ReadLine()
            if ($line) {
                "$(Get-Date -Format 'HH:mm:ss') $($line.Trim())" | Add-Content -LiteralPath $log
            }
        } catch [System.TimeoutException] {
            # Silence is normal; keep the port open so the board is not reset.
        }
    } catch {
        if ($sp) {
            try { if ($sp.IsOpen) { $sp.Close() } } catch { }
            $sp = $null
        }
        Start-Sleep -Milliseconds 300
    }
}

if ($sp) {
    try { if ($sp.IsOpen) { $sp.Close() } } catch { }
    $sp.Dispose()
}
"log end $(Get-Date -Format 'HH:mm:ss')" | Add-Content -LiteralPath $log
