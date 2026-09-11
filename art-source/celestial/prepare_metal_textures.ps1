param([Parameter(Mandatory)][string]$GoldSource, [Parameter(Mandatory)][string]$SteelSource)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$masterDirectory = Join-Path $PSScriptRoot 'textures/masters/golden-metal'
$null = New-Item -ItemType Directory -Force -Path $masterDirectory

function Export-GameJpeg([string]$source, [string]$target) {
    $image = [Drawing.Image]::FromFile($source)
    $bitmap = New-Object Drawing.Bitmap(512, 512, [Drawing.Imaging.PixelFormat]::Format24bppRgb)
    $graphics = [Drawing.Graphics]::FromImage($bitmap)
    $parameters = New-Object Drawing.Imaging.EncoderParameters(1)
    try {
        $graphics.CompositingQuality = [Drawing.Drawing2D.CompositingQuality]::HighQuality
        $graphics.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
        $graphics.PixelOffsetMode = [Drawing.Drawing2D.PixelOffsetMode]::HighQuality
        $graphics.DrawImage($image, 0, 0, 512, 512)
        $parameters.Param[0] = New-Object Drawing.Imaging.EncoderParameter([Drawing.Imaging.Encoder]::Quality, [long]95)
        $encoder = [Drawing.Imaging.ImageCodecInfo]::GetImageEncoders() | Where-Object MimeType -eq 'image/jpeg'
        $bitmap.Save($target, $encoder, $parameters)
    }
    finally { $parameters.Dispose(); $graphics.Dispose(); $bitmap.Dispose(); $image.Dispose() }
}

$sources = @{ Gold = $GoldSource; Ivory = $SteelSource }
foreach ($name in $sources.Keys) {
    $sourcePath = (Resolve-Path -LiteralPath $sources[$name]).Path
    $masterPath = Join-Path $masterDirectory "$name.png"
    if (Test-Path -LiteralPath $masterPath) {
        if ((Get-FileHash -LiteralPath $masterPath).Hash -ne (Get-FileHash -LiteralPath $sourcePath).Hash) {
            throw "A different master already exists: $masterPath"
        }
    }
    else { Copy-Item -LiteralPath $sourcePath -Destination $masterPath }
    Export-GameJpeg $masterPath (Join-Path $PSScriptRoot "textures/Celestial_$name.jpg")
}
