#!/bin/bash
# Sequential solo harness runs (10 min each) — one stream at a time.
cd /c/_wt-radio-cur/probe/solo || exit 1
HARNESS=/c/_wt-radio-cur/out/build/windows-x64/tests/radio/Release/radio_harness.exe
export SDL_AUDIODRIVER=dummy
run() {
  local slug="$1"; local url="$2"
  echo "=== $slug start $(date -u +%H:%M:%S) ==="
  "$HARNESS" "$url" 10 "$slug.csv" > "$slug.log" 2>&1
  echo "$slug exit=$?" >> _solo_exits.txt
}
rm -f _solo_exits.txt
run rp_main "https://stream.radioparadise.com/mp3-128"
run lush "https://ice1.somafm.com/lush-128-mp3"
run beatblender "https://ice1.somafm.com/beatblender-128-mp3"
run sonicuniverse "https://ice1.somafm.com/sonicuniverse-128-mp3"
run groovesalad_ice4 "https://ice4.somafm.com/groovesalad-128-mp3"
run maqtempo_mpb "http://servidor28.brlogic.com:8032/live"
run lofi247 "http://usa9.fastcast4u.com/proxy/jamz?mp=/1"
run classicfm "http://ice-the.musicradio.com/ClassicFMMP3"
run jazz_infomaniak "http://jazz-wr01.ice.infomaniak.ch/jazz-wr01-128.mp3"
echo "QUEUE DONE"
