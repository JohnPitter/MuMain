#!/bin/bash
# Curated harness queue: 10-min runs, 5-way parallel waves, per-run cwd
# (harness writes fixed-name radio_harness.log in its cwd).
HARNESS=/c/_wt-radio-cur/out/build/windows-x64/tests/radio/Release/radio_harness.exe
BASE=/c/_wt-radio-cur/probe/runs2
mkdir -p "$BASE"
export SDL_AUDIODRIVER=dummy
run_one() {
  local slug="$1"; local url="$2"
  local dir="$BASE/$slug"; mkdir -p "$dir"
  ( cd "$dir" && "$HARNESS" "$url" 10 run.csv > run.log 2>&1; echo "$slug exit=$?" >> "$BASE/_exits.txt" )
}
declare -A W1=(
  [swiss_jazz]="http://stream.srg-ssr.ch/m/rsj/mp3_128"
  [ella_bossa]="https://stream.ella-radio.de/ella-bossa/mp3-192/"
  [liga_samba]="http://srv1.braudio.com.br:7308/;stream.nsv"
  [kexp]="https://kexp.streamguys1.com/kexp128.mp3"
  [airport_lounge]="https://az1.mediacp.eu/listen/airport-lounge-radio/radio.mp3"
)
declare -A W2=(
  [swiss_pop]="http://stream.srg-ssr.ch/m/rsp/mp3_128"
  [swiss_classic]="http://stream.srg-ssr.ch/m/rsc_de/mp3_128"
  [anonfm]="https://icecast.anon.fm/radio"
  [yumi]="https://yumicoradio.net/stream"
  [ondalatina]="http://ondalatina.stream.laut.fm/ondalatina"
)
declare -A W3=(
  [covers_lounge]="https://az1.mediacp.eu/listen/100coverslounge/radio.mp3"
  [vip_smoothjazz]="https://radio4.vip-radios.fm:18060/stream-128kmp3-SmoothJazzLounge"
  [delicious_beach]="https://server31435.streamplus.de/;stream.mp3"
  [wfmu]="https://stream0.wfmu.org/freeform-128k"
  [0r_lofi]="https://0nlineradio.radioho.st/0r-lo-fi"
)
case "$1" in
  w1) for s in "${!W1[@]}"; do run_one "$s" "${W1[$s]}" & done; wait ;;
  w2) for s in "${!W2[@]}"; do run_one "$s" "${W2[$s]}" & done; wait ;;
  w3) for s in "${!W3[@]}"; do run_one "$s" "${W3[$s]}" & done; wait ;;
  *) echo "usage: run_curated_queue.sh w1|w2|w3" ;;
esac
echo "WAVE $1 DONE" >> "$BASE/_exits.txt"
