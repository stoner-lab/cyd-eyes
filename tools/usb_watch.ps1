# Logs when the CYD CH340 chip appears and disappears, with timestamps.
# Purpose: tell a mechanical contact problem (random timing) from a power or
# thermal problem (drops after a fairly constant number of seconds).

param([int]$DurationSec = 180)

$log = 'C:\Users\duy\.cline\data\workspaces\chat\cyd-pet\usb_timeline.log'
"timeline start $(Get-Date -Format 'HH:mm:ss') duration ${DurationSec}s" | Set-Content -LiteralPath $log

$end = (Get-Date).AddSeconds($DurationSec)
$prev = $null
$upAt = $null

while ((Get-Date) -lt $end) {
    $dev = Get-PnpDevice -PresentOnly -ErrorAction SilentlyContinue |
           Where-Object { $_.InstanceId -like '*VID_1A86*' }
    $now = [bool]$dev
    $stamp = Get-Date -Format 'HH:mm:ss'

    if ($now -ne $prev) {
        if ($now) {
            $upAt = Get-Date
            "UP    $stamp  $($dev.FriendlyName)" | Add-Content -LiteralPath $log
        } else {
            $held = if ($upAt) { [int]((Get-Date) - $upAt).TotalSeconds } else { 0 }
            "DOWN  $stamp  after ${held}s" | Add-Content -LiteralPath $log
        }
        $prev = $now
    }
    Start-Sleep -Milliseconds 500
}

"timeline end $(Get-Date -Format 'HH:mm:ss')" | Add-Content -LiteralPath $log
