param(
  [Parameter(Mandatory=$true)][string]$MasterPath,
  [Parameter(Mandatory=$true)][ValidateSet("classico","season5")][string]$Theme
)
Set-StrictMode -Version Latest
$ErrorActionPreference="Stop"
Add-Type -AssemblyName System.Drawing

$repoRoot = Split-Path -Parent $PSScriptRoot
$dataDir = Join-Path $repoRoot "src\bin\Data\Interface"
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
function Merge-Seams([System.Drawing.Bitmap]$bmp){
  foreach($x in @(511,1023)){
    if($x+1 -ge $bmp.Width){continue}
    for($y=0;$y -lt $bmp.Height;$y++){
      $a=$bmp.GetPixel($x,$y); $b=$bmp.GetPixel($x+1,$y)
      $m=[System.Drawing.Color]::FromArgb([int](($a.R+$b.R)/2),[int](($a.G+$b.G)/2),[int](($a.B+$b.B)/2))
      $bmp.SetPixel($x,$y,$m); $bmp.SetPixel($x+1,$y,$m)
    }
  }
  for($x=0;$x -lt $bmp.Width;$x++){
    $a=$bmp.GetPixel($x,511); $b=$bmp.GetPixel($x,512)
    $m=[System.Drawing.Color]::FromArgb([int](($a.R+$b.R)/2),[int](($a.G+$b.G)/2),[int](($a.B+$b.B)/2))
    $bmp.SetPixel($x,511,$m); $bmp.SetPixel($x,512,$m)
  }
}

$img=[System.Drawing.Image]::FromFile($MasterPath)
try{
  if($img.Width -ne 1280 -or $img.Height -ne 735){ throw ("Dimensao invalida: " + $img.Width + "x" + $img.Height + " - precisa ser exatamente 1280x735") }
  $bmp=New-Object System.Drawing.Bitmap(1280,735,[System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
  $g=[System.Drawing.Graphics]::FromImage($bmp)
  try{
    $g.InterpolationMode=[System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.DrawImage($img,0,0,1280,735)
  }finally{$g.Dispose()}
  Merge-Seams $bmp

  $prefix = if($Theme -eq 'classico'){'lo_back_im'}else{'lo_back_s5_im'}
  $previewName = if($Theme -eq 'classico'){'luxview-title-loading-preview.jpg'}else{'luxview-title-loading-alternate-preview.jpg'}
  Save-Jpeg $bmp (Join-Path $previewDir $previewName) 94

  $panels=@(
    @{S='01';X=0;Y=0;W=512;H=512}, @{S='02';X=512;Y=0;W=512;H=512}, @{S='03';X=1024;Y=0;W=256;H=512},
    @{S='04';X=0;Y=512;W=512;H=223}, @{S='05';X=512;Y=512;W=512;H=223}, @{S='06';X=1024;Y=512;W=256;H=223}
  )
  foreach($p in $panels){
    $b=New-Object System.Drawing.Bitmap($p.W,$p.H,[System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
    $pg=[System.Drawing.Graphics]::FromImage($b)
    $name=$prefix+$p.S+'.OZJ'
    $tmp=Join-Path $env:TEMP ($name+'.jpg')
    try{
      $pg.DrawImage($bmp,(New-Object System.Drawing.Rectangle(0,0,$p.W,$p.H)),(New-Object System.Drawing.Rectangle($p.X,$p.Y,$p.W,$p.H)),[System.Drawing.GraphicsUnit]::Pixel)
      Save-Jpeg $b $tmp 94
      Write-Ozj $tmp (Join-Path $dataDir $name)
    }finally{$pg.Dispose();$b.Dispose();Remove-Item -Force -ErrorAction SilentlyContinue $tmp}
  }
  Write-Output "OK: $Theme atualizado a partir de $MasterPath"
}finally{$img.Dispose()}
