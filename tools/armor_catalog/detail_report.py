"""Detailed per-family materials and source variants; no inferred visual styling."""
from collections import defaultdict


def appearance_variants(report):
    result = defaultdict(dict)
    for entry in report['occurrences']:
        texture_hashes = tuple(texture.get('sha256', 'unresolved:' + texture['declared']) for texture in entry['textures'])
        key = (entry['sha256'], texture_hashes)
        variant = result[entry['path']].setdefault(key, dict(model_sha256=entry['sha256'], texture_sha256=texture_hashes,
                                                          source_ids=[], canonical=False))
        variant['source_ids'].append(entry['source_id'])
        variant['canonical'] |= entry['canonical']
    return {path: list(variants.values()) for path, variants in sorted(result.items()) if len(variants) > 1}


def material_cell(report, occurrence):
    parts = []
    for binding in occurrence['textures']:
        name = binding['declared'].replace('|', '\\|')
        if binding['runtime_semantics']['hidden_sentinel']:
            parts.append(f'`{name}` (sentinela: malha oculta)')
            continue
        if binding['status'] != 'ok':
            parts.append(f'`{name}` (externa/não resolvida)')
            continue
        texture = report['textures'][binding['sha256']]
        palette = ', '.join(swatch['hex'] for swatch in texture['palette'][:3])
        parts.append(f'`{name}` [{palette}]')
    return '<br>'.join(parts) or 'sem material'


def mesh_row(report, occurrence, label):
    model = report['models'][occurrence['sha256']]
    if model['status'] != 'ok':
        return f"| {label} | `{occurrence['path']}` | — | — | {model['status']} | — |"
    geometry = f"{model['mesh_count']} / {model['vertices']} / {model['polygon_records']}"
    actions = ','.join(str(action['frames']) for action in model['actions'])
    if len(actions) > 80:
        actions = f"{len(model['actions'])} clipes; consulte JSON"
    return f"| {label} | `{occurrence['path']}` | {geometry} | {model['bone_count']} | {actions or '0'} | {material_cell(report, occurrence)} |"


def armor_details(report, families, output):
    canonical = {item['path']: item for item in report['occurrences'] if item['canonical']}
    lines = ['# Armaduras: malhas e materiais por família', '',
             'Métricas nativas e atlas originais. Cores indicadas são as três mais frequentes do atlas, não uma avaliação do visual equipado.', '']
    for family in families['families']:
        lines.extend([f"## {family['item_number']} — {family['name']}", '',
            'Confiabilidade do nome: `' + family.get('name_reliability', 'unknown') + '`; CP932 inferido ou decodificação com perdas requer revisão semântica.', '',
            '| Peça | BMD no ZIP canônico | Malhas/vértices/polígonos | Ossos | Frames por ação local | Materiais e paleta |',
            '|---|---|---:|---:|---|---|'])
        for part, pieces in family['pieces'].items():
            lines.extend(mesh_row(report, canonical[piece['path']], part) for piece in pieces)
        if family['variants']:
            lines.extend(['', 'Variantes auxiliares/presentes fora do caminho primário:', ''])
            lines.extend(f"- `{piece['path']}` — `{piece['variant']}`." for piece in family['variants'])
        if family['missing_parts']:
            lines.extend(['', 'Peças ausentes nesta família: ' + ', '.join(family['missing_parts']) + '.'])
        lines.append('')
    (output / 'armor-details.md').write_text('\n'.join(lines), encoding='utf-8')


def equipment_details(report, output):
    groups = defaultdict(list)
    for occurrence in report['occurrences']:
        if not occurrence['canonical'] or not occurrence['path'].startswith('data/item/'):
            continue
        mapping = report['mappings'][occurrence['mapping_ref']]
        groups[mapping['category']].append((occurrence, mapping))
    lines = ['# Armas, escudos, asas, acessórios e demais recursos', '',
             'A classificação usa vínculos literais do carregador e prefixos de loops conhecidos; `unmapped` não recebe identidade inventada. Nomes correspondem à base canônica, não às variantes antigas.', '']
    for category, items in sorted(groups.items()):
        lines.extend([f'## {category}', '',
            '| Identidade/nome | BMD | Malhas/vértices/polígonos | Ossos | Frames por ação local | Materiais e paleta |',
            '|---|---|---:|---:|---|---|'])
        for occurrence, mapping in items:
            label = f"{mapping.get('identity', 'não resolvido')} — {mapping.get('name', '')}".replace('|', '\\|')
            lines.append(mesh_row(report, occurrence, label))
        lines.append('')
    (output / 'equipment-details.md').write_text('\n'.join(lines), encoding='utf-8')


def unresolved_report(report, output):
    lines = ['# Cobertura e referências pendentes', '',
             'Nenhuma textura é buscada em outra instalação. Candidatos externos abaixo existem na mesma origem, mas só o carregador/renderer confirma qual diretório usar.', '',
             'Casos comprovados: `hide_m.jpg` usa o prefixo `hid`, convertido em `BITMAP_HIDE` pelo carregador; o renderer não desenha essa malha. O arquivo físico não é exigido. O elmo Lucky70 usa `head helmet Luck 40.jpg`, encontrado em Lucky65; o cliente oferece reutilização por nome de textura já carregada (`LoadData.cpp:108`), e carrega Lucky65 antes de Lucky70. A referência continua marcada como externa ao diretório local, com os candidatos preservados abaixo.', '',
             '## Materiais canônicos fora do diretório do modelo', '', '| BMD | Material | Candidatos na mesma base |', '|---|---|---|']
    for occurrence in report['occurrences']:
        if not occurrence['canonical']:
            continue
        for texture in occurrence['textures']:
            if texture['status'] == 'missing':
                candidates = '<br>'.join(texture['elsewhere_same_source_candidates']) or 'nenhum'
                lines.append(f"| `{occurrence['path']}` | `{texture['declared']}` | {candidates} |")
    lines.extend(['', '## Mesmo caminho, arquivos de componentes diferentes entre origens', '',
                  'Agrupamento pelo SHA bruto da malha e lista ordenada de arquivos de textura; diferenças de wrapper/compactação podem representar os mesmos pixels. Isso não prova aparência visual diferente. Uma textura não resolvida é preservada como tal, sem supor equivalência.', '',
                  '| Caminho | Variantes | SHA das malhas |', '|---|---:|---|'])
    for path, variants in report['appearance_variants'].items():
        hashes = ', '.join(sorted({variant['model_sha256'][:16] for variant in variants}))
        lines.append(f'| `{path}` | {len(variants)} | {hashes} |')
    lines.extend(['', '## Arquivos/arquivos compactados fora do levantamento 3D', '', '| Caminho | Motivo |', '|---|---|'])
    for entry in report['discovery_audit']:
        if entry['status'] != 'indexed':
            lines.append(f"| `{entry['path']}` | {entry['status']} |")
    (output / 'coverage.md').write_text('\n'.join(lines) + '\n', encoding='utf-8')


def write_details(report, families, output):
    armor_details(report, families, output)
    equipment_details(report, output)
    unresolved_report(report, output)
