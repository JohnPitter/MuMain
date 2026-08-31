param(
    [Parameter(Mandatory = $true)] [string] $Path,
    [switch] $Verify
)

Set-StrictMode -Version Latest
$FixedRecordSize = 53
$NameOffset = 2
$NameSize = 32
$DescriptionLengthOffset = 51
$BuxCode = [byte[]](0xFC, 0xCF, 0xAB)
$Names = @{
    0 = 'MuRaiz S6 Easy'
    1 = 'MuRaiz 99d Hard'
    2 = 'MuRaiz S2 Medium'
}

function Convert-Bux([byte[]] $Bytes) {
    for ($i = 0; $i -lt $Bytes.Length; $i++) {
        $Bytes[$i] = [byte]($Bytes[$i] -bxor $BuxCode[$i % $BuxCode.Length])
    }
    return $Bytes
}

function Read-Records([byte[]] $Encrypted, [bool] $Rename) {
    $Output = [Collections.Generic.List[byte]]::new()
    $Offset = 0
    $Found = @{}
    $Sequences = @{}
    while ($Offset -lt $Encrypted.Length) {
        if ($Offset + $FixedRecordSize -gt $Encrypted.Length) { throw "Truncated record at offset $Offset" }
        [byte[]] $Fixed = $Encrypted[$Offset..($Offset + $FixedRecordSize - 1)]
        Convert-Bux $Fixed | Out-Null
        $Index = [BitConverter]::ToUInt16($Fixed, 0)
        $Name = [Text.Encoding]::ASCII.GetString($Fixed, $NameOffset, $NameSize).TrimEnd([char]0)
        if ($Rename -and $Names.ContainsKey([int]$Index)) {
            $NewName = $Names[[int]$Index]
            $NameBytes = [Text.Encoding]::ASCII.GetBytes($NewName)
            if ($NameBytes.Length -ge $NameSize) { throw "Name '$NewName' does not fit in $NameSize bytes" }
            [Array]::Clear($Fixed, $NameOffset, $NameSize)
            [Array]::Copy($NameBytes, 0, $Fixed, $NameOffset, $NameBytes.Length)
            $Name = $NewName
        }
        $Found[[int]$Index] = $Name
        $Sequences[[int]$Index] = [int]$Fixed[35]
        $DescriptionLength = [BitConverter]::ToInt16($Fixed, $DescriptionLengthOffset)
        foreach ($Byte in (Convert-Bux ([byte[]]$Fixed))) { $Output.Add($Byte) }
        if ($DescriptionLength -lt 0 -or $Offset + $FixedRecordSize + $DescriptionLength -gt $Encrypted.Length) { throw "Invalid description length $DescriptionLength at offset $Offset" }
        if ($DescriptionLength -gt 0) {
            [byte[]] $Description = $Encrypted[($Offset + $FixedRecordSize)..($Offset + $FixedRecordSize + $DescriptionLength - 1)]
            foreach ($Byte in $Description) { $Output.Add($Byte) }
        }
        $Offset += $FixedRecordSize + $DescriptionLength
    }
    if ($Rename -and $Output.Count -ne $Encrypted.Length) { throw 'Output size changed' }
    return @{ Bytes = $Output.ToArray(); Names = $Found; Sequences = $Sequences }
}

[byte[]] $FileBytes = [IO.File]::ReadAllBytes($Path)
$Result = Read-Records $FileBytes (-not $Verify)
if (-not $Verify) { [IO.File]::WriteAllBytes($Path, $Result.Bytes) }
foreach ($Index in ($Result.Names.Keys | Sort-Object)) {
    "index=$Index sequence=$($Result.Sequences[$Index]) name=$($Result.Names[$Index])"
}
"size=$($FileBytes.Length)"
