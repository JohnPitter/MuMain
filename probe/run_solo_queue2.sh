#!/bin/bash
# Sequential SOLO harness queue — the client plays ONE station at a time;
# parallel runs starve the link (proven by wave1 midday contamination).
HARNESS=/c/_wt-radio-cur/out/build/windows-x64/tests/radio/Release/radio_harness.exe
BASE=/c/_wt-radio-cur/probe/runs2/solo
mkdir -p "$BASE"
export SDL_AUDIODRIVER=dummy
run() {
  local slug="$1"; local url="$2"
  local dir="$BASE/$slug"; mkdir -p "$dir"
  echo "=== $slug start $(date -u +%H:%M:%S) ===" >> "$BASE/_queue.txt"
  ( cd "$dir" && "$HARNESS" "$url" 10 run.csv > run.log 2>&1; echo "$slug exit=$?" >> "$BASE/_queue.txt" )
}
run groovesalad_ref "https://ice1.somafm.com/groovesalad-128-mp3"
run liga_samba "http://srv1.braudio.com.br:7308/;stream.nsv"
run anonfm "https://icecast.anon.fm/radio"
run vip_smoothjazz "https://radio4.vip-radios.fm:18060/stream-128kmp3-SmoothJazzLounge"
run delicious_beach "https://server31435.streamplus.de/;stream.mp3"
run yumi "https://yumicoradio.net/stream"
run ella_bossa "https://stream.ella-radio.de/ella-bossa/mp3-192/"
run airport_lounge_solo "https://az1.mediacp.eu/listen/airport-lounge-radio/radio.mp3"
echo "QUEUE2 DONE $(date -u +%H:%M:%S)" >> "$BASE/_queue.txt"
