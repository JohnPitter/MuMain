"""Physical materials and construction roles shared by the entire Celestial set."""

MATERIALS = {
    'Gold': ((0.95, 0.60, 0.10), 0.92, 0.16),
    'Ivory': ((0.40, 0.45, 0.50), 0.90, 0.20),
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
