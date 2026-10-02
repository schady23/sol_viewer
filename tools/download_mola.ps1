$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')
$base = 'https://pds-geosciences.wustl.edu/mgs/urn-nasa-pds-mgs_mola_topography_derived/meg016'
New-Item -ItemType Directory -Force 'data/source' | Out-Null
function Fetch-Mola($name, $expected) {
    $destination = Join-Path 'data/source' $name
    if ((Test-Path $destination) -and ((Get-FileHash $destination -Algorithm SHA256).Hash -eq $expected)) {
        Write-Host "Verified existing $name"
        return
    }
    Invoke-WebRequest "$base/$name" -OutFile "$destination.part"
    if ((Get-FileHash "$destination.part" -Algorithm SHA256).Hash -ne $expected) {
        throw "Checksum mismatch for $name; source files have not been replaced."
    }
    Move-Item -Force "$destination.part" $destination
}
Fetch-Mola 'megt90n000eb.lbl' '0f2bd919dddb86d83489a008d3007bd915507c892a0f22ce49ad5dd5d2fc7d8a'
Fetch-Mola 'megt90n000eb.img' 'd18d9b9ab8c5516d02e157dd2cde0f1d0d160c21940e953ba22391269a545e7b'
Write-Host 'Ready: build/Release/mola_convert.exe data/source/megt90n000eb.lbl data/source/megt90n000eb.img data/mars16.mola'
