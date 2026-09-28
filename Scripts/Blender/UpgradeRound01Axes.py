"""Rotate existing round-one Blender sources/FBX without changing their design.
Run after generating the HQ source, or independently. Idempotent.
"""
import bpy
import json
import sys
import shutil
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from Round01Coordinates import ensure_north,FRAME

ROOT=Path(__file__).resolve().parents[2]
BACKUP=ROOT/'Saved'/'Round01_BeforeNorthAxis'
BACKUP.mkdir(parents=True,exist_ok=True)

for folder in ('Round01','Round01_v2'):
    out=ROOT/'Art'/'Maps'/folder
    source=out/'Round01_Emberwild.blend'
    metadata=out/'Round01_Layout.json'
    layout=json.loads(metadata.read_text(encoding='utf-8'))
    if layout.get('coordinate_frame')==FRAME and '--reexport' not in sys.argv:
        print('NORTH_ALREADY_SET',folder,flush=True)
        continue
    for file in (source,metadata):
        dest=BACKUP/folder/file.name
        dest.parent.mkdir(parents=True,exist_ok=True)
        if not dest.exists(): shutil.copy2(file,dest)
    bpy.ops.wm.open_mainfile(filepath=str(source))
    scene=bpy.context.scene
    ensure_north(scene,layout)
    bpy.context.view_layer.update()
    for export in layout['exports']:
        cname=export['asset'].removeprefix('SM_R01_')
        collection=next(c for c in bpy.data.collections if c.name.split('_',1)[-1]==cname)
        bpy.ops.object.select_all(action='DESELECT')
        copies=[]
        for original in collection.objects:
            if original.type!='MESH': continue
            obj=original.copy();obj.data=original.data.copy();scene.collection.objects.link(obj)
            obj.select_set(True);copies.append(obj)
        bpy.context.view_layer.objects.active=copies[0]
        bpy.ops.object.join()
        obj=bpy.context.object;obj.name=export['asset']
        scene.cursor.location=(0,0,0)
        bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
        bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
        uv=obj.data.uv_layers.active or obj.data.uv_layers.new(name='UVMap')
        for polygon in obj.data.polygons:
            axis=max(range(3),key=lambda k:abs(polygon.normal[k])); axes=[k for k in range(3) if k!=axis]
            for li in polygon.loop_indices:
                p=obj.data.vertices[obj.data.loops[li].vertex_index].co
                uv.data[li].uv=(p[axes[0]]/4,p[axes[1]]/4)
        bpy.ops.export_scene.fbx(filepath=str(out/'Exports'/export['file']),use_selection=True,
            apply_unit_scale=True,apply_scale_options='FBX_SCALE_NONE',axis_forward='-Y',axis_up='Z',
            object_types={'MESH'},mesh_smooth_type='FACE',use_triangles=True,bake_anim=False,add_leaf_bones=False)
        bpy.data.objects.remove(obj,do_unlink=True)
    scene.camera=bpy.data.objects['Camera_Overview']
    for screen in bpy.data.screens:
        for area in screen.areas:
            if area.type=='VIEW_3D':
                area.spaces.active.region_3d.view_rotation=bpy.data.objects['Camera_Top'].rotation_euler.to_quaternion()
                area.spaces.active.region_3d.view_perspective='ORTHO'
                area.spaces.active.region_3d.view_distance=132
    bpy.context.preferences.filepaths.save_version=0
    bpy.ops.wm.save_as_mainfile(filepath=str(source))
    metadata.write_text(json.dumps(layout,ensure_ascii=False,indent=2),encoding='utf-8')
    print('NORTH_UPDATED',folder,flush=True)
