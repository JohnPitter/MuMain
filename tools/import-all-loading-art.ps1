param([string]$ArtRoot = "C:\Users\joaop\Desenvolvimento\openmu\imagens")
Set-StrictMode -Version Latest
$ErrorActionPreference="Stop"
Add-Type -AssemblyName System.Drawing

$repoRoot = Split-Path -Parent $PSScriptRoot
$dataDir = Join-Path $repoRoot "src\bin\Data"
$previewDir = Join-Path $repoRoot "docs\assets"

function Save-Jpeg([System.Drawing.Image]$img,[string]$path,[long]$q=94){
  $codec=[System.Drawing.Imaging.ImageCodecInfo]::GetImageEncoders() | Where-Object {$_.MimeType -eq 'image/jpeg'}
  $p=New-Object System.Drawing.Imaging.EncoderParameters(1)
  $p.Param[0]=New-Object System.Drawing.Imaging.EncoderParameter([System.Drawing.Imaging.Encoder]::Quality,$q)
  try{$img.Save($path,$codec,$p)}finally{$p.Dispose()}
}
function Write-Ozj([string]$jpegPath,[string]$dest){
  $existing=[IO.File]::ReadAllBytes($dest)
  if($existing.Length -lt 24){ throw "OZJ header missing: $dest" }
  $jpeg=[IO.File]::ReadAllBytes($jpegPath)
  $out=New-Object byte[] (24+$jpeg.Length)
  [Array]::Copy($existing,0,$out,0,24); [Array]::Copy($jpeg,0,$out,24,$jpeg.Length)
  [IO.File]::WriteAllBytes($dest,$out)
}
function Import-Direct([string]$jpg,[string]$ozj){
  Write-Ozj $jpg $ozj
  Write-Output ("OK " + (Split-Path $ozj -Leaf))
}
function Merge-Seams([System.Drawing.Bitmap]$bmp,[int[]]$cutsX,[int[]]$cutsY){
  foreach($cxRaw in @($cutsX)){ $cx=[int]$cxRaw; $xA=$cx-1; $xB=$cx; foreach($xRaw in @($xA,$xB)){ $x=[int]$xRaw;
    if($x -lt 0 -or $x+1 -ge $bmp.Width){continue}
    for($y=0;$y -lt $bmp.Height;$y++){
      $a=$bmp.GetPixel($x,$y); $b=$bmp.GetPixel($x+1,$y)
      $m=[System.Drawing.Color]::FromArgb([int](($a.R+$b.R)/2),[int](($a.G+$b.G)/2),[int](($a.B+$b.B)/2))
      $bmp.SetPixel($x,$y,$m); $bmp.SetPixel($x+1,$y,$m)
    }}}
  foreach($cy in $cutsY){
    if($cy -lt 1 -or $cy -ge $bmp.Height){continue}
    for($x=0;$x -lt $bmp.Width;$x++){
      $a=$bmp.GetPixel($x,$cy-1); $b=$bmp.GetPixel($x,$cy)
      $m=[System.Drawing.Color]::FromArgb([int](($a.R+$b.R)/2),[int](($a.G+$b.G)/2),[int](($a.B+$b.B)/2))
      $bmp.SetPixel($x,$cy-1,$m); $bmp.SetPixel($x,$cy,$m)
    }}
}
function Split-Panels([string]$master,[int]$w,[int]$h,[array]$panels,[string]$previewPath){
  $img=[System.Drawing.Image]::FromFile($master)
  if($img.Width -ne $w -or $img.Height -ne $h){ $img.Dispose(); throw ("Dimensao invalida em " + $master) }
  $bmp=New-Object System.Drawing.Bitmap($w,$h,[System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
  $g=[System.Drawing.Graphics]::FromImage($bmp)
  try{
    $g.InterpolationMode=[System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.DrawImage($img,0,0,$w,$h)
  }finally{$g.Dispose()}
  $img.Dispose()
  Merge-Seams -bmp $bmp -cutsX ([int[]]@(512,1024)) -cutsY ([int[]]@(512))
  if($previewPath){ Save-Jpeg $bmp $previewPath 94 }
  foreach($p in $panels){
    $b=New-Object System.Drawing.Bitmap($p.W,$p.H,[System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
    $pg=[System.Drawing.Graphics]::FromImage($b)
    $tmp=Join-Path $env:TEMP ($p.Name+'.jpg')
    try{
      $pg.DrawImage($bmp,(New-Object System.Drawing.Rectangle(0,0,$p.W,$p.H)),(New-Object System.Drawing.Rectangle($p.X,$p.Y,$p.W,$p.H)),[System.Drawing.GraphicsUnit]::Pixel)
      Save-Jpeg $b $tmp 94
      Write-Ozj $tmp (Join-Path (Join-Path $dataDir 'Interface') $p.Name)
      Write-Output ("OK " + $p.Name)
    }finally{$pg.Dispose();$b.Dispose();Remove-Item -Force -ErrorAction SilentlyContinue $tmp}
  }
  $bmp.Dispose()
}

$titlePanels=@(
  @{S='01';X=0;Y=0;W=512;H=512}, @{S='02';X=512;Y=0;W=512;H=512}, @{S='03';X=1024;Y=0;W=256;H=512},
  @{S='04';X=0;Y=512;W=512;H=223}, @{S='05';X=512;Y=512;W=512;H=223}, @{S='06';X=1024;Y=512;W=256;H=223}
)
$classic=$titlePanels | ForEach-Object { @{Name=('lo_back_im'+$_.S+'.OZJ');X=$_.X;Y=$_.Y;W=$_.W;H=$_.H} }
$season5=$titlePanels | ForEach-Object { @{Name=('lo_back_s5_im'+$_.S+'.OZJ');X=$_.X;Y=$_.Y;W=$_.W;H=$_.H} }

Split-Panels (Join-Path $ArtRoot 'loading-principal\classico\FULL_1280x735.jpg') 1280 735 $classic (Join-Path $previewDir 'luxview-title-loading-preview.jpg')
Split-Panels (Join-Path $ArtRoot 'loading-principal\season5\FULL_1280x735.jpg') 1280 735 $season5 (Join-Path $previewDir 'luxview-title-loading-alternate-preview.jpg')

$mapPanels=@(
  @{Name='LSBg01.OZJ';X=0;Y=0;W=400;H=512}, @{Name='LSBg02.OZJ';X=400;Y=0;W=400;H=512},
  @{Name='LSBg03.OZJ';X=0;Y=512;W=400;H=88}, @{Name='LSBg04.OZJ';X=400;Y=512;W=400;H=88}
)
Split-Panels (Join-Path $ArtRoot 'loading-mapa\LSBg.jpg') 800 600 $mapPanels (Join-Path $previewDir 'luxview-loading-preview.jpg')

$ifDir=Join-Path $dataDir 'Interface'
Import-Direct (Join-Path $ArtRoot 'loading-principal\molduras\New_lo_back_01.jpg') (Join-Path $ifDir 'New_lo_back_01.OZJ')
Import-Direct (Join-Path $ArtRoot 'loading-principal\molduras\New_lo_back_02.jpg') (Join-Path $ifDir 'New_lo_back_02.OZJ')
Import-Direct (Join-Path $ArtRoot 'loading-principal\molduras\lo_back_s5_03.jpg') (Join-Path $ifDir 'lo_back_s5_03.OZJ')
Import-Direct (Join-Path $ArtRoot 'loading-principal\molduras\lo_back_s5_04.jpg') (Join-Path $ifDir 'lo_back_s5_04.OZJ')
Import-Direct (Join-Path $ArtRoot 'loading-principal\molduras\lo_lo.jpg') (Join-Path $ifDir 'lo_lo.OZJ')

$logoDir=Join-Path $dataDir 'Logo'
Import-Direct (Join-Path $ArtRoot 'abertura\Loading01.jpg') (Join-Path $logoDir 'Loading01.OZJ')
Import-Direct (Join-Path $ArtRoot 'abertura\Loading02.jpg') (Join-Path $logoDir 'Loading02.OZJ')
Import-Direct (Join-Path $ArtRoot 'abertura\Loading03.jpg') (Join-Path $logoDir 'Loading03.OZJ')
Import-Direct (Join-Path $ArtRoot 'abertura\Webzenlogo.jpg') (Join-Path $logoDir 'Webzenlogo.OZJ')
Import-Direct (Join-Path $ArtRoot 'abertura\Everyone.jpg') (Join-Path $logoDir 'Everyone.OZJ')
Import-Direct (Join-Path $ArtRoot 'logos\logo.jpg') (Join-Path $logoDir 'logo.OZJ')
Import-Direct (Join-Path $ArtRoot 'logos\logo2.jpg') (Join-Path $logoDir 'logo2.OZJ')
Import-Direct (Join-Path $ArtRoot 'logos\MU-logo_g.jpg') (Join-Path $logoDir 'MU-logo_g.OZJ')
Import-Direct (Join-Path $ArtRoot 'logos\MuBlue_logo_g.jpg') (Join-Path $logoDir 'MuBlue_logo_g.OZJ')
Import-Direct (Join-Path $ArtRoot 'logos\mulogo_01.jpg') (Join-Path $logoDir 'mulogo_01.OZJ')

# copias usadas pela cena de abertura em Data/Local (everyone local e 192x64)
$localDir=Join-Path $dataDir 'Local'
Import-Direct (Join-Path $ArtRoot 'abertura\Loading01.jpg') (Join-Path $localDir 'loading01.ozj')
Import-Direct (Join-Path $ArtRoot 'abertura\Loading02.jpg') (Join-Path $localDir 'loading02.ozj')
Import-Direct (Join-Path $ArtRoot 'abertura\Loading03.jpg') (Join-Path $localDir 'loading03.ozj')
Import-Direct (Join-Path $ArtRoot 'abertura\Webzenlogo.jpg') (Join-Path $localDir 'webzenlogo.ozj')
$ev=[System.Drawing.Image]::FromFile((Join-Path $ArtRoot 'abertura\Everyone.jpg'))
$evb=New-Object System.Drawing.Bitmap(192,64,[System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
$evg=[System.Drawing.Graphics]::FromImage($evb)
try{
  $evg.InterpolationMode=[System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
  $evg.DrawImage($ev,0,0,192,64)
  $tmp=Join-Path $env:TEMP 'everyone-local.jpg'
  Save-Jpeg $evb $tmp 94
  Write-Ozj $tmp (Join-Path $localDir 'everyone.ozj')
  Remove-Item -Force $tmp
  Write-Output 'OK everyone.ozj (192x64)'
}finally{$evg.Dispose();$evb.Dispose();$ev.Dispose()}

Write-Output 'IMPORT COMPLETE'
