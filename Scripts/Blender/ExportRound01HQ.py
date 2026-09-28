"""Re-export the saved HQ source without rebuilding procedural art or rendering.

Blender meters are baked into FBX object transforms (FBX header: centimeters).
This avoids depending on an Unreal reimporter's cached scene-unit setting.
"""
import bpy
import json
from pathlib import Path

OUT=Path(__file__).resolve().parents[2]/'Art'/'Maps'/'Round01_HQ'
bpy.ops.wm.open_mainfile(filepath=str(OUT/'Round01_Emberwild_HQ.blend'))
scene=bpy.context.scene
layout=json.loads((OUT/'Round01_Layout.json').read_text(encoding='utf-8'))
assert scene.unit_settings.scale_length==1
for entry in layout['exports']:
    name=entry['asset']
    if entry['role']=='tree_prototype':originals=[bpy.data.objects[name]]
    else:
        suffix=name.removeprefix('SM_R01HQ_')
        collection=next(c for c in bpy.data.collections if c.name.split('_',1)[-1]==suffix)
        originals=list(collection.objects)
    bpy.ops.object.select_all(action='DESELECT')
    copies=[]
    for source in originals:
        if source.type!='MESH':continue
        obj=source.copy();obj.data=source.data.copy();scene.collection.objects.link(obj)
        obj.hide_viewport=False;obj.hide_render=False;obj.select_set(True);copies.append(obj)
    bpy.context.view_layer.objects.active=copies[0]
    bpy.ops.object.convert(target='MESH');bpy.ops.object.join()
    obj=bpy.context.object;obj.name=name
    scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
    bpy.ops.export_scene.fbx(filepath=str(OUT/'Exports'/entry['file']),use_selection=True,
        apply_unit_scale=True,apply_scale_options='FBX_SCALE_NONE',axis_forward='-Y',axis_up='Z',
        object_types={'MESH'},mesh_smooth_type='FACE',use_triangles=True,bake_anim=False,add_leaf_bones=False)
    bpy.data.objects.remove(obj,do_unlink=True)
    print('HQ_CENTIMETER_EXPORT',name,flush=True)
