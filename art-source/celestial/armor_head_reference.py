"""Preview the existing Soul Master face; never include it in equipment exports."""
from pathlib import Path
from tempfile import NamedTemporaryFile

import bpy

from import_native_reference import create_mesh
from inspect_bmd_rig import inspect


def native_material(directory, texture):
    source = directory / Path(texture).with_suffix('.OZJ')
    data = source.read_bytes()[24:]
    if not data.startswith(b'\xff\xd8'):
        raise ValueError(f'Invalid native JPEG payload: {source}')
    with NamedTemporaryFile(suffix='.jpg', delete=False) as temporary:
        temporary.write(data)
        path = Path(temporary.name)
    try:
        image = bpy.data.images.load(str(path))
        image.name = 'REFERENCE_' + texture
        image.pack()
    finally:
        path.unlink()
    material = bpy.data.materials.new('REFERENCE native Soul Master face')
    material.use_nodes = True
    shader = next(node for node in material.node_tree.nodes if node.type == 'BSDF_PRINCIPLED')
    shader.inputs['Roughness'].default_value = .8
    node = material.node_tree.nodes.new('ShaderNodeTexImage')
    node.image = image
    material.node_tree.links.new(node.outputs['Color'], shader.inputs['Base Color'])
    return material


def add_head(directory, rig, bind):
    model = inspect(directory / 'HelmClass201.bmd', True)
    group = bpy.data.collections.new('REFERENCE ONLY / native Soul Master head')
    bpy.context.scene.collection.children.link(group)
    objects = []
    for index, source in enumerate(model['meshes']):
        obj = create_mesh(source, rig, bind, index)
        obj.name = f'REFERENCE native Soul Master head {index}'
        obj['exclude_from_equipment_export'] = True
        obj.data.materials.append(native_material(directory, source['texture']))
        for face in obj.data.polygons:
            face.use_smooth = True
        for old in list(obj.users_collection):
            old.objects.unlink(obj)
        group.objects.link(obj)
        objects.append(obj)
    return objects
