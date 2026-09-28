"""Read-only Blender/source validation for the north-axis and HQ art update."""
import bpy
import json
import math
from pathlib import Path
from mathutils import Matrix, Vector

ROOT=Path(__file__).resolve().parents[2]
ART=ROOT/'Art'/'Maps'


def same(a,b,path='layout'):
    if isinstance(a,dict):
        assert a.keys()==b.keys(),path
        for key in a:same(a[key],b[key],path+'.'+key)
    elif isinstance(a,list):
        assert len(a)==len(b),path
        for i,(x,y) in enumerate(zip(a,b)):same(x,y,path+'.'+str(i))
    elif isinstance(a,(int,float)) and not isinstance(a,bool):
        assert abs(a-b)<1e-4,(path,a,b)
    else:assert a==b,(path,a,b)


base=json.loads((ART/'Round01_v2'/'Round01_Layout.json').read_text(encoding='utf-8'))
hq=json.loads((ART/'Round01_HQ'/'Round01_Layout.json').read_text(encoding='utf-8'))
groups=['camps','spawns','bushes','paths','walls','markers']
for key in groups:same(base[key],hq[key],key)
report=dict(layout_source='Round01_v2',compared_groups=groups,
            all_gameplay_layout_values_preserved=True,camps=len(hq['camps']),
            markers=len(hq['markers']),tree_instances=len(hq['instances']),
            blender_coordinate_frame=hq['coordinate_frame'],prototype_art_checks={})
rotation=Matrix.Rotation(-math.pi/2,4,'Z')
for variant in ('Round01','Round01_v2','Round01_HQ'):
    folder=ART/variant
    source=folder/('Round01_Emberwild_HQ.blend' if variant.endswith('HQ') else 'Round01_Emberwild.blend')
    backup=ROOT/'Saved'/'Round01_BeforeNorthAxis'/variant/'Round01_Emberwild.blend'
    old={}
    if backup.exists():
        bpy.ops.wm.open_mainfile(filepath=str(backup))
        bpy.context.view_layer.update()
        for obj in bpy.context.scene.objects:
            if obj.type=='MESH':
                old[obj.name]=(rotation@obj.matrix_world,len(obj.data.vertices),len(obj.data.polygons),
                               [m.name for m in obj.data.materials],[(v.co.x,v.co.y,v.co.z) for v in obj.data.vertices])
    bpy.ops.wm.open_mainfile(filepath=str(source))
    bpy.context.view_layer.update()
    scene=bpy.context.scene
    assert scene['round01_coordinate_frame']=='X_NORTH_Y_WEST'
    cam=bpy.data.objects['Camera_Top']
    q=cam.matrix_world.to_quaternion()
    up=q@Vector((0,1,0));right=q@Vector((1,0,0))
    assert up.x>.9999 and right.y<-.9999,(variant,up,right)
    meshes=[o for o in scene.objects if o.type=='MESH']
    if old:
        assert len(meshes)==len(old)
        for obj in meshes:
            matrix,nv,np,mats,verts=old[obj.name]
            assert max(abs(obj.matrix_world[i][j]-matrix[i][j]) for i in range(4) for j in range(4))<.001,obj.name
            assert (len(obj.data.vertices),len(obj.data.polygons),[m.name for m in obj.data.materials])==(nv,np,mats),obj.name
            assert all((v.co-Vector(p)).length<1e-6 for v,p in zip(obj.data.vertices,verts)),obj.name
        report['prototype_art_checks'][variant]=dict(mesh_objects=len(meshes),
            same_local_geometry_and_material_slots=True,only_world_rotation_changed=True)
    if variant.endswith('HQ'):
        images=[img for img in bpy.data.images if img.name.startswith('T_HQ_')]
        assert len(images)==18
        invalid=[(img.name,img.filepath) for img in images
                 if not img.filepath.replace('\\','/').startswith('//Textures/') or not Path(bpy.path.abspath(img.filepath)).is_file()]
        assert not invalid,invalid
        for name in ('Camera_HQ_Spawn','Camera_HQ_Camp'):
            assert bpy.data.objects[name].location.length>10
        report['textures_resolve_relative_to_blend']=True
        report['blender_top_camera_up']=list(up)
        report['blender_top_camera_right']=list(right)
(ART/'Round01_HQ'/'Layout_Preservation_Verification.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('ROUND01_SOURCE_VERIFIED',json.dumps(report),flush=True)
