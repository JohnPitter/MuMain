"""Provisional preview palette; texture atlas baking is deliberately not implemented."""
import bpy

# Azul Celeste (principal) e Branco Platinado (detalhes) do conceito; 'Emissive'
# carrega a energia de raio. Nomes = futuros atlas exigidos pelo loader.
PALETTE = {
    'Blue': ((0.035, 0.22, 0.62), 0.85, 0.26),
    'Platina': ((0.843, 0.827, 0.831), 0.92, 0.2),
    'Emissive': ((0.10, 0.45, 1.0), 0.25, 0.15),
}
EMISSIVE_STRENGTH = 0.35


def create_materials():
    result = {}
    for role, (color, metallic, roughness) in PALETTE.items():
        material = bpy.data.materials.new(f'Zeus_{role}.jpg')
        material.use_nodes = True
        material.diffuse_color = (*color, 1)
        shader = next(node for node in material.node_tree.nodes if node.type == 'BSDF_PRINCIPLED')
        shader.inputs['Base Color'].default_value = (*color, 1)
        shader.inputs['Metallic'].default_value = metallic
        shader.inputs['Roughness'].default_value = roughness
        if role == 'Emissive':
            shader.inputs['Emission Color'].default_value = (*color, 1)
            shader.inputs['Emission Strength'].default_value = EMISSIVE_STRENGTH
        material['atlas_status'] = 'PROVISIONAL: procedural Blender material only'
        result[role] = material
    return result
