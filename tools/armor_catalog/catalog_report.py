"""Generate diagnostic coverage tables and contact sheets from actual textures."""
from pathlib import Path

from PIL import Image, ImageDraw


SHEET_COLUMNS = 6
SHEET_ROWS = 5
CELL_WIDTH = 190
CELL_HEIGHT = 215
METRIC_LABELS = {
    'sources': 'Bases de origem', 'bmd_occurrences': 'Ocorrências de arquivos BMD',
    'unique_bmd_sha256': 'Arquivos BMD únicos por SHA-256', 'duplicate_bmd_occurrences': 'Ocorrências duplicadas por conteúdo',
    'canonical_bmd': 'Arquivos BMD na base canônica', 'parsed_unique_bmd': 'Malhas únicas lidas com sucesso',
    'failed_unique_bmd': 'Falhas de leitura de malha', 'non_mesh_bmd_tables': 'Tabelas BMD sem malha 3D',
    'unique_texture_sha256': 'Texturas únicas por SHA-256', 'failed_texture_decode': 'Falhas de decodificação de textura',
    'missing_texture_references': 'Referências de textura não resolvidas no diretório (todas as bases)',
    'canonical_missing_texture_references': 'Referências de textura não resolvidas no diretório (base canônica)',
    'conflicting_same_paths': 'Caminhos com malhas diferentes entre bases',
    'appearance_variant_paths': 'Caminhos com variantes de arquivos de malha/material entre bases',
}
CATEGORY_LABELS = {
    'accessory': 'Acessórios', 'unmapped_item_resource': 'Recursos de item sem identidade confirmada',
    'consumable_quest_or_other': 'Consumíveis, missões ou outros', 'weapon': 'Armas', 'shield': 'Escudos',
    'wings_or_cape': 'Asas ou capas', 'helper_pet_or_other': 'Ajudantes, mascotes ou outros',
    'player_auxiliary_resource': 'Recursos auxiliares de personagem', 'unmapped_equipment_candidate': 'Candidatos de equipamento sem vínculo confirmado',
    'class_body_or_head': 'Corpos/cabeças de classe', 'equipment': 'Peças e variantes de armadura',
    'player_animation_reference': 'Referências de animação do personagem',
}


def texture_sheets(report, output):
    references = {}
    for occurrence in report['occurrences']:
        if not occurrence['canonical']:
            continue
        for texture in occurrence['textures']:
            if texture.get('status') == 'ok':
                references.setdefault(texture['sha256'], texture['declared'])
    items, sheets = sorted(references.items(), key=lambda item: (item[1].lower(), item[0])), []
    page_size = SHEET_COLUMNS * SHEET_ROWS
    for offset in range(0, len(items), page_size):
        sheet = Image.new('RGB', (SHEET_COLUMNS * CELL_WIDTH, SHEET_ROWS * CELL_HEIGHT), '#20252B')
        draw = ImageDraw.Draw(sheet)
        for cell, (digest, name) in enumerate(items[offset:offset + page_size]):
            x, y = cell % SHEET_COLUMNS * CELL_WIDTH, cell // SHEET_COLUMNS * CELL_HEIGHT
            texture = report['textures'][digest]
            with Image.open(output / texture['thumbnail']) as image:
                sheet.paste(image, (x + (CELL_WIDTH - image.width) // 2, y + 5), image)
            draw.text((x + 8, y + 168), name[:28], fill='white')
            draw.text((x + 8, y + 185), digest[:12], fill='#BBC6D5')
            for index, swatch in enumerate(texture['palette']):
                draw.rectangle((x + 8 + index * 28, y + 202, x + 34 + index * 28, y + 211), fill=swatch['hex'])
        relative = f'texture-sheets/textures-{offset // page_size + 1:02}.jpg'
        (output / relative).parent.mkdir(parents=True, exist_ok=True)
        sheet.save(output / relative, quality=90)
        sheets.append(relative)
    return sheets


def family_table(report, families):
    canonical = {entry['path']: entry for entry in report['occurrences'] if entry['canonical']}
    lines = ['| ID | Família (nome preservado) | Peças primárias | Polígonos BMD | Texturas |',
             '|---:|---|---|---:|---:|']
    for family in families['families']:
        entries = [canonical[piece['path']] for pieces in family['pieces'].values() for piece in pieces if piece['path'] in canonical]
        models = [report['models'][entry['sha256']] for entry in entries if report['models'][entry['sha256']]['status'] == 'ok']
        polygons = sum(model['polygon_records'] for model in models)
        textures = {texture.get('sha256') for entry in entries for texture in entry['textures'] if texture.get('sha256')}
        name = family['name'].replace('|', '\\|')
        if family.get('name_reliability') in ('client_cp932_inferred', 'unresolved_or_lossy_decode'):
            name += ' (nome requer revisão)'
        lines.append(f"| {family['item_number']} | {name} | {', '.join(family['present_parts'])} | {polygons} | {len(textures)} |")
    return lines


def write_report(report, families, output):
    summary = report['summary']
    lines = ['# Catálogo de referência dos equipamentos MU', '',
             'Inventário diagnóstico dos arquivos disponíveis; não representa inspeção dentro do jogo nem garante que todos os recursos sejam equipáveis.', '',
             '## Base e limites', '',
             f"- Base canônica: `{report['canonical_archive']}`.",
             '- Alternativas permanecem separadas por origem e SHA-256; o mesmo nome pode possuir malhas ou texturas diferentes.',
             '- Peças Class/Head/Face, esqueleto Player, ajudantes e consumíveis ficam identificados separadamente.',
             '- Paletas medem pixels opacos dos atlas originais (amostra até 128×128, seis cores). Não medem área visível por UV, iluminação, brilho Excellent ou efeitos do jogo.',
             '- As animações listadas são as que existem em cada BMD. Armaduras equipadas normalmente recebem os movimentos do Player.bmd; consulte o guia de execução.',
             '- Nomes vêm das definições Season 6 e de quatro Item.bmd da base canônica. A tentativa CP932 é uma inferência de codificação, não confirmação semântica; os bytes originais e a confiabilidade ficam explícitos no JSON. Nomes não resolvidos ou potencialmente corrompidos exigem revisão.', '',
             '## Cobertura', '', '| Métrica | Total |', '|---|---:|']
    lines.extend(f'| {METRIC_LABELS.get(key, key)} | {value} |' for key, value in summary.items() if isinstance(value, int))
    lines.extend(['', '### Categorias canônicas', '', '| Categoria | Modelos/arquivos |', '|---|---:|'])
    lines.extend(f'| {CATEGORY_LABELS.get(key, key)} | {value} |' for key, value in sorted(summary['canonical_categories'].items()))
    lines.extend(['', '### Origens', '', '| Origem | Canônica | BMD candidatos |', '|---|---|---:|'])
    for source in report['sources']:
        lines.append(f"| `{source['location']}!{source['prefix']}` | {'sim' if source['canonical'] else 'não'} | {source['candidates']} |")
    lines.extend(['', '## Famílias de armadura canônicas', '',
                  'A ausência de elmo ou luvas pode ser intencional para a classe. Nº53 é carregado pelo cliente, mas não possui definição/nome confirmado; não é rotulado como equipamento utilizável.', ''])
    lines.extend(family_table(report, families))
    lines.extend(['', '## Falhas e recursos não resolvidos', ''])
    failures = [model for model in report['models'].values() if model['status'] == 'error']
    lines.extend(f"- `{model['first_path']}`: {model['error']}" for model in failures)
    if not failures:
        lines.append('Nenhuma falha de leitura BMD no conjunto inventariado.')
    lines.extend(['', 'Referências de textura ausentes são registradas por ocorrência em `catalog.json`; podem ser recursos externos do renderer, placeholders ou arquivos realmente faltantes. Não são substituídas por texturas de outra origem.', '',
                  '## Arquivos do catálogo', '',
                  '- `catalog.json`: todas as ocorrências, modelos únicos, ossos, ações, materiais, texturas e paletas.',
                  '- `canonical-families.json`: caminhos originais no ZIP agrupados por família/peça para inspeção 3D.',
                  '- `item-names.json`: nomes e requisitos declarados nas fontes; não substitui configuração persistida do servidor.',
                  '- [Detalhes das armaduras](armor-details.md) e [demais equipamentos](equipment-details.md): métricas e materiais peça a peça.',
                  '- [Cobertura e pendências](coverage.md): referências externas e diferenças entre origens.',
                  '- `texture-sheets/`: pranchas diagnósticas dos atlas reais, sem recoloração.',
                  '- `texture-thumbnails/`: amostras diagnósticas deduplicadas por hash.',
                  '- `equipment-effects-index.json` e `runtime-guide.md`: índice de efeitos e funcionamento do renderer (levantamento complementar).', '',
                  '### Pranchas de texturas', ''])
    lines.extend(f'- [{Path(sheet).name}]({sheet})' for sheet in report['texture_sheets'])
    (output / 'catalog.md').write_text('\n'.join(lines) + '\n', encoding='utf-8')
