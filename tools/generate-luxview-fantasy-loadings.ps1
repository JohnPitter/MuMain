param()
Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

$repoRoot = Split-Path -Parent $PSScriptRoot
$dataDir = Join-Path $repoRoot "src\bin\Data\Interface"
$previewDir = Join-Path $repoRoot "docs\assets"
$exportRoot = "C:\Users\joaop\Desenvolvimento\openmu\imagens"

# ---------- helpers ----------

function New-Canvas([int]$w,[int]$h) {
    $bmp = New-Object System.Drawing.Bitmap($w,$h,[System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit
    return @($bmp,$g)
}

function Save-Jpeg([System.Drawing.Image]$img,[string]$path,[long]$q=94) {
    $codec = [System.Drawing.Imaging.ImageCodecInfo]::GetImageEncoders() | Where-Object { $_.MimeType -eq "image/jpeg" }
    $p = New-Object System.Drawing.Imaging.EncoderParameters(1)
    $p.Param[0] = New-Object System.Drawing.Imaging.EncoderParameter([System.Drawing.Imaging.Encoder]::Quality,$q)
    try { $img.Save($path,$codec,$p) } finally { $p.Dispose() }
}

function Add-GradientSky([System.Drawing.Graphics]$g,[int]$w,[int]$h,[System.Drawing.Color]$top,[System.Drawing.Color]$mid,[System.Drawing.Color]$horizon) {
    $brush = New-Object System.Drawing.Drawing2D.LinearGradientBrush((New-Object System.Drawing.Point(0,0)),(New-Object System.Drawing.Point(0,$h)),$top,$mid)
    try { $g.FillRectangle($brush,0,0,$w,$h) } finally { $brush.Dispose() }
    $glow = New-Object System.Drawing.Drawing2D.LinearGradientBrush((New-Object System.Drawing.Point(0,[int]($h*0.55))),(New-Object System.Drawing.Point(0,$h)),[System.Drawing.Color]::FromArgb(0,$horizon),[System.Drawing.Color]::FromArgb(160,$horizon))
    try { $g.FillRectangle($glow,0,[int]($h*0.55),$w,[int]($h*0.45)) } finally { $glow.Dispose() }
}

function Add-RadialGlow([System.Drawing.Graphics]$g,[float]$cx,[float]$cy,[float]$radius,[System.Drawing.Color]$color,[int]$alpha) {
    $path = New-Object System.Drawing.Drawing2D.GraphicsPath
    $path.AddEllipse($cx-$radius,$cy-$radius,$radius*2,$radius*2)
    $pgb = New-Object System.Drawing.Drawing2D.PathGradientBrush($path)
    $pgb.CenterColor = [System.Drawing.Color]::FromArgb($alpha,$color)
    $pgb.SurroundColors = @([System.Drawing.Color]::FromArgb(0,$color))
    try { $g.FillEllipse($pgb,$cx-$radius,$cy-$radius,$radius*2,$radius*2) } finally { $pgb.Dispose(); $path.Dispose() }
}

function Add-Mountains([System.Drawing.Graphics]$g,[int]$w,[int]$baseY,[int]$peak,[System.Random]$rnd,[System.Drawing.Color]$color) {
    $pts = New-Object System.Collections.Generic.List[System.Drawing.Point]
    $pts.Add((New-Object System.Drawing.Point(0,$baseY)))
    $x = 0
    while ($x -lt $w) {
        $x += $rnd.Next(60,160)
        $y = $baseY - $rnd.Next(20,$peak)
        $pts.Add((New-Object System.Drawing.Point([Math]::Min($x,$w),$y)))
    }
    $pts.Add((New-Object System.Drawing.Point($w,$baseY)))
    $bottomY = $baseY + 400
    $pts.Add((New-Object System.Drawing.Point($w,$bottomY)))
    $pts.Add((New-Object System.Drawing.Point(0,$bottomY)))
    $brush = New-Object System.Drawing.SolidBrush($color)
    try { $g.FillPolygon($brush,$pts.ToArray()) } finally { $brush.Dispose() }
}

function Add-Castle([System.Drawing.Graphics]$g,[int]$groundY,[float]$scale,[int]$centerX,[System.Drawing.Color]$sil,[System.Drawing.Color]$window) {
    $brush = New-Object System.Drawing.SolidBrush($sil)
    $winBrush = New-Object System.Drawing.SolidBrush($window)
    try {
        $s = $scale
        # muro
        $g.FillRectangle($brush,[int]($centerX-260*$s),[int]($groundY-90*$s),[int](520*$s),[int](90*$s))
        # ameias do muro
        for($i=0;$i -lt 13;$i++){ $g.FillRectangle($brush,[int]($centerX-260*$s+$i*40*$s),[int]($groundY-104*$s),[int](24*$s),[int](16*$s)) }
        # torres laterais
        foreach($tx in @(-300,240)){
            $g.FillRectangle($brush,[int]($centerX+$tx*$s),[int]($groundY-220*$s),[int](60*$s),[int](220*$s))
            $roof = @( (New-Object System.Drawing.Point([int]($centerX+$tx*$s-8),[int]($groundY-220*$s))), (New-Object System.Drawing.Point([int]($centerX+($tx+30)*$s),[int]($groundY-290*$s))), (New-Object System.Drawing.Point([int]($centerX+($tx+68)*$s),[int]($groundY-220*$s))) )
            $g.FillPolygon($brush,$roof)
        }
        # torre central
        $g.FillRectangle($brush,[int]($centerX-70*$s),[int]($groundY-300*$s),[int](140*$s),[int](300*$s))
        for($i=0;$i -lt 4;$i++){ $g.FillRectangle($brush,[int]($centerX-70*$s+$i*36*$s),[int]($groundY-318*$s),[int](22*$s),[int](20*$s)) }
        $roofC = @( (New-Object System.Drawing.Point([int]($centerX-84*$s),[int]($groundY-318*$s))), (New-Object System.Drawing.Point([int]($centerX),[int]($groundY-420*$s))), (New-Object System.Drawing.Point([int]($centerX+84*$s),[int]($groundY-318*$s))) )
        $g.FillPolygon($brush,$roofC)
        # janelas acesas
        $rnd = New-Object System.Random(7)
        foreach($wy in @(-60,-110,-160,-240)){
            foreach($wx in @(-40,-8,24)){
                if($rnd.Next(2) -eq 0){ $g.FillRectangle($winBrush,[int]($centerX+$wx*$s),[int]($groundY+$wy*$s),[int](10*$s),[int](16*$s)) }
            }
        }
    } finally { $brush.Dispose(); $winBrush.Dispose() }
}

function Add-Embers([System.Drawing.Graphics]$g,[int]$w,[int]$h,[System.Random]$rnd,[int]$count,[System.Drawing.Color]$color) {
    foreach($i in 0..($count-1)){
        $x = $rnd.Next(0,$w); $y = $rnd.Next([int]($h*0.35),$h)
        $size = 1 + $rnd.NextDouble()*3.5
        $alpha = [Math]::Min(255, 60 + $rnd.Next(160))
        $b = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb($alpha,$color))
        try { $g.FillEllipse($b,[float]$x,[float]$y,[float]$size,[float]$size) } finally { $b.Dispose() }
    }
}

function Add-Vignette([System.Drawing.Graphics]$g,[int]$w,[int]$h) {
    $path = New-Object System.Drawing.Drawing2D.GraphicsPath
    $path.AddRectangle((New-Object System.Drawing.Rectangle(0,0,$w,$h)))
    $pgb = New-Object System.Drawing.Drawing2D.PathGradientBrush($path)
    $pgb.CenterColor = [System.Drawing.Color]::FromArgb(0,0,0,0)
    $pgb.SurroundColors = @([System.Drawing.Color]::FromArgb(150,0,0,0))
    $pgb.CenterPoint = New-Object System.Drawing.PointF([float]($w/2),[float]($h/2))
    try { $g.FillRectangle($pgb,0,0,$w,$h) } finally { $pgb.Dispose(); $path.Dispose() }
}

function Add-BronzeFrame([System.Drawing.Graphics]$g,[int]$w,[int]$h) {
    $outer = [System.Drawing.Color]::FromArgb(200,138,107,63)
    $inner = [System.Drawing.Color]::FromArgb(120,90,62,30)
    $p1 = New-Object System.Drawing.Pen($outer,4)
    $p2 = New-Object System.Drawing.Pen($inner,1.5)
    try {
        $g.DrawRectangle($p1,6,6,$w-12,$h-12)
        $g.DrawRectangle($p2,14,14,$w-28,$h-28)
    } finally { $p1.Dispose(); $p2.Dispose() }
}

function Add-BrandText([System.Drawing.Graphics]$g,[int]$w,[int]$h,[bool]$withMark=$true) {
    $bronze = [System.Drawing.Color]::FromArgb(255,196,150,88)
    $wine = [System.Drawing.Color]::FromArgb(255,158,42,58)
    $brandFont = New-Object System.Drawing.Font("Georgia",30,[System.Drawing.FontStyle]::Bold,[System.Drawing.GraphicsUnit]::Pixel)
    $mainFont = New-Object System.Drawing.Font("Georgia",46,[System.Drawing.FontStyle]::Bold,[System.Drawing.GraphicsUnit]::Pixel)
    $subFont = New-Object System.Drawing.Font("Georgia",15,[System.Drawing.FontStyle]::Bold,[System.Drawing.GraphicsUnit]::Pixel)
    try {
        $shadow = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(200,0,0,0))
        $fb = New-Object System.Drawing.SolidBrush($bronze)
        $sb = New-Object System.Drawing.SolidBrush($wine)
        try {
            $g.DrawString("LUXVIEW",$brandFont,$shadow,52,37)
            $g.DrawString("LUXVIEW",$brandFont,$fb,50,35)
            $g.DrawString("MU ONLINE",$mainFont,$shadow,[float]($w-565),[float]($h-132))
            $g.DrawString("MU ONLINE",$mainFont,$fb,[float]($w-567),[float]($h-134))
            $g.DrawString("O CONTINENTE DA NOSTALGIA",$subFont,$shadow,[float]($w-561),[float]($h-72))
            $g.DrawString("O CONTINENTE DA NOSTALGIA",$subFont,$sb,[float]($w-563),[float]($h-74))
        } finally { $shadow.Dispose(); $fb.Dispose(); $sb.Dispose() }
    } finally { $brandFont.Dispose(); $mainFont.Dispose(); $subFont.Dispose() }
}

function Merge-Seams([System.Drawing.Bitmap]$bmp) {
    foreach($x in @(511,1023)){
        if($x+1 -ge $bmp.Width){ continue }
        for($y=0;$y -lt $bmp.Height;$y++){
            $a=$bmp.GetPixel($x,$y); $b=$bmp.GetPixel($x+1,$y)
            $m=[System.Drawing.Color]::FromArgb([int](($a.R+$b.R)/2),[int](($a.G+$b.G)/2),[int](($a.B+$b.B)/2))
            $bmp.SetPixel($x,$y,$m); $bmp.SetPixel($x+1,$y,$m)
        }
    }
    if($bmp.Height -gt 512){
        for($x=0;$x -lt $bmp.Width;$x++){
            $a=$bmp.GetPixel($x,511); $b=$bmp.GetPixel($x,512)
            $m=[System.Drawing.Color]::FromArgb([int](($a.R+$b.R)/2),[int](($a.G+$b.G)/2),[int](($a.B+$b.B)/2))
            $bmp.SetPixel($x,511,$m); $bmp.SetPixel($x,512,$m)
        }
    }
}

function Write-Ozj([string]$jpegPath,[string]$dest) {
    $existing=[IO.File]::ReadAllBytes($dest)
    if($existing.Length -lt 24){ throw "OZJ header missing: $dest" }
    $jpeg=[IO.File]::ReadAllBytes($jpegPath)
    $out=New-Object byte[] (24+$jpeg.Length)
    [Array]::Copy($existing,0,$out,0,24)
    [Array]::Copy($jpeg,0,$out,24,$jpeg.Length)
    [IO.File]::WriteAllBytes($dest,$out)
}

function Split-TitlePanels([System.Drawing.Bitmap]$master,[string]$prefix) {
    $panels=@(
        @{S="01";X=0;Y=0;W=512;H=512}, @{S="02";X=512;Y=0;W=512;H=512}, @{S="03";X=1024;Y=0;W=256;H=512},
        @{S="04";X=0;Y=512;W=512;H=223}, @{S="05";X=512;Y=512;W=512;H=223}, @{S="06";X=1024;Y=512;W=256;H=223}
    )
    foreach($p in $panels){
        $b=New-Object System.Drawing.Bitmap($p.W,$p.H,[System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
        $pg=[System.Drawing.Graphics]::FromImage($b)
        $name=$prefix+$p.S+".OZJ"
        $tmp=Join-Path $env:TEMP ($name+".jpg")
        try {
            $pg.DrawImage($master,(New-Object System.Drawing.Rectangle(0,0,$p.W,$p.H)),(New-Object System.Drawing.Rectangle($p.X,$p.Y,$p.W,$p.H)),[System.Drawing.GraphicsUnit]::Pixel)
            Save-Jpeg $b $tmp 94
            Write-Ozj $tmp (Join-Path $dataDir $name)
        } finally { $pg.Dispose(); $b.Dispose(); Remove-Item -Force -ErrorAction SilentlyContinue $tmp }
    }
}

function Split-MapPanels([System.Drawing.Bitmap]$master) {
    $panels=@(
        @{N="LSBg01.OZJ";X=0;Y=0;W=400;H=512}, @{N="LSBg02.OZJ";X=400;Y=0;W=400;H=512},
        @{N="LSBg03.OZJ";X=0;Y=512;W=400;H=88}, @{N="LSBg04.OZJ";X=400;Y=512;W=400;H=88}
    )
    foreach($p in $panels){
        $b=New-Object System.Drawing.Bitmap($p.W,$p.H,[System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
        $pg=[System.Drawing.Graphics]::FromImage($b)
        $tmp=Join-Path $env:TEMP ($p.N+".jpg")
        try {
            $pg.DrawImage($master,(New-Object System.Drawing.Rectangle(0,0,$p.W,$p.H)),(New-Object System.Drawing.Rectangle($p.X,$p.Y,$p.W,$p.H)),[System.Drawing.GraphicsUnit]::Pixel)
            Save-Jpeg $b $tmp 94
            Write-Ozj $tmp (Join-Path $dataDir $p.N)
        } finally { $pg.Dispose(); $b.Dispose(); Remove-Item -Force -ErrorAction SilentlyContinue $tmp }
    }
}

# ---------- CLASSIC: fortaleza ao fogo âmbar ----------
$amber = [System.Drawing.Color]::FromArgb(255,176,86)
$graphite = [System.Drawing.Color]::FromArgb(13,13,16)
$v = New-Canvas 1280 735
$bmp=$v[0]; $g=$v[1]
try {
    Add-GradientSky $g 1280 735 ([System.Drawing.Color]::FromArgb(8,8,11)) ([System.Drawing.Color]::FromArgb(22,17,13)) $amber
    Add-RadialGlow $g 960 620 520 $amber 110
    $moonB = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(235,238,220,190))
    try { Add-RadialGlow $g 1010 150 150 $amber 90; $g.FillEllipse($moonB,955,95,110,110) } finally { $moonB.Dispose() }
    $rnd = New-Object System.Random(42)
    Add-Mountains $g 1280 520 130 $rnd ([System.Drawing.Color]::FromArgb(26,26,30))
    Add-Mountains $g 1280 570 90 $rnd ([System.Drawing.Color]::FromArgb(15,15,18))
    Add-Castle $g 690 0.72 400 ([System.Drawing.Color]::FromArgb(9,9,11)) $amber
    Add-RadialGlow $g 1080 700 300 $amber 130
    Add-Embers $g 1280 735 $rnd 130 $amber
    $ground = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(6,6,8))
    try { $g.FillRectangle($ground,0,690,1280,45) } finally { $ground.Dispose() }
    Add-Vignette $g 1280 735
    Add-BronzeFrame $g 1280 735
    Add-BrandText $g 1280 735
    Merge-Seams $bmp
    Save-Jpeg $bmp (Join-Path $previewDir "luxview-title-loading-preview.jpg") 94
    Save-Jpeg $bmp (Join-Path $exportRoot "loading-principal\classico\FULL_1280x735.jpg") 94
    Split-TitlePanels $bmp "lo_back_im"
} finally { $g.Dispose(); $bmp.Dispose() }

# ---------- SEASON 5: portal demoníaco vinho + magia azul ----------
$wine = [System.Drawing.Color]::FromArgb(142,30,46)
$magic = [System.Drawing.Color]::FromArgb(96,156,255)
$v = New-Canvas 1280 735
$bmp=$v[0]; $g=$v[1]
try {
    Add-GradientSky $g 1280 735 ([System.Drawing.Color]::FromArgb(9,7,10)) ([System.Drawing.Color]::FromArgb(26,10,14)) $wine
    Add-RadialGlow $g 640 380 430 $wine 120
    # vortice: elipses rotacionadas
    $pen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(120,$wine),3)
    $penB = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(150,$magic),2)
    $state = $g.Save()
    try {
        $g.TranslateTransform(640,380)
        foreach($i in 0..11){
            $r1 = 300.0 - $i*22
            $g.RotateTransform(15)
            $g.DrawEllipse($pen,-$r1,-$r1*0.42,$r1*2,$r1*0.84)
        }
        $g.ResetTransform()
        # circulo de runas azul
        $g.TranslateTransform(640,560)
        $g.DrawEllipse($penB,-150,-42,300,84)
        foreach($i in 0..17){
            $ang = [Math]::PI*2*$i/18
            $x1 = [Math]::Cos($ang)*150; $y1=[Math]::Sin($ang)*42
            $x2 = [Math]::Cos($ang)*132; $y2=[Math]::Sin($ang)*37
            $g.DrawLine($penB,[float]$x1,[float]$y1,[float]$x2,[float]$y2)
        }
        $g.ResetTransform()
    } finally { $g.Restore($state); $pen.Dispose(); $penB.Dispose() }
    # espada cravada (silhueta central)
    $sil = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(8,8,10))
    try {
        $blade = @((New-Object System.Drawing.Point(632,170)),(New-Object System.Drawing.Point(640,140)),(New-Object System.Drawing.Point(648,170)),(New-Object System.Drawing.Point(648,470)),(New-Object System.Drawing.Point(632,470)))
        $g.FillPolygon($sil,$blade)
        $g.FillRectangle($sil,600,470,80,16)   # guarda
        $g.FillRectangle($sil,628,486,24,58)   # punho
        $g.FillEllipse($sil,622,540,36,20)     # pomo
    } finally { $sil.Dispose() }
    Add-RadialGlow $g 640 300 190 $magic 70
    $rnd = New-Object System.Random(99)
    Add-Embers $g 1280 735 $rnd 110 $magic
    Add-Mountains $g 1280 640 70 $rnd ([System.Drawing.Color]::FromArgb(12,9,12))
    Add-Vignette $g 1280 735
    Add-BronzeFrame $g 1280 735
    Add-BrandText $g 1280 735
    Merge-Seams $bmp
    Save-Jpeg $bmp (Join-Path $previewDir "luxview-title-loading-alternate-preview.jpg") 94
    Save-Jpeg $bmp (Join-Path $exportRoot "loading-principal\season5\FULL_1280x735.jpg") 94
    Split-TitlePanels $bmp "lo_back_s5_im"
} finally { $g.Dispose(); $bmp.Dispose() }

# ---------- MAPA (LSBg 800x600): campo de brasas ----------
$v = New-Canvas 800 600
$bmp=$v[0]; $g=$v[1]
try {
    Add-GradientSky $g 800 600 ([System.Drawing.Color]::FromArgb(8,8,11)) ([System.Drawing.Color]::FromArgb(24,16,12)) $amber
    Add-RadialGlow $g 400 560 420 $amber 120
    $rnd = New-Object System.Random(13)
    Add-Mountains $g 800 430 110 $rnd ([System.Drawing.Color]::FromArgb(24,24,28))
    Add-Mountains $g 800 480 70 $rnd ([System.Drawing.Color]::FromArgb(14,14,17))
    Add-Castle $g 570 0.5 250 ([System.Drawing.Color]::FromArgb(9,9,11)) $amber
    Add-Embers $g 800 600 $rnd 150 $amber
    Add-Vignette $g 800 600
    Add-BronzeFrame $g 800 600
    $mainFont = New-Object System.Drawing.Font("Georgia",34,[System.Drawing.FontStyle]::Bold,[System.Drawing.GraphicsUnit]::Pixel)
    $subFont = New-Object System.Drawing.Font("Georgia",13,[System.Drawing.FontStyle]::Bold,[System.Drawing.GraphicsUnit]::Pixel)
    $shadow = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(200,0,0,0))
    $fb = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255,196,150,88))
    try {
        $g.DrawString("LUXVIEW MU",$mainFont,$shadow,42,502)
        $g.DrawString("LUXVIEW MU",$mainFont,$fb,40,500)
        $g.DrawString("CARREGANDO O CONTINENTE",$subFont,$shadow,44,550)
        $g.DrawString("CARREGANDO O CONTINENTE",$subFont,$fb,42,548)
    } finally { $mainFont.Dispose(); $subFont.Dispose(); $shadow.Dispose(); $fb.Dispose() }
    Save-Jpeg $bmp (Join-Path $previewDir "luxview-loading-preview.jpg") 94
    Save-Jpeg $bmp (Join-Path $exportRoot "loading-mapa\LSBg.jpg") 94
    Split-MapPanels $bmp
} finally { $g.Dispose(); $bmp.Dispose() }

Write-Output "Generated classic, season5 and map loading arts + OZJ panels"
