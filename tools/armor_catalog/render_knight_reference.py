"""Blender diagnostic views of native meshes, without MU runtime glow or asset edits."""
import argparse
import json
from pathlib import Path
import sys
from tempfile import TemporaryDirectory
from zipfile import ZipFile

import bpy

sys.path.insert(0, str(Path(__file__).resolve().parent))
from render_native_families import ArchiveAssets, create_mesh, frame_family, setup_scene, world_matrices

MODELS = ('Data/Monster/Monster150.bmd', 'Data/Monster/Monster209.bmd')
VIEWS = (('texture', 'front'), ('texture', 'rear'), ('clay', 'front'), ('wire', 'front'))


def diagnostic_material(mode, image):
    material = bpy.data.materials.new(mode)
    material.use_nodes = True
    nodes = material.node_tree.nodes
    shader = next(node for node in nodes if node.type == 'BSDF_PRINCIPLED')
    shader.inputs['Base Color'].default_value = (.28, .28, .28, 1)
    shader.inputs['Roughness'].default_value = .8
    links = material.node_tree.links
    if mode == 'texture':
        texture = nodes.new('ShaderNodeTexImage')
        texture.image = image
        emission = nodes.new('ShaderNodeEmission')
        output = next(node for node in nodes if node.type == 'OUTPUT_MATERIAL')
        links.new(texture.outputs['Color'], emission.inputs['Color'])
        links.new(emission.outputs['Emission'], output.inputs['Surface'])
    elif mode == 'wire':
        wire = nodes.new('ShaderNodeWireframe')
        wire.inputs['Size'].default_value = .18
        mix = nodes.new('ShaderNodeMixRGB')
        mix.inputs[1].default_value = (.4, .4, .4, 1)
        mix.inputs[2].default_value = (.015, .015, .015, 1)
        links.new(wire.outputs['Fac'], mix.inputs[0])
        links.new(mix.outputs[0], shader.inputs['Base Color'])
    return material


def render_model(entry, assets, scene, output):
    model = assets.model(entry)
    bind = world_matrices(model, 0)
    objects = [create_mesh(mesh, None, bind, index) for index, mesh in enumerate(model['meshes'])]
    views = []
    for mode, view in VIEWS:
        for obj, mesh in zip(objects, model['meshes']):
            image = assets.texture(mesh['texture'], 'Data/Monster')
            if image is None:
                raise ValueError(f"Missing native texture {mesh['texture']}")
            obj.data.materials.clear()
            obj.data.materials.append(diagnostic_material(mode, image))
        frame_family(scene, objects, view)
        destination = output / f'{Path(entry).stem.lower()}-{mode}-{view}.png'
        scene.render.filepath = str(destination)
        bpy.ops.render.render(write_still=True)
        views.append(destination.name)
    for obj in objects:
        bpy.data.objects.remove(obj, do_unlink=True)
    return dict(entry=entry, sha256=model['sha256'], action=0, frame=0, views=views)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args(sys.argv[sys.argv.index('--') + 1:])
    args.archive = args.archive.resolve()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    scene = setup_scene()
    scene.render.resolution_x, scene.render.resolution_y = 512, 640
    with ZipFile(args.archive) as archive, TemporaryDirectory(prefix='mu-knight-diagnostic-') as temporary:
        assets = ArchiveAssets(archive, temporary)
        records = [render_model(entry, assets, scene, args.output) for entry in MODELS]
    report = dict(models=records, source_assets_written=False, native_normals_preserved=True,
                  mu_runtime_passes_reproduced=False,
                  note='Textura sem luz mostra a pintura original; clay/wire usam luz neutra. Não são capturas do jogo.')
    (args.output / 'render-report.json').write_text(json.dumps(report, indent=2), encoding='utf-8')


if __name__ == '__main__':
    main()
