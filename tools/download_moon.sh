#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
base=https://pds-geosciences.wustl.edu/lro/lro-l-lola-3-rdr-v1/lrolol_1xxx/data/lola_gdr/cylindrical/img
mkdir -p data/source/moon
fetch() {
    local name="$1" expected="$2" destination="data/source/moon/$1"
    if [[ -f "$destination" ]] && [[ "$(sha256sum "$destination" | cut -d ' ' -f 1)" == "$expected" ]]; then
        echo "Verified existing $name"
        return
    fi
    curl --fail --location --retry 3 --connect-timeout 30 --max-time 600 "$base/$name" --output "$destination.part"
    echo "$expected  $destination.part" | sha256sum --check
    mv "$destination.part" "$destination"
}
fetch ldem_64.lbl 4e7a94c76c716a147b21ccf07669d004246b0d1b017b7d63d50befaa1a34142d
fetch ldem_64.img 1c4958699d4cffd7e777d51421b6044bba300aa0bef286a544309efb082cdbd6
echo "Ready: build/moon_convert data/source/moon/ldem_64.lbl data/source/moon/ldem_64.img data/moon64.dem"
