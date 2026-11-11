# Waits for the CYD CH340 serial chip, then flashes the probe build.
# Usage: powershell -ExecutionPolicy Bypass -File flash_when_ready.ps1 [-TimeoutSec 900]
# The board drops off USB while idle, so replugging has to be followed by an
# upload inside the same window.

param([int]$TimeoutSec = 900, [string]$Env = 'cyd_probe')

$proj = 'C:\Users\duy\.cline\data\workspaces\chat\cyd-pet'
$log  = Join-Path $proj 'flash_watch.log'
$pio  = 'C:\Python314\python.exe'

"watch start $(Get-Date -Format 'HH:mm:ss') env=$Env" | Set-Content -LiteralPath $log
$deadline = (Get-Date).AddSeconds($TimeoutSec)
$found = $false

while ((Get-Date) -lt $deadline) {
    $dev = Get-PnpDevice -PresentOnly -ErrorAction SilentlyContinue |
           Where-Object { $_.InstanceId -like '*VID_1A86*' }
    if ($dev) {
        "found $(Get-Date -Format 'HH:mm:ss'): $($dev.FriendlyName)" | Add-Content -LiteralPath $log
        $found = $true
        break
    }
    Start-Sleep -Milliseconds 800
}

if (-not $found) {
    "timeout: no CH340 seen in $TimeoutSec s" | Add-Content -LiteralPath $log
    exit 1
}

# The port needs a moment to finish enumerating before esptool opens it.
Start-Sleep -Seconds 3
"upload start $(Get-Date -Format 'HH:mm:ss')" | Add-Content -LiteralPath $log
& $pio -m platformio run -e $Env -t upload --project-dir $proj *>&1 | Add-Content -LiteralPath $log
"upload exit code $LASTEXITCODE $(Get-Date -Format 'HH:mm:ss')" | Add-Content -LiteralPath $log
