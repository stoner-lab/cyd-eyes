# Sends every JPEG from Desktop\cyd to the board's microSD over the serial line.
# Images are cropped to 4:3 and resized to fit 320x240 before sending, because
# phone photos are far too large for the panel and the decoder only halves.
#
# Usage: powershell -ExecutionPolicy Bypass -File send_photos.ps1 [-Port COM3]

param([string]$Port = 'COM3', [string]$Source = 'C:\Users\duy\Desktop\cyd')

Add-Type -AssemblyName System.Drawing

$ErrorActionPreference = 'Stop'
$targetW = 320
$targetH = 240

$files = Get-ChildItem -LiteralPath $Source -Filter *.jpg -File | Sort-Object Name
if ($files.Count -eq 0) { throw "no .jpg in $Source" }

$serial = New-Object System.IO.Ports.SerialPort $Port, 115200, 'None', 8, 'One'
$serial.ReadTimeout = 8000
$serial.WriteTimeout = 8000
$serial.DtrEnable = $false
$serial.RtsEnable = $false
$serial.Open()
Start-Sleep -Milliseconds 300

function Resize-ToPanel([string]$path) {
    $img = [System.Drawing.Image]::FromFile($path)
    try {
        $scale = [Math]::Max($targetW / $img.Width, $targetH / $img.Height)
        $w = [int][Math]::Round($img.Width * $scale)
        $h = [int][Math]::Round($img.Height * $scale)
        $bmp = New-Object System.Drawing.Bitmap $targetW, $targetH
        $g = [System.Drawing.Graphics]::FromImage($bmp)
        try {
            $g.InterpolationMode = 'HighQualityBicubic'
            $g.DrawImage($img, [int](($targetW - $w) / 2), [int](($targetH - $h) / 2), $w, $h)
        } finally { $g.Dispose() }
        $stream = New-Object System.IO.MemoryStream
        $codec = [System.Drawing.Imaging.ImageCodecInfo]::GetImageEncoders() |
                 Where-Object { $_.MimeType -eq 'image/jpeg' }
        $params = New-Object System.Drawing.Imaging.EncoderParameters 1
        $params.Param[0] = New-Object System.Drawing.Imaging.EncoderParameter([System.Drawing.Imaging.Encoder]::Quality, [long]82)
        $bmp.Save($stream, $codec, $params)
        $bytes = $stream.ToArray()
        $stream.Dispose()
        $bmp.Dispose()
        return $bytes
    } finally { $img.Dispose() }
}

$sent = 0
foreach ($file in $files) {
    $bytes = Resize-ToPanel $file.FullName
    $name = $file.BaseName.Substring(0, [Math]::Min(40, $file.BaseName.Length)) + '.jpg'

    $header = "PUT $name $($bytes.Length)`n"
    $serial.Write($header)
    Start-Sleep -Milliseconds 150

    # 115200 baud moves about 11.5 kB/s; pace the chunks so the ESP32 buffer
    # never overruns and no byte is lost.
    $chunk = 512
    for ($i = 0; $i -lt $bytes.Length; $i += $chunk) {
        $size = [Math]::Min($chunk, $bytes.Length - $i)
        $serial.Write($bytes, $i, $size)
        Start-Sleep -Milliseconds 60
    }

    # The board also prints fps/mood lines, so read until the actual reply.
    $reply = 'timeout'
    $deadline = (Get-Date).AddSeconds(15)
    while ((Get-Date) -lt $deadline) {
        try { $line = $serial.ReadLine().Trim() } catch { break }
        if ($line -like 'OK *' -or $line -like 'ERR *') { $reply = $line; break }
    }
    "{0} -> {1} ({2} bytes)" -f $file.Name, $reply.Trim(), $bytes.Length
    $sent++
}

$serial.Close()
$serial.Dispose()
"done: $sent files"
