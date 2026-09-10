#!/usr/bin/env bash
# Replace ONLY Data/Local/RadioStations.ini inside the global client zip.
# No main.exe change: the station list is data, not code. Preserves the
# on-air radio assets and Client.Library.dll (md5 checked before/after).
set -euo pipefail
ZIP="/data/luxview/storage/_global/openmu-assets/openmu-s6-base.zip"
INI_SRC="${1:?usage: publish_ini_via_api.sh /path/to/RadioStations.ini}"
WORK=/tmp/mu-ini-publish
rm -rf "$WORK"; mkdir -p "$WORK/Data/Local"
cp -f "$INI_SRC" "$WORK/Data/Local/RadioStations.ini"
python3 - <<'PY'
import hashlib, zipfile
z = zipfile.ZipFile("/data/luxview/storage/_global/openmu-assets/openmu-s6-base.zip")
for n in ["Data/Interface/Radio_icon.OZT","Data/Interface/Radio_btn_plate.OZT","MUnique.Client.Library.dll","Data/Local/RadioStations.ini"]:
    raw = z.read(n)
    print("BEFORE", n, len(raw), hashlib.md5(raw).hexdigest())
PY
echo "=== ini sanity ==="
head -5 "$WORK/Data/Local/RadioStations.ini"
grep -c "=" "$WORK/Data/Local/RadioStations.ini" || true
echo "=== update zip entry ==="
(cd /tmp/mu-ini-publish && zip -u "$ZIP" Data/Local/RadioStations.ini)
chmod 644 "$ZIP"
python3 - <<'PY'
import hashlib, zipfile, os
zpath = "/data/luxview/storage/_global/openmu-assets/openmu-s6-base.zip"
z = zipfile.ZipFile(zpath)
bad = z.testzip()
print("testzip:", "OK" if bad is None else f"CORRUPT {bad}")
for n in ["Data/Interface/Radio_icon.OZT","Data/Interface/Radio_btn_plate.OZT","MUnique.Client.Library.dll","Data/Local/RadioStations.ini"]:
    raw = z.read(n)
    print("AFTER", n, len(raw), hashlib.md5(raw).hexdigest())
st = os.stat(zpath)
print("zip_size", st.st_size)
PY
echo "INI_PUBLISH_OK"
