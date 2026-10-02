#!/usr/bin/env bash
# Offline preparation only; the application never downloads data.
set -euo pipefail
mkdir -p data/source/earth
base='https://www.ngdc.noaa.gov/thredds/dodsC/global/ETOPO2022/60s/60s_surface_elev_netcdf/ETOPO_2022_v1_60s_N90W180_surface.nc'
for i in {0..9}; do
    printf -v band '%02d' "$i"
    first=$((4+i*1080))
    last=$((1075+i*1080))
    output="data/source/earth/etopo2022-band${band}.dods"
    if [[ -f "$output" ]]; then continue; fi
    curl --fail --location --retry 3 --globoff "${base}.dods?z[${first}:9:${last}][4:9:21595]" -o "${output}.part"
    mv "${output}.part" "$output"
done
python3 tools/prepare_earth_topography.py data/source/earth "${1:-assets/earth2022.dem}"
