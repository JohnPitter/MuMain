"""Compose the human-reviewed visual notes without inventing item identities."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2] / 'docs/art/armor-reference'


def main():
    catalog = json.loads((ROOT / 'canonical-families.json').read_text(encoding='utf-8-sig'))
    notes = {}
    for name in ('early', 'late'):
        part = json.loads((ROOT / f'visual-style-notes-{name}.json').read_text(encoding='utf-8-sig'))
        if notes.keys() & part.keys():
            raise ValueError('Duplicate visual notes')
        notes.update(part)
    if set(notes) != {str(f['item_number']) for f in catalog['families']}:
        raise ValueError('Visual notes must cover every canonical armor family')
    lines = [
        '# Formas, estilos e leitura visual das armaduras', '',
        'Inspeção humana assistida das 70 famílias: frente e costas, em 140 renders dos BMD reais. '
        'As descrições são interpretações visuais, não nomes oficiais de estilos nem avaliação ingame. '
        'As imagens usam a pose-base e não incluem armas, asas, partículas ou brilho do cliente.', '',
        'Referência canônica: base Ultimate de 11/09/2026 anterior à inversão dourada do Celestial. '
        'Paletas de atlas estão no [catálogo](catalog.md); regras de renderização no [guia](runtime-guide.md). '
        'Cabelo/rosto podem vir das referências nativas. A composição não preenche todas as '
        'lacunas com corpo/cabeça de classe: especialmente 2, 10–13, 15, 20, 21, 23 e 24. '
        'Vazios entre peças não comprovam recorte do traje nem defeito ingame. '
        'Peça ausente não equivale automaticamente a item quebrado; confira as variantes.', '',
        '| Família / nome confirmado no servidor | Formas, ornamentação e verso | Imagens |',
        '|---|---|---|',
    ]
    for family in catalog['families']:
        number = family['item_number']
        entries = family['pieces'].get('Armor') or next(iter(family['pieces'].values()))
        name = (entries[0].get('server') or {}).get('name') or 'Nome não confirmado no servidor'
        front = f'native-renders/family-{number:03d}-front.png'
        rear = f'native-renders/family-{number:03d}-rear.png'
        lines.append(f'| {number} — {name} | {notes[str(number)]} | [Frente]({front}) · [Costas]({rear}) |')
    lines.extend(['', '## Pranchas comparativas', ''])
    for page in range(1, 8):
        lines.append(f'- Prancha {page}: [frente](native-sheets/front-{page:02d}.png) / '
                     f'[costas](native-sheets/rear-{page:02d}.png).')
    (ROOT / 'visual-styles.md').write_text('\n'.join(lines) + '\n', encoding='utf-8')
    print(f'Written visual styles for {len(notes)} families')


if __name__ == '__main__':
    main()
