"""Procedural preview palette; texture atlas baking is deliberately not implemented."""
import bpy

PALETTE = {
    'Black': ((0.023, 0.032, 0.044), 0.88, 0.23),
    'Gold': ((0.72, 0.40, 0.105), 0.84, 0.24),
    'Blue': ((0.009, 0.21, 0.48), 0.42, 0.18),
}


def create_materials():
    result = {}
    for role, (color, metallic, roughness) in PALETTE.items():
        material = bpy.data.materials.new(f'Poseidon_{role}.jpg')
        material.use_nodes = True
        material.diffuse_color = (*color, 1)
        shader = next(node for node in material.node_tree.nodes if node.type == 'BSDF_PRINCIPLED')
        shader.inputs['Base Color'].default_value = (*color, 1)
        shader.inputs['Metallic'].default_value = metallic
        shader.inputs['Roughness'].default_value = roughness
        if role == 'Blue':
            shader.inputs['Emission Color'].default_value = (*color, 1)
            shader.inputs['Emission Strength'].default_value = 0.18
        material['atlas_status'] = 'PROVISIONAL: procedural Blender material only'
        result[role] = material
    return result
