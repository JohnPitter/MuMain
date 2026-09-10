#!/bin/bash
# Runs radio_harness for each station in parallel (10 min each).
# SDL_AUDIODRIVER=dummy: no audible output; dummy backend consumes in real time.
HARNESS=/c/_wt-radio-cur/out/build/windows-x64/tests/radio/Release/radio_harness.exe
OUT=/c/_wt-radio-cur/probe/runs
mkdir -p "$OUT"
run_one() {
  local slug="$1"; shift
  local url="$1"; shift
  SDL_AUDIODRIVER=dummy "$HARNESS" "$url" 10 "$OUT/${slug}.csv" > "$OUT/${slug}.log" 2>&1
  echo "$slug exit=$?" >> "$OUT/_exits.txt"
}
rm -f "$OUT/_exits.txt"
declare -A STATIONS=(
  [groovesalad]="https://ice1.somafm.com/groovesalad-128-mp3"
  [lush]="https://ice1.somafm.com/lush-128-mp3"
  [beatblender]="https://ice1.somafm.com/beatblender-128-mp3"
  [sonicuniverse]="https://ice1.somafm.com/sonicuniverse-128-mp3"
  [rp_main]="https://stream.radioparadise.com/mp3-128"
  [nightride]="https://stream.nightride.fm/nightride.mp3"
  [fip]="https://icecast.radiofrance.fr/fip-midfi.mp3"
  [lofi247]="http://usa9.fastcast4u.com/proxy/jamz?mp=/1"
  [mpr_relax]="http://relax.stream.publicradio.org/relax.mp3"
  [classicfm]="http://ice-the.musicradio.com/ClassicFMMP3"
  [jazz_infomaniak]="http://jazz-wr01.ice.infomaniak.ch/jazz-wr01-128.mp3"
  [maqtempo_mpb]="http://servidor28.brlogic.com:8032/live"
  [transamerica]="http://servidor39.brlogic.com:8146/live"
)
for slug in "${!STATIONS[@]}"; do
  run_one "$slug" "${STATIONS[$slug]}" &
done
wait
echo "ALL DONE" >> "$OUT/_exits.txt"
cat "$OUT/_exits.txt"
