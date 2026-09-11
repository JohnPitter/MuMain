"""Procedural preview palette for the cape; atlas baking is deliberately absent.

Reuses the armor roles (poseidon_materials.PALETTE) verbatim so the family
reads as one set, and adds the concept sheet's fourth swatch, Branco Perolado,
as a cape-specific accent. Every role names a future atlas that does not exist
yet; nothing here may reach a client.
"""
import bpy

from poseidon_materials import PALETTE

PEARL_COLOR = (0.824, 0.796, 0.812)  # concept swatch #CCCBD2 (linear approximation)


def create_cape_materials():
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
    pearl = bpy.data.materials.new('Poseidon_Pearl.jpg')
    pearl.use_nodes = True
    pearl.diffuse_color = (*PEARL_COLOR, 1)
    shader = next(node for node in pearl.node_tree.nodes if node.type == 'BSDF_PRINCIPLED')
    shader.inputs['Base Color'].default_value = (*PEARL_COLOR, 1)
    shader.inputs['Metallic'].default_value = 0.35
    shader.inputs['Roughness'].default_value = 0.28
    shader.inputs['Emission Color'].default_value = (*PEARL_COLOR, 1)
    shader.inputs['Emission Strength'].default_value = 0.06
    pearl['atlas_status'] = 'PROVISIONAL: procedural Blender material only'
    result['Pearl'] = pearl
    return result
