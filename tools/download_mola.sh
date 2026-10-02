#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
base=https://pds-geosciences.wustl.edu/mgs/urn-nasa-pds-mgs_mola_topography_derived/meg016
mkdir -p data/source
fetch() {
    local name="$1" expected="$2" destination="data/source/$1"
    if [[ -f "$destination" ]] && [[ "$(sha256sum "$destination" | cut -d ' ' -f 1)" == "$expected" ]]; then
        echo "Verified existing $name"
        return
    fi
    curl --fail --location --retry 3 --connect-timeout 30 --max-time 600 "$base/$name" --output "$destination.part"
    echo "$expected  $destination.part" | sha256sum --check
    mv "$destination.part" "$destination"
}
fetch megt90n000eb.lbl 0f2bd919dddb86d83489a008d3007bd915507c892a0f22ce49ad5dd5d2fc7d8a
fetch megt90n000eb.img d18d9b9ab8c5516d02e157dd2cde0f1d0d160c21940e953ba22391269a545e7b
echo "Ready: build/mola_convert data/source/megt90n000eb.lbl data/source/megt90n000eb.img data/mars16.mola"
