# Retries the upload whenever the CH340 port shows up, because the link on this
# board drops within seconds. Every attempt is logged with a timestamp, so the
# failure pattern is visible afterwards.
# Usage: powershell -ExecutionPolicy Bypass -File flash_retry.ps1 [-Env cyd_probe] [-Attempts 10]

param([string]$Env = 'cyd_probe', [int]$Attempts = 10)

$proj = 'C:\Users\duy\.cline\data\workspaces\chat\cyd-pet'
$log  = Join-Path $proj 'flash_retry.log'
$pio  = 'C:\Python314\python.exe'

"retry start $(Get-Date -Format 'HH:mm:ss') env=$Env attempts=$Attempts" | Set-Content -LiteralPath $log

function Get-CydPort {
    $dev = Get-PnpDevice -PresentOnly -ErrorAction SilentlyContinue |
           Where-Object { $_.InstanceId -like '*VID_1A86*' } |
           Select-Object -First 1
    if ($dev) { return $dev.FriendlyName }
    return $null
}

for ($i = 1; $i -le $Attempts; $i++) {
    $found = $null
    $waitUntil = (Get-Date).AddSeconds(45)
    while ((Get-Date) -lt $waitUntil) {
        $found = Get-CydPort
        if ($found) { break }
        Start-Sleep -Milliseconds 400
    }
    if (-not $found) {
        "attempt $i skipped: port never appeared" | Add-Content -LiteralPath $log
        continue
    }

    # Give the port time to finish enumerating before esptool grabs it.
    Start-Sleep -Seconds 2
    "attempt $i $(Get-Date -Format 'HH:mm:ss') on $found" | Add-Content -LiteralPath $log

    & $pio -m platformio run -e $Env -t upload --project-dir $proj *>&1 |
        Add-Content -LiteralPath $log

    if ($LASTEXITCODE -eq 0) {
        "attempt $i SUCCESS $(Get-Date -Format 'HH:mm:ss')" | Add-Content -LiteralPath $log
        break
    }
    "attempt $i failed code $LASTEXITCODE $(Get-Date -Format 'HH:mm:ss')" | Add-Content -LiteralPath $log
    Start-Sleep -Seconds 3
}

"retry end $(Get-Date -Format 'HH:mm:ss')" | Add-Content -LiteralPath $log
