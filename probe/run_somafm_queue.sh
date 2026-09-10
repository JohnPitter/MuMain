#!/bin/bash
# SomaFM family solo queue — the only infra whose delivery pattern the
# production engine tolerates (family signature: 1-2 underruns/10min,
# max stall <=342ms). Waits (sentinel) until no radio_harness.exe is
# running so it never overlaps the previous queue's link usage.
HARNESS=/c/_wt-radio-cur/out/build/windows-x64/tests/radio/Release/radio_harness.exe
BASE=/c/_wt-radio-cur/probe/runs2/solo
mkdir -p "$BASE"
export SDL_AUDIODRIVER=dummy
echo "=== somafm queue armed $(date -u +%H:%M:%S), waiting for free link ===" >> "$BASE/_queue.txt"
while tasklist //FI "IMAGENAME eq radio_harness.exe" 2>/dev/null | grep -qi radio_harness.exe; do
  sleep 30
done
echo "=== somafm queue start $(date -u +%H:%M:%S) ===" >> "$BASE/_queue.txt"
run() {
  local slug="$1"; local url="$2"
  local dir="$BASE/$slug"; mkdir -p "$dir"
  echo "=== $slug start $(date -u +%H:%M:%S) ===" >> "$BASE/_queue.txt"
  ( cd "$dir" && "$HARNESS" "$url" 10 run.csv > run.log 2>&1; echo "$slug exit=$?" >> "$BASE/_queue.txt" )
}
run secretagent "https://ice1.somafm.com/secretagent-128-mp3"
run vaporwaves "https://ice1.somafm.com/vaporwaves-128-mp3"
run u80s "https://ice1.somafm.com/u80s-128-mp3"
run dronezone "https://ice1.somafm.com/dronezone-128-mp3"
run lush_day "https://ice1.somafm.com/lush-128-mp3"
echo "SOMAFM QUEUE DONE $(date -u +%H:%M:%S)" >> "$BASE/_queue.txt"
