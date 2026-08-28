param(
    [string]$ClassicSourceUrl = "https://web.luxview.cloud/games/mu-online-v2.jpg",
    [string]$AlternateSourceUrl = "https://raw.githubusercontent.com/JohnPitter/luxview-cloud/main/luxview-launcher/frontend/src/assets/games/mu.jpg"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

$repoRoot = Split-Path -Parent $PSScriptRoot
$dataDir = Join-Path $repoRoot "src\bin\Data\Interface"
$previewDir = Join-Path $repoRoot "docs\assets"
$classSource = Join-Path $env:TEMP "luxview-title-classic.img"
$alternateSource = Join-Path $env:TEMP "luxview-title-alternate.img"

New-Item -ItemType Directory -Force -Path $previewDir | Out-Null
Invoke-WebRequest -Uri $ClassicSourceUrl -OutFile $classSource -UseBasicParsing
Invoke-WebRequest -Uri $AlternateSourceUrl -OutFile $alternateSource -UseBasicParsing

function Save-Jpeg {
    param([System.Drawing.Image]$Image, [string]$Path, [long]$Quality = 94)

    $codec = [System.Drawing.Imaging.ImageCodecInfo]::GetImageEncoders() |
        Where-Object { $_.MimeType -eq "image/jpeg" }
    $parameters = New-Object System.Drawing.Imaging.EncoderParameters(1)
    $parameters.Param[0] = New-Object System.Drawing.Imaging.EncoderParameter(
        [System.Drawing.Imaging.Encoder]::Quality, $Quality)
    try {
        $Image.Save($Path, $codec, $parameters)
    }
    finally {
        $parameters.Dispose()
    }
}

function Write-Ozj {
    param([string]$JpegPath, [string]$DestinationPath)

    $existing = [System.IO.File]::ReadAllBytes($DestinationPath)
    if ($existing.Length -lt 24) {
        throw "OZJ header missing: $DestinationPath"
    }
    $jpeg = [System.IO.File]::ReadAllBytes($JpegPath)
    $output = New-Object byte[] (24 + $jpeg.Length)
    [Array]::Copy($existing, 0, $output, 0, 24)
    [Array]::Copy($jpeg, 0, $output, 24, $jpeg.Length)
    [System.IO.File]::WriteAllBytes($DestinationPath, $output)
}

function Draw-OutlinedText {
    param(
        [System.Drawing.Graphics]$Graphics,
        [string]$Text,
        [System.Drawing.Font]$Font,
        [System.Drawing.PointF]$Position,
        [System.Drawing.Color]$Color,
        [float]$OutlineWidth
    )

    $path = New-Object System.Drawing.Drawing2D.GraphicsPath
    $emSize = $Graphics.DpiY * $Font.Size / 72.0
    $path.AddString($Text, $Font.FontFamily, [int]$Font.Style, $emSize, $Position,
        [System.Drawing.StringFormat]::GenericDefault)
    $outline = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(210, 4, 8, 16), $OutlineWidth)
    $fill = New-Object System.Drawing.SolidBrush($Color)
    try {
        $Graphics.DrawPath($outline, $path)
        $Graphics.FillPath($fill, $path)
    }
    finally {
        $fill.Dispose()
        $outline.Dispose()
        $path.Dispose()
    }
}

function Add-LuxviewBrand {
    param([System.Drawing.Graphics]$Graphics, [System.Drawing.Color]$Accent)

    $sky = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(235, 185, 221, 255))
    $sun = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 255, 212, 71))
    $leaf = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 8, 121, 88))
    $wave = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::White)
    $brandFont = New-Object System.Drawing.Font("Segoe UI", 34, [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)
    $gamesFont = New-Object System.Drawing.Font("Segoe UI", 13, [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)
    try {
        $Graphics.FillRectangle($sky, 52, 38, 44, 39)
        $Graphics.FillEllipse($sun, 59, 44, 17, 17)
        $Graphics.FillEllipse($leaf, 77, 48, 15, 22)
        $Graphics.FillEllipse($wave, 49, 63, 51, 23)
        Draw-OutlinedText $Graphics "Luxview" $brandFont (New-Object System.Drawing.PointF(108, 38)) ([System.Drawing.Color]::White) 1.8
        Draw-OutlinedText $Graphics "G A M E S" $gamesFont (New-Object System.Drawing.PointF(111, 77)) $Accent 1.1
    }
    finally {
        $gamesFont.Dispose()
        $brandFont.Dispose()
        $wave.Dispose()
        $leaf.Dispose()
        $sun.Dispose()
        $sky.Dispose()
    }
}

function Merge-PanelSeams {
    param([System.Drawing.Bitmap]$Canvas)

    foreach ($x in @(511, 1023)) {
        for ($y = 0; $y -lt $Canvas.Height; $y++) {
            $left = $Canvas.GetPixel($x, $y)
            $right = $Canvas.GetPixel($x + 1, $y)
            $merged = [System.Drawing.Color]::FromArgb(
                [int](($left.R + $right.R) / 2),
                [int](($left.G + $right.G) / 2),
                [int](($left.B + $right.B) / 2))
            $Canvas.SetPixel($x, $y, $merged)
            $Canvas.SetPixel($x + 1, $y, $merged)
        }
    }
    for ($x = 0; $x -lt $Canvas.Width; $x++) {
        $top = $Canvas.GetPixel($x, 511)
        $bottom = $Canvas.GetPixel($x, 512)
        $merged = [System.Drawing.Color]::FromArgb(
            [int](($top.R + $bottom.R) / 2),
            [int](($top.G + $bottom.G) / 2),
            [int](($top.B + $bottom.B) / 2))
        $Canvas.SetPixel($x, 511, $merged)
        $Canvas.SetPixel($x, 512, $merged)
    }
}

function Write-Theme {
    param(
        [string]$SourcePath,
        [string]$Prefix,
        [string]$PreviewPath,
        [System.Drawing.Color]$Accent
    )

    $source = [System.Drawing.Image]::FromFile($SourcePath)
    $canvas = New-Object System.Drawing.Bitmap(1280, 735, [System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
    $graphics = [System.Drawing.Graphics]::FromImage($canvas)
    try {
        $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
        $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
        $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
        $graphics.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit

        $graphics.DrawImage($source,
            (New-Object System.Drawing.Rectangle(0, 0, 1280, 735)),
            (New-Object System.Drawing.Rectangle(0, 71, 1536, 882)),
            [System.Drawing.GraphicsUnit]::Pixel)

        $topGradient = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
            (New-Object System.Drawing.Point(0, 0)), (New-Object System.Drawing.Point(0, 180)),
            [System.Drawing.Color]::FromArgb(210, 5, 10, 20), [System.Drawing.Color]::FromArgb(0, 5, 10, 20))
        $bottomGradient = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
            (New-Object System.Drawing.Point(0, 500)), (New-Object System.Drawing.Point(0, 735)),
            [System.Drawing.Color]::FromArgb(0, 5, 10, 18), [System.Drawing.Color]::FromArgb(230, 5, 10, 18))
        try {
            $graphics.FillRectangle($topGradient, 0, 0, 1280, 180)
            $graphics.FillRectangle($bottomGradient, 0, 500, 1280, 235)
        }
        finally {
            $bottomGradient.Dispose()
            $topGradient.Dispose()
        }

        Add-LuxviewBrand $graphics $Accent
        $mainFont = New-Object System.Drawing.Font("Segoe UI", 48, [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)
        $subFont = New-Object System.Drawing.Font("Segoe UI", 17, [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)
        try {
            # Keep both lines inside the center panels; no text crosses a texture seam.
            Draw-OutlinedText $graphics "MU ONLINE" $mainFont (New-Object System.Drawing.PointF(565, 600)) ([System.Drawing.Color]::White) 2.8
            Draw-OutlinedText $graphics "O CONTINENTE DA NOSTALGIA" $subFont (New-Object System.Drawing.PointF(570, 662)) $Accent 1.6
        }
        finally {
            $subFont.Dispose()
            $mainFont.Dispose()
        }

        Merge-PanelSeams $canvas
        Save-Jpeg $canvas $PreviewPath 94

        $panels = @(
            @{ Suffix = "01"; X = 0; Y = 0; Width = 512; Height = 512 },
            @{ Suffix = "02"; X = 512; Y = 0; Width = 512; Height = 512 },
            @{ Suffix = "03"; X = 1024; Y = 0; Width = 256; Height = 512 },
            @{ Suffix = "04"; X = 0; Y = 512; Width = 512; Height = 223 },
            @{ Suffix = "05"; X = 512; Y = 512; Width = 512; Height = 223 },
            @{ Suffix = "06"; X = 1024; Y = 512; Width = 256; Height = 223 }
        )
        foreach ($panel in $panels) {
            $bitmap = New-Object System.Drawing.Bitmap($panel.Width, $panel.Height, [System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
            $panelGraphics = [System.Drawing.Graphics]::FromImage($bitmap)
            $name = $Prefix + $panel.Suffix + ".OZJ"
            $jpegPath = Join-Path $env:TEMP ($name + ".jpg")
            try {
                $panelGraphics.DrawImage($canvas,
                    (New-Object System.Drawing.Rectangle(0, 0, $panel.Width, $panel.Height)),
                    (New-Object System.Drawing.Rectangle($panel.X, $panel.Y, $panel.Width, $panel.Height)),
                    [System.Drawing.GraphicsUnit]::Pixel)
                Save-Jpeg $bitmap $jpegPath 94
                Write-Ozj $jpegPath (Join-Path $dataDir $name)
            }
            finally {
                Remove-Item -Force -ErrorAction SilentlyContinue $jpegPath
                $panelGraphics.Dispose()
                $bitmap.Dispose()
            }
        }
    }
    finally {
        $graphics.Dispose()
        $canvas.Dispose()
        $source.Dispose()
    }
}

try {
    Write-Theme $classSource "lo_back_im" (Join-Path $previewDir "luxview-title-loading-preview.jpg") ([System.Drawing.Color]::FromArgb(255, 255, 212, 71))
    Write-Theme $alternateSource "lo_back_s5_im" (Join-Path $previewDir "luxview-title-loading-alternate-preview.jpg") ([System.Drawing.Color]::FromArgb(255, 185, 221, 255))
}
finally {
    Remove-Item -Force -ErrorAction SilentlyContinue $classSource, $alternateSource
}

Write-Output "Generated two aligned Luxview title-loading themes."
