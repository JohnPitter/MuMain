param(
    [string]$SourceUrl = "https://web.luxview.cloud/games/mu-online-v2.jpg"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

$repoRoot = Split-Path -Parent $PSScriptRoot
$dataDir = Join-Path $repoRoot "src\bin\Data\Interface"
$previewDir = Join-Path $repoRoot "docs\assets"
$sourcePath = Join-Path $env:TEMP "luxview-mu-loading-source.jpg"
$previewPath = Join-Path $previewDir "luxview-loading-preview.jpg"

New-Item -ItemType Directory -Force -Path $previewDir | Out-Null
Invoke-WebRequest -Uri $SourceUrl -OutFile $sourcePath -UseBasicParsing

function Save-Jpeg {
    param(
        [System.Drawing.Image]$Image,
        [string]$Path,
        [long]$Quality = 92
    )

    $codec = [System.Drawing.Imaging.ImageCodecInfo]::GetImageEncoders() |
        Where-Object { $_.MimeType -eq "image/jpeg" }
    $parameters = New-Object System.Drawing.Imaging.EncoderParameters(1)
    $parameters.Param[0] = New-Object System.Drawing.Imaging.EncoderParameter(
        [System.Drawing.Imaging.Encoder]::Quality,
        $Quality)
    try {
        $Image.Save($Path, $codec, $parameters)
    }
    finally {
        $parameters.Dispose()
    }
}

function Draw-OutlinedText {
    param(
        [System.Drawing.Graphics]$Graphics,
        [string]$Text,
        [System.Drawing.Font]$Font,
        [System.Drawing.PointF]$Position,
        [System.Drawing.Color]$Color,
        [float]$OutlineWidth = 2.0
    )

    $path = New-Object System.Drawing.Drawing2D.GraphicsPath
    $family = $Font.FontFamily
    $style = [int]$Font.Style
    $emSize = $Graphics.DpiY * $Font.Size / 72.0
    $path.AddString($Text, $family, $style, $emSize, $Position, [System.Drawing.StringFormat]::GenericDefault)
    $outline = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(190, 5, 10, 18), $OutlineWidth)
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

function Add-LuxviewMark {
    param(
        [System.Drawing.Graphics]$Graphics,
        [int]$X,
        [int]$Y
    )

    $sky = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(235, 185, 221, 255))
    $sun = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 255, 212, 71))
    $leaf = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 8, 121, 88))
    $wave = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::White)
    try {
        $Graphics.FillRectangle($sky, $X, $Y, 44, 39)
        $Graphics.FillEllipse($sun, $X + 7, $Y + 6, 17, 17)
        $Graphics.FillEllipse($leaf, $X + 25, $Y + 10, 15, 22)
        $Graphics.FillEllipse($wave, $X - 3, $Y + 25, 51, 23)
    }
    finally {
        $wave.Dispose()
        $leaf.Dispose()
        $sun.Dispose()
        $sky.Dispose()
    }
}

function Write-Ozj {
    param(
        [string]$JpegPath,
        [string]$DestinationPath
    )

    $existing = [System.IO.File]::ReadAllBytes($DestinationPath)
    if ($existing.Length -lt 24) {
        throw "OZJ header missing: $DestinationPath"
    }
    $header = New-Object byte[] 24
    [Array]::Copy($existing, 0, $header, 0, 24)
    $jpeg = [System.IO.File]::ReadAllBytes($JpegPath)
    $output = New-Object byte[] ($header.Length + $jpeg.Length)
    [Array]::Copy($header, 0, $output, 0, $header.Length)
    [Array]::Copy($jpeg, 0, $output, $header.Length, $jpeg.Length)
    [System.IO.File]::WriteAllBytes($DestinationPath, $output)
}

$source = [System.Drawing.Image]::FromFile($sourcePath)
$canvas = New-Object System.Drawing.Bitmap(800, 600, [System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
$graphics = [System.Drawing.Graphics]::FromImage($canvas)
try {
    $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
    $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $graphics.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit

    # Crop the 3:2 site artwork to the client's 4:3 loading canvas.
    $sourceRect = New-Object System.Drawing.Rectangle(85, 0, 1366, 1024)
    $destinationRect = New-Object System.Drawing.Rectangle(0, 0, 800, 600)
    $graphics.DrawImage($source, $destinationRect, $sourceRect, [System.Drawing.GraphicsUnit]::Pixel)

    $topGradient = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
        (New-Object System.Drawing.Point(0, 0)),
        (New-Object System.Drawing.Point(0, 170)),
        [System.Drawing.Color]::FromArgb(205, 6, 12, 25),
        [System.Drawing.Color]::FromArgb(0, 6, 12, 25))
    $bottomGradient = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
        (New-Object System.Drawing.Point(0, 395)),
        (New-Object System.Drawing.Point(0, 600)),
        [System.Drawing.Color]::FromArgb(0, 5, 10, 18),
        [System.Drawing.Color]::FromArgb(225, 5, 10, 18))
    try {
        $graphics.FillRectangle($topGradient, 0, 0, 800, 170)
        $graphics.FillRectangle($bottomGradient, 0, 395, 800, 205)
    }
    finally {
        $bottomGradient.Dispose()
        $topGradient.Dispose()
    }

    Add-LuxviewMark -Graphics $graphics -X 34 -Y 29
    $brandFont = New-Object System.Drawing.Font("Segoe UI", 27, [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)
    $gamesFont = New-Object System.Drawing.Font("Segoe UI", 11, [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)
    $titleFont = New-Object System.Drawing.Font("Segoe UI", 36, [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)
    $subtitleFont = New-Object System.Drawing.Font("Segoe UI", 14, [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)
    $taglineFont = New-Object System.Drawing.Font("Segoe UI", 12, [System.Drawing.FontStyle]::Regular, [System.Drawing.GraphicsUnit]::Pixel)
    try {
        Draw-OutlinedText -Graphics $graphics -Text "Luxview" -Font $brandFont -Position (New-Object System.Drawing.PointF(88, 31)) -Color ([System.Drawing.Color]::White) -OutlineWidth 1.5
        Draw-OutlinedText -Graphics $graphics -Text "G A M E S" -Font $gamesFont -Position (New-Object System.Drawing.PointF(90, 62)) -Color ([System.Drawing.Color]::FromArgb(255, 255, 212, 71)) -OutlineWidth 1.0
        Draw-OutlinedText -Graphics $graphics -Text "MU ONLINE" -Font $titleFont -Position (New-Object System.Drawing.PointF(505, 476)) -Color ([System.Drawing.Color]::White) -OutlineWidth 2.4
        Draw-OutlinedText -Graphics $graphics -Text "O CONTINENTE DA NOSTALGIA" -Font $subtitleFont -Position (New-Object System.Drawing.PointF(510, 530)) -Color ([System.Drawing.Color]::FromArgb(255, 255, 212, 71)) -OutlineWidth 1.4
        Draw-OutlinedText -Graphics $graphics -Text "Ideias que ganham vida." -Font $taglineFont -Position (New-Object System.Drawing.PointF(512, 556)) -Color ([System.Drawing.Color]::FromArgb(235, 225, 235, 242)) -OutlineWidth 1.0
    }
    finally {
        $taglineFont.Dispose()
        $subtitleFont.Dispose()
        $titleFont.Dispose()
        $gamesFont.Dispose()
        $brandFont.Dispose()
    }

    Save-Jpeg -Image $canvas -Path $previewPath -Quality 94

    $panels = @(
        @{ Name = "LSBg01.OZJ"; X = 0; Y = 0; Width = 400; Height = 512 },
        @{ Name = "LSBg02.OZJ"; X = 400; Y = 0; Width = 400; Height = 512 },
        @{ Name = "LSBg03.OZJ"; X = 0; Y = 512; Width = 400; Height = 88 },
        @{ Name = "LSBg04.OZJ"; X = 400; Y = 512; Width = 400; Height = 88 }
    )

    foreach ($panel in $panels) {
        $bitmap = New-Object System.Drawing.Bitmap($panel.Width, $panel.Height, [System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
        $panelGraphics = [System.Drawing.Graphics]::FromImage($bitmap)
        $jpegPath = Join-Path $env:TEMP ($panel.Name + ".jpg")
        try {
            $sourcePanel = New-Object System.Drawing.Rectangle($panel.X, $panel.Y, $panel.Width, $panel.Height)
            $targetPanel = New-Object System.Drawing.Rectangle(0, 0, $panel.Width, $panel.Height)
            $panelGraphics.DrawImage($canvas, $targetPanel, $sourcePanel, [System.Drawing.GraphicsUnit]::Pixel)
            Save-Jpeg -Image $bitmap -Path $jpegPath -Quality 94
            Write-Ozj -JpegPath $jpegPath -DestinationPath (Join-Path $dataDir $panel.Name)
        }
        finally {
            $panelGraphics.Dispose()
            $bitmap.Dispose()
            Remove-Item -Force -ErrorAction SilentlyContinue $jpegPath
        }
    }

    # The first startup screen uses a separate 1280x735 six-panel background.
    # Generate both randomized themes so branding never falls back to Webzen art.
    $titlePreviewPath = Join-Path $previewDir "luxview-title-loading-preview.jpg"
    $titleCanvas = New-Object System.Drawing.Bitmap(1280, 735, [System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
    $titleGraphics = [System.Drawing.Graphics]::FromImage($titleCanvas)
    try {
        $titleGraphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
        $titleGraphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
        $titleGraphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
        $titleGraphics.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit

        $titleSource = New-Object System.Drawing.Rectangle(0, 71, 1536, 882)
        $titleTarget = New-Object System.Drawing.Rectangle(0, 0, 1280, 735)
        $titleGraphics.DrawImage($source, $titleTarget, $titleSource, [System.Drawing.GraphicsUnit]::Pixel)

        $titleTopGradient = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
            (New-Object System.Drawing.Point(0, 0)),
            (New-Object System.Drawing.Point(0, 190)),
            [System.Drawing.Color]::FromArgb(210, 5, 10, 20),
            [System.Drawing.Color]::FromArgb(0, 5, 10, 20))
        $titleBottomGradient = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
            (New-Object System.Drawing.Point(0, 490)),
            (New-Object System.Drawing.Point(0, 735)),
            [System.Drawing.Color]::FromArgb(0, 5, 10, 18),
            [System.Drawing.Color]::FromArgb(230, 5, 10, 18))
        try {
            $titleGraphics.FillRectangle($titleTopGradient, 0, 0, 1280, 190)
            $titleGraphics.FillRectangle($titleBottomGradient, 0, 490, 1280, 245)
        }
        finally {
            $titleBottomGradient.Dispose()
            $titleTopGradient.Dispose()
        }

        Add-LuxviewMark -Graphics $titleGraphics -X 52 -Y 38
        $titleBrandFont = New-Object System.Drawing.Font("Segoe UI", 34, [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)
        $titleGamesFont = New-Object System.Drawing.Font("Segoe UI", 13, [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)
        $titleMainFont = New-Object System.Drawing.Font("Segoe UI", 52, [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)
        $titleSubFont = New-Object System.Drawing.Font("Segoe UI", 19, [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)
        try {
            Draw-OutlinedText -Graphics $titleGraphics -Text "Luxview" -Font $titleBrandFont -Position (New-Object System.Drawing.PointF(108, 38)) -Color ([System.Drawing.Color]::White) -OutlineWidth 1.8
            Draw-OutlinedText -Graphics $titleGraphics -Text "G A M E S" -Font $titleGamesFont -Position (New-Object System.Drawing.PointF(111, 77)) -Color ([System.Drawing.Color]::FromArgb(255, 255, 212, 71)) -OutlineWidth 1.1
            Draw-OutlinedText -Graphics $titleGraphics -Text "MU ONLINE" -Font $titleMainFont -Position (New-Object System.Drawing.PointF(835, 590)) -Color ([System.Drawing.Color]::White) -OutlineWidth 3.0
            Draw-OutlinedText -Graphics $titleGraphics -Text "O CONTINENTE DA NOSTALGIA" -Font $titleSubFont -Position (New-Object System.Drawing.PointF(831, 662)) -Color ([System.Drawing.Color]::FromArgb(255, 255, 212, 71)) -OutlineWidth 1.8
        }
        finally {
            $titleSubFont.Dispose()
            $titleMainFont.Dispose()
            $titleGamesFont.Dispose()
            $titleBrandFont.Dispose()
        }

        Save-Jpeg -Image $titleCanvas -Path $titlePreviewPath -Quality 94

        $titlePanels = @(
            @{ Suffix = "01"; X = 0; Y = 0; Width = 512; Height = 512 },
            @{ Suffix = "02"; X = 512; Y = 0; Width = 512; Height = 512 },
            @{ Suffix = "03"; X = 1024; Y = 0; Width = 256; Height = 512 },
            @{ Suffix = "04"; X = 0; Y = 512; Width = 512; Height = 223 },
            @{ Suffix = "05"; X = 512; Y = 512; Width = 512; Height = 223 },
            @{ Suffix = "06"; X = 1024; Y = 512; Width = 256; Height = 223 }
        )
        foreach ($panel in $titlePanels) {
            $bitmap = New-Object System.Drawing.Bitmap($panel.Width, $panel.Height, [System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
            $panelGraphics = [System.Drawing.Graphics]::FromImage($bitmap)
            try {
                $sourcePanel = New-Object System.Drawing.Rectangle($panel.X, $panel.Y, $panel.Width, $panel.Height)
                $targetPanel = New-Object System.Drawing.Rectangle(0, 0, $panel.Width, $panel.Height)
                $panelGraphics.DrawImage($titleCanvas, $targetPanel, $sourcePanel, [System.Drawing.GraphicsUnit]::Pixel)
                foreach ($prefix in @("lo_back_im", "lo_back_s5_im")) {
                    $name = $prefix + $panel.Suffix + ".OZJ"
                    $jpegPath = Join-Path $env:TEMP ($name + ".jpg")
                    try {
                        Save-Jpeg -Image $bitmap -Path $jpegPath -Quality 94
                        Write-Ozj -JpegPath $jpegPath -DestinationPath (Join-Path $dataDir $name)
                    }
                    finally {
                        Remove-Item -Force -ErrorAction SilentlyContinue $jpegPath
                    }
                }
            }
            finally {
                $panelGraphics.Dispose()
                $bitmap.Dispose()
            }
        }
    }
    finally {
        $titleGraphics.Dispose()
        $titleCanvas.Dispose()
    }
}
finally {
    $graphics.Dispose()
    $canvas.Dispose()
    $source.Dispose()
    Remove-Item -Force -ErrorAction SilentlyContinue $sourcePath
}

Write-Output "Generated: $previewPath"
Write-Output "Updated: LSBg01-04 and both lo_back_im01-06 title themes"
