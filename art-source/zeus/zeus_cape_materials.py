"""Provisional preview palette for the cape; atlas baking is deliberately absent.

Reuses the Zeus family roles (zeus_materials.PALETTE) verbatim so the cape
reads as one set with the armor and wings: Azul Celeste principal, Branco
Platinado nos detalhes and the emissive storm channels. Every role names a
future atlas that does not exist yet; nothing here may reach a client.
"""
import bpy

from zeus_materials import PALETTE, EMISSIVE_STRENGTH


def create_cape_materials():
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
