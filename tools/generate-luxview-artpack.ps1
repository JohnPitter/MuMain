param()
Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

$repoRoot = Split-Path -Parent $PSScriptRoot
$dataDir = Join-Path $repoRoot "src\bin\Data"
$ifDir = Join-Path $dataDir "Interface"
$logoDir = Join-Path $dataDir "Logo"
$localDir = Join-Path $dataDir "Local"
$previewDir = Join-Path $repoRoot "docs\assets"
$exportRoot = "C:\Users\joaop\Desenvolvimento\openmu\imagens"

$amber  = [System.Drawing.Color]::FromArgb(255,176,86)
$bronze = [System.Drawing.Color]::FromArgb(196,150,88)
$bronzeLight = [System.Drawing.Color]::FromArgb(235,205,140)
$bronzeDark  = [System.Drawing.Color]::FromArgb(110,74,32)
$wine   = [System.Drawing.Color]::FromArgb(142,30,46)
$magic  = [System.Drawing.Color]::FromArgb(96,156,255)
$graphite = [System.Drawing.Color]::FromArgb(13,13,16)

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

function Write-Ozj([string]$jpegPath,[string]$dest) {
    $existing=[IO.File]::ReadAllBytes($dest)
    if($existing.Length -lt 24){ throw ("OZJ header missing: " + $dest) }
    $jpeg=[IO.File]::ReadAllBytes($jpegPath)
    $out=New-Object byte[] (24+$jpeg.Length)
    [Array]::Copy($existing,0,$out,0,24)
    [Array]::Copy($jpeg,0,$out,24,$jpeg.Length)
    [IO.File]::WriteAllBytes($dest,$out)
}

function Save-Direct([System.Drawing.Bitmap]$bmp,[string]$dest) {
    $tmp = Join-Path $env:TEMP ((Split-Path $dest -Leaf)+".jpg")
    try { Save-Jpeg $bmp $tmp 94; Write-Ozj $tmp $dest; Write-Output ("OK " + (Split-Path $dest -Leaf)) }
    finally { Remove-Item -Force -ErrorAction SilentlyContinue $tmp }
}

function Add-GradientSky([System.Drawing.Graphics]$g,[int]$w,[int]$h,[System.Drawing.Color]$top,[System.Drawing.Color]$mid,[System.Drawing.Color]$horizon) {
    $p0 = New-Object System.Drawing.Point(0,0)
    $p1 = New-Object System.Drawing.Point(0,$h)
    $brush = New-Object System.Drawing.Drawing2D.LinearGradientBrush($p0,$p1,$top,$mid)
    try { $g.FillRectangle($brush,0,0,$w,$h) } finally { $brush.Dispose() }
    $gy = [int]($h*0.55)
    $gh = $h - $gy
    $q0 = New-Object System.Drawing.Point(0,$gy)
    $q1 = New-Object System.Drawing.Point(0,$h)
    $glow = New-Object System.Drawing.Drawing2D.LinearGradientBrush($q0,$q1,[System.Drawing.Color]::FromArgb(0,$horizon),[System.Drawing.Color]::FromArgb(160,$horizon))
    try { $g.FillRectangle($glow,0,$gy,$w,$gh) } finally { $glow.Dispose() }
}

function Add-RadialGlow([System.Drawing.Graphics]$g,[float]$cx,[float]$cy,[float]$radius,[System.Drawing.Color]$color,[int]$alpha) {
    $x = $cx-$radius; $y = $cy-$radius; $d = $radius*2
    $path = New-Object System.Drawing.Drawing2D.GraphicsPath
    $path.AddEllipse($x,$y,$d,$d)
    $pgb = New-Object System.Drawing.Drawing2D.PathGradientBrush($path)
    $pgb.CenterColor = [System.Drawing.Color]::FromArgb($alpha,$color)
    $pgb.SurroundColors = @([System.Drawing.Color]::FromArgb(0,$color))
    try { $g.FillEllipse($pgb,$x,$y,$d,$d) } finally { $pgb.Dispose(); $path.Dispose() }
}

function Add-Mountains([System.Drawing.Graphics]$g,[int]$w,[int]$baseY,[int]$peak,[System.Random]$rnd,[System.Drawing.Color]$color) {
    $pts = New-Object System.Collections.Generic.List[System.Drawing.Point]
    $pts.Add((New-Object System.Drawing.Point(0,$baseY)))
    $x = 0
    while ($x -lt $w) {
        $x = $x + $rnd.Next(60,160)
        if ($x -gt $w) { $x = $w }
        $y = $baseY - $rnd.Next(20,$peak)
        $pts.Add((New-Object System.Drawing.Point($x,$y)))
    }
    $pts.Add((New-Object System.Drawing.Point($w,$baseY)))
    $bottomY = $baseY + 400
    $pts.Add((New-Object System.Drawing.Point($w,$bottomY)))
    $pts.Add((New-Object System.Drawing.Point(0,$bottomY)))
    $brush = New-Object System.Drawing.SolidBrush($color)
    try { $g.FillPolygon($brush,$pts.ToArray()) } finally { $brush.Dispose() }
}

function Add-Castle([System.Drawing.Graphics]$g,[int]$groundY,[float]$s,[int]$cx,[System.Drawing.Color]$sil,[System.Drawing.Color]$window) {
    $brush = New-Object System.Drawing.SolidBrush($sil)
    $winBrush = New-Object System.Drawing.SolidBrush($window)
    try {
        $wallX = $cx - [int](260*$s); $wallY = $groundY - [int](90*$s); $wallW = [int](520*$s); $wallH = [int](90*$s)
        $g.FillRectangle($brush,$wallX,$wallY,$wallW,$wallH)
        for($i=0;$i -lt 13;$i++){
            $bx = $cx - [int](260*$s) + [int]($i*40*$s); $by = $groundY - [int](104*$s)
            $g.FillRectangle($brush,$bx,$by,[int](24*$s),[int](16*$s))
        }
        foreach($tx in @(-300,240)){
            $tox = $cx + [int]($tx*$s); $toy = $groundY - [int](220*$s)
            $g.FillRectangle($brush,$tox,$toy,[int](60*$s),[int](220*$s))
            $r1x = $tox - 8; $r1y = $toy
            $r2x = $cx + [int](($tx+30)*$s); $r2y = $groundY - [int](290*$s)
            $r3x = $cx + [int](($tx+68)*$s); $r3y = $toy
            $roof = @((New-Object System.Drawing.Point($r1x,$r1y)),(New-Object System.Drawing.Point($r2x,$r2y)),(New-Object System.Drawing.Point($r3x,$r3y)))
            $g.FillPolygon($brush,$roof)
        }
        $ctx = $cx - [int](70*$s); $cty = $groundY - [int](300*$s)
        $g.FillRectangle($brush,$ctx,$cty,[int](140*$s),[int](300*$s))
        for($i=0;$i -lt 4;$i++){
            $bx = $cx - [int](70*$s) + [int]($i*36*$s); $by = $groundY - [int](318*$s)
            $g.FillRectangle($brush,$bx,$by,[int](22*$s),[int](20*$s))
        }
        $c1x = $cx - [int](84*$s); $c1y = $groundY - [int](318*$s)
        $c2x = $cx; $c2y = $groundY - [int](420*$s)
        $c3x = $cx + [int](84*$s); $c3y = $c1y
        $roofC = @((New-Object System.Drawing.Point($c1x,$c1y)),(New-Object System.Drawing.Point($c2x,$c2y)),(New-Object System.Drawing.Point($c3x,$c3y)))
        $g.FillPolygon($brush,$roofC)
        $rnd = New-Object System.Random(7)
        foreach($wy in @(-60,-110,-160,-240)){
            foreach($wx in @(-40,-8,24)){
                if($rnd.Next(2) -eq 0){
                    $wxp = $cx + [int]($wx*$s); $wyp = $groundY + [int]($wy*$s)
                    $g.FillRectangle($winBrush,$wxp,$wyp,[int](10*$s),[int](16*$s))
                }
            }
        }
    } finally { $brush.Dispose(); $winBrush.Dispose() }
}

function Add-Embers([System.Drawing.Graphics]$g,[int]$w,[int]$h,[System.Random]$rnd,[int]$count,[System.Drawing.Color]$color) {
    for($i=0;$i -lt $count;$i++){
        $x = $rnd.Next(0,$w); $yMin = [int]($h*0.35); $y = $rnd.Next($yMin,$h)
        $size = 1.0 + $rnd.NextDouble()*3.5
        $alpha = [Math]::Min(255, 60 + $rnd.Next(160))
        $b = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb($alpha,$color))
        try { $g.FillEllipse($b,[float]$x,[float]$y,[float]$size,[float]$size) } finally { $b.Dispose() }
    }
}

function Add-Vignette([System.Drawing.Graphics]$g,[int]$w,[int]$h) {
    $rect = New-Object System.Drawing.Rectangle(0,0,$w,$h)
    $path = New-Object System.Drawing.Drawing2D.GraphicsPath
    $path.AddRectangle($rect)
    $pgb = New-Object System.Drawing.Drawing2D.PathGradientBrush($path)
    $pgb.CenterColor = [System.Drawing.Color]::FromArgb(0,0,0,0)
    $pgb.SurroundColors = @([System.Drawing.Color]::FromArgb(150,0,0,0))
    $cw = $w/2; $ch = $h/2
    $pgb.CenterPoint = New-Object System.Drawing.PointF([float]$cw,[float]$ch)
    try { $g.FillRectangle($pgb,0,0,$w,$h) } finally { $pgb.Dispose(); $path.Dispose() }
}

function Add-BronzeFrame([System.Drawing.Graphics]$g,[int]$w,[int]$h) {
    $outer = [System.Drawing.Color]::FromArgb(200,138,107,63)
    $inner = [System.Drawing.Color]::FromArgb(120,90,62,30)
    $p1 = New-Object System.Drawing.Pen($outer,4)
    $p2 = New-Object System.Drawing.Pen($inner,1.5)
    try {
        $g.DrawRectangle($p1,6,6,($w-12),($h-12))
        $g.DrawRectangle($p2,14,14,($w-28),($h-28))
    } finally { $p1.Dispose(); $p2.Dispose() }
}

function Merge-Seams([System.Drawing.Bitmap]$bmp,[int[]]$cutsX,[int[]]$cutsY) {
    foreach($cxRaw in @($cutsX)) {
        $cx=[int]$cxRaw; $xA=$cx-1; $xB=$cx
        foreach($xRaw in @($xA,$xB)) {
            $x=[int]$xRaw
            if($x -lt 0 -or ($x+1) -ge $bmp.Width){continue}
            for($y=0;$y -lt $bmp.Height;$y++){
                $a=$bmp.GetPixel($x,$y); $b=$bmp.GetPixel(($x+1),$y)
                $m=[System.Drawing.Color]::FromArgb([int](($a.R+$b.R)/2),[int](($a.G+$b.G)/2),[int](($a.B+$b.B)/2))
                $bmp.SetPixel($x,$y,$m); $bmp.SetPixel(($x+1),$y,$m)
            }
        }
    }
    foreach($cyRaw in @($cutsY)) {
        $cy=[int]$cyRaw
        if($cy -lt 1 -or $cy -ge $bmp.Height){continue}
        $cyM1 = $cy-1
        for($x=0;$x -lt $bmp.Width;$x++){
            $a=$bmp.GetPixel($x,$cyM1); $b=$bmp.GetPixel($x,$cy)
            $m=[System.Drawing.Color]::FromArgb([int](($a.R+$b.R)/2),[int](($a.G+$b.G)/2),[int](($a.B+$b.B)/2))
            $bmp.SetPixel($x,$cyM1,$m); $bmp.SetPixel($x,$cy,$m)
        }
    }
}

function Add-Warrior([System.Drawing.Graphics]$g,[int]$cx,[int]$groundY,[float]$s,[System.Drawing.Color]$body,[System.Drawing.Color]$trim) {
    $b = New-Object System.Drawing.SolidBrush($body)
    $t = New-Object System.Drawing.SolidBrush($trim)
    try {
        $capeTopY = $groundY - [int](260*$s)
        $c1x = $cx - [int](70*$s); $c2x = $cx - [int](140*$s); $c3x = $cx - [int](10*$s); $c4x = $cx - [int](5*$s)
        $cape = @((New-Object System.Drawing.Point($c1x,$capeTopY)),(New-Object System.Drawing.Point($c2x,$groundY)),(New-Object System.Drawing.Point($c3x,$groundY)),(New-Object System.Drawing.Point($c4x,$capeTopY)))
        $g.FillPolygon($b,$cape)

        $torsoX = $cx - [int](36*$s); $torsoY = $groundY - [int](210*$s); $torsoW = [int](72*$s); $torsoH = [int](180*$s)
        $g.FillRectangle($b,$torsoX,$torsoY,$torsoW,$torsoH)

        $legX = $cx - [int](28*$s); $legY = $groundY - [int](40*$s); $legW = [int](56*$s); $legH = [int](40*$s)
        $g.FillRectangle($b,$legX,$legY,$legW,$legH)

        $shR = [int](22*$s)
        $shLx = $cx - [int](58*$s); $shLy = $groundY - [int](218*$s)
        $shRx = $cx + [int](14*$s); $shRy = $shLy
        $g.FillEllipse($t,$shLx,$shLy,$shR,$shR)
        $g.FillEllipse($t,$shRx,$shRy,$shR,$shR)

        $headR = [int](30*$s)
        $headX = $cx - [int](15*$s); $headY = $groundY - [int](250*$s)
        $g.FillEllipse($b,$headX,$headY,$headR,$headR)

        $swX1 = $cx + [int](40*$s); $swY1 = $groundY - [int](260*$s)
        $swX2 = $cx + [int](48*$s); $swY2 = $groundY - [int](250*$s)
        $swX3 = $cx + [int](30*$s); $swY3 = $groundY - [int](20*$s)
        $swX4 = $cx + [int](22*$s); $swY4 = $groundY - [int](30*$s)
        $blade = @((New-Object System.Drawing.Point($swX1,$swY1)),(New-Object System.Drawing.Point($swX2,$swY2)),(New-Object System.Drawing.Point($swX3,$swY3)),(New-Object System.Drawing.Point($swX4,$swY4)))
        $g.FillPolygon($t,$blade)
    } finally { $b.Dispose(); $t.Dispose() }
}

function Add-Mage([System.Drawing.Graphics]$g,[int]$cx,[int]$groundY,[float]$s,[System.Drawing.Color]$body,[System.Drawing.Color]$glow) {
    $b = New-Object System.Drawing.SolidBrush($body)
    try {
        $topY = $groundY - [int](230*$s)
        $r1x = $cx; $r1y = $topY
        $r2x = $cx - [int](60*$s); $r2y = $groundY
        $r3x = $cx + [int](60*$s); $r3y = $groundY
        $robe = @((New-Object System.Drawing.Point($r1x,$r1y)),(New-Object System.Drawing.Point($r2x,$r2y)),(New-Object System.Drawing.Point($r3x,$r3y)))
        $g.FillPolygon($b,$robe)
        $headR = [int](26*$s)
        $headX = $cx - [int](13*$s); $headY = $topY - [int](22*$s)
        $g.FillEllipse($b,$headX,$headY,$headR,$headR)
        $staffX = $cx + [int](40*$s); $staffTopY = $topY - [int](10*$s)
        $pen = New-Object System.Drawing.Pen($body,[float](4*$s))
        try { $g.DrawLine($pen,$staffX,$staffTopY,$staffX,$groundY) } finally { $pen.Dispose() }
        Add-RadialGlow $g $staffX $staffTopY ([float](26*$s)) $glow 200
    } finally { $b.Dispose() }
}

function Draw-OutlinedText([System.Drawing.Graphics]$g,[string]$text,[System.Drawing.Font]$font,[System.Drawing.PointF]$pos,[System.Drawing.Brush]$fillBrush,[float]$outlineW) {
    $path = New-Object System.Drawing.Drawing2D.GraphicsPath
    $emSize = $g.DpiY * $font.Size / 72.0
    $path.AddString($text,$font.FontFamily,[int]$font.Style,$emSize,$pos,[System.Drawing.StringFormat]::GenericDefault)
    $outline = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(220,4,4,6),$outlineW)
    try { $g.DrawPath($outline,$path); $g.FillPath($fillBrush,$path) } finally { $outline.Dispose(); $path.Dispose() }
}

function Get-BronzeBrush([float]$x,[float]$y,[float]$w,[float]$h) {
    $rect = New-Object System.Drawing.RectangleF($x,$y,$w,$h)
    return New-Object System.Drawing.Drawing2D.LinearGradientBrush($rect,$bronzeLight,$bronzeDark,[System.Drawing.Drawing2D.LinearGradientMode]::Vertical)
}

function Write-Ozj-Panel([System.Drawing.Bitmap]$master,[string]$name,[int]$x,[int]$y,[int]$w,[int]$h,[string]$outDir) {
    $b=New-Object System.Drawing.Bitmap($w,$h,[System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
    $pg=[System.Drawing.Graphics]::FromImage($b)
    $srcRect = New-Object System.Drawing.Rectangle($x,$y,$w,$h)
    $dstRect = New-Object System.Drawing.Rectangle(0,0,$w,$h)
    $tmp=Join-Path $env:TEMP ($name+".jpg")
    try {
        $pg.DrawImage($master,$dstRect,$srcRect,[System.Drawing.GraphicsUnit]::Pixel)
        Save-Jpeg $b $tmp 94
        Write-Ozj $tmp (Join-Path $outDir $name)
        Write-Output ("OK " + $name)
    } finally { $pg.Dispose(); $b.Dispose(); Remove-Item -Force -ErrorAction SilentlyContinue $tmp }
}

# ============================================================
# CLASSIC master 1280x735
# ============================================================
$v = New-Canvas 1280 735
$bmp=$v[0]; $g=$v[1]
try {
    Add-GradientSky $g 1280 735 ([System.Drawing.Color]::FromArgb(8,8,11)) ([System.Drawing.Color]::FromArgb(22,17,13)) $amber
    $moonB = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(235,238,220,190))
    try { Add-RadialGlow $g 1010 150 150 $amber 90; $g.FillEllipse($moonB,955,95,110,110) } finally { $moonB.Dispose() }
    $rnd = New-Object System.Random(42)
    Add-Mountains $g 1280 520 130 $rnd ([System.Drawing.Color]::FromArgb(26,26,30))
    Add-Mountains $g 1280 570 90 $rnd ([System.Drawing.Color]::FromArgb(15,15,18))
    Add-Castle $g 690 0.72 640 ([System.Drawing.Color]::FromArgb(9,9,11)) $amber
    Add-RadialGlow $g 640 330 110 $amber 90
    Add-Warrior $g 190 700 1.05 ([System.Drawing.Color]::FromArgb(10,10,12)) $bronze
    Add-Mage $g 1140 700 0.95 ([System.Drawing.Color]::FromArgb(9,9,11)) $magic
    Add-RadialGlow $g 1080 700 300 $amber 110
    Add-Embers $g 1280 735 $rnd 130 $amber
    $ground = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(6,6,8))
    try { $g.FillRectangle($ground,0,690,1280,45) } finally { $ground.Dispose() }
    Add-Vignette $g 1280 735
    Add-BronzeFrame $g 1280 735
    Merge-Seams $bmp -cutsX ([int[]]@(512,1024)) -cutsY ([int[]]@(512))
    Save-Jpeg $bmp (Join-Path $previewDir "luxview-title-loading-preview.jpg") 94
    if (Test-Path $exportRoot) { Save-Jpeg $bmp (Join-Path $exportRoot "loading-principal\classico\FULL_1280x735.jpg") 94 }
    $panels=@(
        @{S="01";X=0;Y=0;W=512;H=512}, @{S="02";X=512;Y=0;W=512;H=512}, @{S="03";X=1024;Y=0;W=256;H=512},
        @{S="04";X=0;Y=512;W=512;H=223}, @{S="05";X=512;Y=512;W=512;H=223}, @{S="06";X=1024;Y=512;W=256;H=223}
    )
    foreach($p in $panels){ Write-Ozj-Panel $bmp ("lo_back_im"+$p.S+".OZJ") $p.X $p.Y $p.W $p.H $ifDir }
} finally { $g.Dispose(); $bmp.Dispose() }

# ============================================================
# SEASON5 master 1280x735
# ============================================================
$v = New-Canvas 1280 735
$bmp=$v[0]; $g=$v[1]
try {
    Add-GradientSky $g 1280 735 ([System.Drawing.Color]::FromArgb(9,7,10)) ([System.Drawing.Color]::FromArgb(26,10,14)) $wine
    Add-RadialGlow $g 640 380 430 $wine 120
    $pen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(120,$wine),3)
    $penB = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(150,$magic),2)
    $state = $g.Save()
    try {
        $g.TranslateTransform(640,380)
        for($i=0;$i -lt 12;$i++){
            $r1 = 300.0 - $i*22
            $g.RotateTransform(15)
            $g.DrawEllipse($pen,(-$r1),(-$r1*0.42),($r1*2),($r1*0.84))
        }
        $g.ResetTransform()
        $g.TranslateTransform(640,560)
        $g.DrawEllipse($penB,-150,-42,300,84)
        for($i=0;$i -lt 18;$i++){
            $ang = [Math]::PI*2*$i/18
            $x1 = [Math]::Cos($ang)*150; $y1=[Math]::Sin($ang)*42
            $x2 = [Math]::Cos($ang)*132; $y2=[Math]::Sin($ang)*37
            $g.DrawLine($penB,[float]$x1,[float]$y1,[float]$x2,[float]$y2)
        }
        $g.ResetTransform()
    } finally { $g.Restore($state); $pen.Dispose(); $penB.Dispose() }
    $sil = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(8,8,10))
    try {
        $blade = @((New-Object System.Drawing.Point(632,170)),(New-Object System.Drawing.Point(640,140)),(New-Object System.Drawing.Point(648,170)),(New-Object System.Drawing.Point(648,470)),(New-Object System.Drawing.Point(632,470)))
        $g.FillPolygon($sil,$blade)
        $g.FillRectangle($sil,600,470,80,16)
        $g.FillRectangle($sil,628,486,24,58)
        $g.FillEllipse($sil,622,540,36,20)
    } finally { $sil.Dispose() }
    Add-RadialGlow $g 640 300 190 $magic 70
    Add-Warrior $g 175 705 1.0 ([System.Drawing.Color]::FromArgb(10,8,9)) $bronze
    Add-Mage $g 1130 705 0.95 ([System.Drawing.Color]::FromArgb(9,7,8)) $magic
    $rnd = New-Object System.Random(99)
    Add-Embers $g 1280 735 $rnd 110 $magic
    Add-Mountains $g 1280 640 70 $rnd ([System.Drawing.Color]::FromArgb(12,9,12))
    Add-Vignette $g 1280 735
    Add-BronzeFrame $g 1280 735
    Merge-Seams $bmp -cutsX ([int[]]@(512,1024)) -cutsY ([int[]]@(512))
    Save-Jpeg $bmp (Join-Path $previewDir "luxview-title-loading-alternate-preview.jpg") 94
    if (Test-Path $exportRoot) { Save-Jpeg $bmp (Join-Path $exportRoot "loading-principal\season5\FULL_1280x735.jpg") 94 }
    $panels=@(
        @{S="01";X=0;Y=0;W=512;H=512}, @{S="02";X=512;Y=0;W=512;H=512}, @{S="03";X=1024;Y=0;W=256;H=512},
        @{S="04";X=0;Y=512;W=512;H=223}, @{S="05";X=512;Y=512;W=512;H=223}, @{S="06";X=1024;Y=512;W=256;H=223}
    )
    foreach($p in $panels){ Write-Ozj-Panel $bmp ("lo_back_s5_im"+$p.S+".OZJ") $p.X $p.Y $p.W $p.H $ifDir }
} finally { $g.Dispose(); $bmp.Dispose() }

# ============================================================
# LSBg map loading 800x600
# ============================================================
$v = New-Canvas 800 600
$bmp=$v[0]; $g=$v[1]
try {
    Add-GradientSky $g 800 600 ([System.Drawing.Color]::FromArgb(8,8,11)) ([System.Drawing.Color]::FromArgb(24,16,12)) $amber
    Add-RadialGlow $g 400 560 420 $amber 120
    $rnd = New-Object System.Random(13)
    Add-Mountains $g 800 430 110 $rnd ([System.Drawing.Color]::FromArgb(24,24,28))
    Add-Mountains $g 800 480 70 $rnd ([System.Drawing.Color]::FromArgb(14,14,17))
    Add-Castle $g 570 0.5 400 ([System.Drawing.Color]::FromArgb(9,9,11)) $amber
    Add-Warrior $g 140 585 0.62 ([System.Drawing.Color]::FromArgb(10,10,12)) $bronze
    Add-Embers $g 800 600 $rnd 150 $amber
    Add-Vignette $g 800 600
    Add-BronzeFrame $g 800 600
    Merge-Seams $bmp -cutsX ([int[]]@(400)) -cutsY ([int[]]@(512))
    Save-Jpeg $bmp (Join-Path $previewDir "luxview-loading-preview.jpg") 94
    if (Test-Path $exportRoot) { Save-Jpeg $bmp (Join-Path $exportRoot "loading-mapa\LSBg.jpg") 94 }
    Write-Ozj-Panel $bmp "LSBg01.OZJ" 0 0 400 512 $ifDir
    Write-Ozj-Panel $bmp "LSBg02.OZJ" 400 0 400 512 $ifDir
    Write-Ozj-Panel $bmp "LSBg03.OZJ" 0 512 400 88 $ifDir
    Write-Ozj-Panel $bmp "LSBg04.OZJ" 400 512 400 88 $ifDir
} finally { $g.Dispose(); $bmp.Dispose() }

# ============================================================
# MOLDURAS: New_lo_back_01/02 (400x69), lo_back_s5_03/04 (400x100), lo_lo (4x15)
# ============================================================
foreach($pair in @(@("New_lo_back_01.OZJ",0),@("New_lo_back_02.OZJ",1))) {
    $v = New-Canvas 400 69
    $bmp=$v[0]; $g=$v[1]
    try {
        $g.Clear($graphite)
        $rect = New-Object System.Drawing.RectangleF(0,0,400,69)
        $bg = New-Object System.Drawing.Drawing2D.LinearGradientBrush($rect,([System.Drawing.Color]::FromArgb(18,16,16)),([System.Drawing.Color]::FromArgb(40,20,18)),[System.Drawing.Drawing2D.LinearGradientMode]::Horizontal)
        try { $g.FillRectangle($bg,0,0,400,69) } finally { $bg.Dispose() }
        Add-RadialGlow $g 200 34 130 $amber 60
        $pen1 = New-Object System.Drawing.Pen($bronze,2)
        $pen2 = New-Object System.Drawing.Pen($bronzeDark,1)
        try {
            $g.DrawRectangle($pen1,3,3,393,62)
            $g.DrawRectangle($pen2,8,8,383,52)
            $g.DrawEllipse($pen1,190,24,20,20)
        } finally { $pen1.Dispose(); $pen2.Dispose() }
        Save-Direct $bmp (Join-Path $ifDir $pair[0])
    } finally { $g.Dispose(); $bmp.Dispose() }
}

foreach($name in @("lo_back_s5_03.OZJ","lo_back_s5_04.OZJ")) {
    $v = New-Canvas 400 100
    $bmp=$v[0]; $g=$v[1]
    try {
        $rect = New-Object System.Drawing.RectangleF(0,0,400,100)
        $bg = New-Object System.Drawing.Drawing2D.LinearGradientBrush($rect,([System.Drawing.Color]::FromArgb(16,14,15)),([System.Drawing.Color]::FromArgb(34,16,16)),[System.Drawing.Drawing2D.LinearGradientMode]::Horizontal)
        try { $g.FillRectangle($bg,0,0,400,100) } finally { $bg.Dispose() }
        Add-RadialGlow $g 200 50 160 $wine 70
        Add-RadialGlow $g 200 50 90 $amber 40
        $pen1 = New-Object System.Drawing.Pen($bronze,3)
        $pen2 = New-Object System.Drawing.Pen($bronzeDark,1)
        try {
            $g.DrawRectangle($pen1,4,4,392,92)
            $g.DrawRectangle($pen2,12,12,376,76)
        } finally { $pen1.Dispose(); $pen2.Dispose() }
        Save-Direct $bmp (Join-Path $ifDir $name)
    } finally { $g.Dispose(); $bmp.Dispose() }
}

# lo_lo: 4x15 repeating gauge-bar fill tile
$v = New-Canvas 4 15
$bmp=$v[0]; $g=$v[1]
try {
    $rect = New-Object System.Drawing.RectangleF(0,0,4,15)
    $bg = New-Object System.Drawing.Drawing2D.LinearGradientBrush($rect,$bronzeLight,$bronzeDark,[System.Drawing.Drawing2D.LinearGradientMode]::Vertical)
    try { $g.FillRectangle($bg,0,0,4,15) } finally { $bg.Dispose() }
    $ap = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(160,$amber))
    try { $g.FillRectangle($ap,0,6,4,3) } finally { $ap.Dispose() }
    Save-Direct $bmp (Join-Path $ifDir "lo_lo.OZJ")
} finally { $g.Dispose(); $bmp.Dispose() }

# ============================================================
# SPLASHES: Loading01 (256x256 emblem), Loading02 (128x256 rune blade), Loading03 (256x256 winged)
# ============================================================
$v = New-Canvas 256 256
$bmp=$v[0]; $g=$v[1]
try {
    $g.Clear($graphite)
    Add-RadialGlow $g 128 128 130 $amber 90
    $pen = New-Object System.Drawing.Pen($bronze,3)
    $pen2 = New-Object System.Drawing.Pen($bronzeDark,1.5)
    try {
        $g.DrawEllipse($pen,38,38,180,180)
        $g.DrawEllipse($pen2,54,54,148,148)
        for($i=0;$i -lt 8;$i++){
            $ang=[Math]::PI*2*$i/8
            $x1=128+[Math]::Cos($ang)*90; $y1=128+[Math]::Sin($ang)*90
            $x2=128+[Math]::Cos($ang)*74; $y2=128+[Math]::Sin($ang)*74
            $g.DrawLine($pen2,[float]$x1,[float]$y1,[float]$x2,[float]$y2)
        }
    } finally { $pen.Dispose(); $pen2.Dispose() }
    $font=New-Object System.Drawing.Font("Georgia",34,[System.Drawing.FontStyle]::Bold,[System.Drawing.GraphicsUnit]::Pixel)
    try {
        $sz=$g.MeasureString("MU",$font)
        $px=(256-$sz.Width)/2; $py=(256-$sz.Height)/2
        $bb=Get-BronzeBrush $px $py $sz.Width $sz.Height
        try { Draw-OutlinedText $g "MU" $font (New-Object System.Drawing.PointF([float]$px,[float]$py)) $bb 2.4 } finally { $bb.Dispose() }
    } finally { $font.Dispose() }
    Save-Direct $bmp (Join-Path $logoDir "Loading01.OZJ")
    Save-Direct $bmp (Join-Path $localDir "loading01.ozj")
} finally { $g.Dispose(); $bmp.Dispose() }

$v = New-Canvas 128 256
$bmp=$v[0]; $g=$v[1]
try {
    $g.Clear($graphite)
    Add-RadialGlow $g 64 128 110 $amber 80
    $sil = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(8,8,10))
    try {
        $blade=@((New-Object System.Drawing.Point(58,30)),(New-Object System.Drawing.Point(70,30)),(New-Object System.Drawing.Point(74,60)),(New-Object System.Drawing.Point(54,60)))
        $g.FillPolygon($sil,$blade)
        $g.FillRectangle($sil,60,60,8,150)
        $g.FillRectangle($sil,40,210,48,10)
        $g.FillRectangle($sil,54,220,20,26)
    } finally { $sil.Dispose() }
    $pen2 = New-Object System.Drawing.Pen($bronzeDark,1)
    try { $g.DrawEllipse($pen2,24,120,80,80) } finally { $pen2.Dispose() }
    Save-Direct $bmp (Join-Path $logoDir "Loading02.OZJ")
    Save-Direct $bmp (Join-Path $localDir "loading02.ozj")
} finally { $g.Dispose(); $bmp.Dispose() }

$v = New-Canvas 256 256
$bmp=$v[0]; $g=$v[1]
try {
    Add-GradientSky $g 256 256 ([System.Drawing.Color]::FromArgb(9,8,10)) ([System.Drawing.Color]::FromArgb(24,12,14)) $wine
    Add-RadialGlow $g 128 100 120 $wine 110
    $sil = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(9,9,11))
    try {
        $wingL=@((New-Object System.Drawing.Point(128,90)),(New-Object System.Drawing.Point(40,60)),(New-Object System.Drawing.Point(60,130)),(New-Object System.Drawing.Point(110,120)))
        $wingR=@((New-Object System.Drawing.Point(128,90)),(New-Object System.Drawing.Point(216,60)),(New-Object System.Drawing.Point(196,130)),(New-Object System.Drawing.Point(146,120)))
        $g.FillPolygon($sil,$wingL); $g.FillPolygon($sil,$wingR)
        $g.FillEllipse($sil,110,70,36,36)
        $g.FillRectangle($sil,116,100,24,90)
    } finally { $sil.Dispose() }
    Add-RadialGlow $g 128 200 90 $amber 60
    Add-BronzeFrame $g 256 256
    Save-Direct $bmp (Join-Path $logoDir "Loading03.OZJ")
    Save-Direct $bmp (Join-Path $localDir "loading03.ozj")
} finally { $g.Dispose(); $bmp.Dispose() }

# ============================================================
# LOGOS: logo/logo2 (256x128), MU-logo_g (512x256), MuBlue_logo_g (432x384), mulogo_01 (256x166)
# ============================================================
function Build-Wordmark([int]$w,[int]$h,[string]$text,[float]$fontSize,[System.Drawing.Color]$glow,[string]$dest) {
    $v = New-Canvas $w $h
    $bmp=$v[0]; $g=$v[1]
    try {
        $g.Clear($graphite)
        $cx = $w/2; $cy = $h/2
        Add-RadialGlow $g $cx $cy ([float]($w*0.4)) $glow 70
        $font=New-Object System.Drawing.Font("Georgia",$fontSize,[System.Drawing.FontStyle]::Bold,[System.Drawing.GraphicsUnit]::Pixel)
        try {
            $sz=$g.MeasureString($text,$font)
            $px=($w-$sz.Width)/2; $py=($h-$sz.Height)/2
            $bb=Get-BronzeBrush $px $py $sz.Width $sz.Height
            try { Draw-OutlinedText $g $text $font (New-Object System.Drawing.PointF([float]$px,[float]$py)) $bb ([float]($fontSize*0.06)) } finally { $bb.Dispose() }
        } finally { $font.Dispose() }
        $lineY = $cy + ($h*0.22)
        $pen = New-Object System.Drawing.Pen($bronzeDark,1.5)
        try {
            $g.DrawLine($pen,($w*0.12),$lineY,($w*0.38),$lineY)
            $g.DrawLine($pen,($w*0.62),$lineY,($w*0.88),$lineY)
        } finally { $pen.Dispose() }
        Save-Direct $bmp $dest
    } finally { $g.Dispose(); $bmp.Dispose() }
}

Build-Wordmark 256 128 "MU" 46 $amber (Join-Path $logoDir "logo.OZJ")
Build-Wordmark 256 128 "MU" 46 $wine (Join-Path $logoDir "logo2.OZJ")
Build-Wordmark 512 256 "MU ONLINE" 62 $amber (Join-Path $logoDir "MU-logo_g.OZJ")
Build-Wordmark 432 384 "MU ONLINE" 54 $magic (Join-Path $logoDir "MuBlue_logo_g.OZJ")
Build-Wordmark 256 166 "MU" 40 $bronze (Join-Path $logoDir "mulogo_01.OZJ")

Write-Output "ARTPACK COMPLETE"
