$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')
$base = 'https://planetarydata.jpl.nasa.gov/img/data/messenger/MDIS/messdem_1001/DEM/GLOBAL/IMG'
New-Item -ItemType Directory -Force 'data/source/mercury' | Out-Null
function Fetch-Mercury($name, $expected) {
    $destination = Join-Path 'data/source/mercury' $name
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
Fetch-Mercury 'MSGR_DEM_USG_SC_I_V02.LBL' '6433d227b380cfe86b6589a5bed2f3623351475bf49b6d85c95cea093a4bef14'
Fetch-Mercury 'MSGR_DEM_USG_SC_I_V02.IMG' 'b1bb587a9414ff542a74337866582eb7b1b16e325f90a6367116f9f4cc9cdf30'
Write-Host 'Ready: build/Release/mercury_convert.exe data/source/mercury/MSGR_DEM_USG_SC_I_V02.LBL data/source/mercury/MSGR_DEM_USG_SC_I_V02.IMG data/mercury64.dem'
