"""Render a short real mesh-animation review from the verified Blender source."""
import json
from pathlib import Path
import sys

SOURCE = Path(__file__).resolve().parent
sys.path.insert(0, str(SOURCE))
from motion_review import render_preview

ROOT = SOURCE / 'authored-armor'


def main():
    report = json.loads((ROOT / 'armor-report.json').read_text())
    clip = next(clip for clip in report['clips'] if clip['name'] == 'PLAYER_WALK_SWORD')
    render_preview(ROOT / 'celestial-authored-armor.blend', ROOT / 'Celestial_Armor_walk_preview.mp4',
                   (clip['start'], clip['end'] - 1))


if __name__ == '__main__':
    main()
