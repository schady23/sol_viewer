$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')
$base = 'https://pds-geosciences.wustl.edu/lro/lro-l-lola-3-rdr-v1/lrolol_1xxx/data/lola_gdr/cylindrical/img'
New-Item -ItemType Directory -Force 'data/source/moon' | Out-Null
function Fetch-Moon($name, $expected) {
    $destination = Join-Path 'data/source/moon' $name
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
Fetch-Moon 'ldem_64.lbl' '4e7a94c76c716a147b21ccf07669d004246b0d1b017b7d63d50befaa1a34142d'
Fetch-Moon 'ldem_64.img' '1c4958699d4cffd7e777d51421b6044bba300aa0bef286a544309efb082cdbd6'
Write-Host 'Ready: build/Release/moon_convert.exe data/source/moon/ldem_64.lbl data/source/moon/ldem_64.img data/moon64.dem'
