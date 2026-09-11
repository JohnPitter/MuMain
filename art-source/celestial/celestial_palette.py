"""Physical materials and construction roles shared by the entire Celestial set."""

MATERIALS = {
    'Gold': ((0.83, 0.49, 0.12), 0.78, 0.23),
    'Ivory': ((0.92, 0.9, 0.8), 0.38, 0.25),
    'Sapphire': ((0.035, 0.54, 0.95), 0.35, 0.18),
    'Emissive': ((1.0, 0.83, 0.43), 0.25, 0.22),
}

CONSTRUCTION_ROLES = {
    'Base': 'Gold',
    'Trim': 'Ivory',
    'Sapphire': 'Sapphire',
    'Emissive': 'Emissive',
}


def construction_palette(materials_by_name):
    return {role: materials_by_name[name] for role, name in CONSTRUCTION_ROLES.items()}
