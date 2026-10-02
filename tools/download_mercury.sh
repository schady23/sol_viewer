#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
base=https://planetarydata.jpl.nasa.gov/img/data/messenger/MDIS/messdem_1001/DEM/GLOBAL/IMG
mkdir -p data/source/mercury
fetch() {
    local name="$1" expected="$2" destination="data/source/mercury/$1"
    if [[ -f "$destination" ]] && [[ "$(sha256sum "$destination" | cut -d ' ' -f 1)" == "$expected" ]]; then
        echo "Verified existing $name"
        return
    fi
    curl --fail --location --retry 3 --connect-timeout 30 --max-time 600 "$base/$name" --output "$destination.part"
    echo "$expected  $destination.part" | sha256sum --check
    mv "$destination.part" "$destination"
}
fetch MSGR_DEM_USG_SC_I_V02.LBL 6433d227b380cfe86b6589a5bed2f3623351475bf49b6d85c95cea093a4bef14
fetch MSGR_DEM_USG_SC_I_V02.IMG b1bb587a9414ff542a74337866582eb7b1b16e325f90a6367116f9f4cc9cdf30
echo "Ready: build/mercury_convert data/source/mercury/MSGR_DEM_USG_SC_I_V02.LBL data/source/mercury/MSGR_DEM_USG_SC_I_V02.IMG data/mercury64.dem"
