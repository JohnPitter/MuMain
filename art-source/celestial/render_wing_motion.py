"""Render one continuous cycle of the actual authored wing mesh animation."""
from pathlib import Path
import sys

SOURCE = Path(__file__).resolve().parent
sys.path.insert(0, str(SOURCE))
from motion_review import render_preview

ROOT = SOURCE / 'authored-wings'


if __name__ == '__main__':
    render_preview(ROOT / 'celestial-authored-wings.blend', ROOT / 'Celestial_Wings_flap_preview.mp4', (1, 32))
