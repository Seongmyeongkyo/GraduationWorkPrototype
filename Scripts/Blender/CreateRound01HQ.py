"""Detailed Emberwild art study, preserving the v2 gameplay layout. No external assets.

blender --background --factory-startup --threads 4 --python-exit-code 1 --python <this file>
Use -- --no-render to export without previews, --render-only to render the saved source.
"""
import bpy
import json
import math
import random
import sys
import copy
from pathlib import Path
from mathutils import Vector
from mathutils.bvhtree import BVHTree
sys.path.insert(0,str(Path(__file__).resolve().parent))
from Round01Coordinates import ensure_north,rotate_scene,rotate_layout,FRAME
from EmberwildDetail import textures,tree_mesh,fern_mesh,grass_mesh
from ArenaPrototypeGeometry import box,cylinder,ring,beam,camera

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'Art'/'Maps'/'Round01_HQ'
SOURCE=ROOT/'Art'/'Maps'/'Round01_v2'
for folder in (OUT,OUT/'Exports',OUT/'Textures',OUT/'Previews'):folder.mkdir(parents=True,exist_ok=True)
RNG=random.Random(290926)


def collection(name):
    result=bpy.data.collections.new(name);bpy.context.scene.collection.children.link(result)
    return result


def assign_projection(obj,scale=3):
    if obj.type!='MESH' or obj.data.uv_layers: return
    uv=obj.data.uv_layers.new(name='UVMap')
    for face in obj.data.polygons:
        axis=max(range(3),key=lambda i:abs(face.normal[i]));axes=[i for i in range(3) if i!=axis]
        for li in face.loop_indices:
            co=obj.matrix_world@obj.data.vertices[obj.data.loops[li].vertex_index].co
            uv.data[li].uv=(co[axes[0]]/scale,co[axes[1]]/scale)


def render():
    scene=bpy.context.scene
    scene.render.engine='CYCLES';scene.cycles.samples=40;scene.cycles.use_denoising=True
    scene.render.threads_mode='FIXED';scene.render.threads=4
    for name,cam,w,h in [('Spawn','Camera_HQ_Spawn',1600,1200),('Overview','Camera_Overview',1800,1550),
                         ('TopDown','Camera_Top',1800,1800),('Camp','Camera_HQ_Camp',1600,1200)]:
        if '--detail-only' in sys.argv and name not in ('Spawn','Camp'):continue
        scene.camera=bpy.data.objects[cam];scene.render.resolution_x=w;scene.render.resolution_y=h
        scene.render.resolution_percentage=100;scene.render.filepath=str(OUT/'Previews'/('Round01_HQ_'+name+'.png'))
        bpy.ops.render.render(write_still=True)
        print('HQ_RENDER_DONE',name,flush=True)


if '--render-only' in sys.argv:
    bpy.ops.wm.open_mainfile(filepath=str(OUT/'Round01_Emberwild_HQ.blend'));render();sys.exit(0)

bpy.ops.wm.open_mainfile(filepath=str(SOURCE/'Round01_Emberwild.blend'))
scene=bpy.context.scene
layout=json.loads((SOURCE/'Round01_Layout.json').read_text(encoding='utf-8'))
if scene.get('round01_coordinate_frame')==FRAME:
    rotate_scene(scene,True);rotate_layout(layout,True);scene['round01_coordinate_frame']='DESIGN_X_EAST_Y_NORTH'
bpy.context.view_layer.update()
reference=copy.deepcopy(layout)
ground=bpy.data.collections['01_Ground_and_Trails'];walls=bpy.data.collections['02_Cliffs_and_Boundary']
props=bpy.data.collections['03_Camps_and_Ruins'];trees=bpy.data.collections['04_Trees']
bushes=bpy.data.collections['05_Bushes_NoCollision'];fire=bpy.data.collections['06_Fire_Visual_Placeholders']
presentation=bpy.data.collections['08_Presentation_Only']
rocks=collection('09_Weathered_Rock_Detail');understory=collection('10_Understory')
library=collection('11_Reusable_Foliage_Library');library.hide_render=True;library.hide_viewport=True
print('HQ_BUILD_TEXTURES',flush=True)
TEX=textures(OUT/'Textures')
MATMETA={}


def material(name,kind=None,tint=(1,1,1),vertex=False,foliage=False,emission=0):
    m=bpy.data.materials.new('M_HQ_'+name);m.use_nodes=True;m.diffuse_color=(*tint,1)
    nodes=m.node_tree.nodes;links=m.node_tree.links;bs=nodes.get('Principled BSDF')
    bs.inputs['Roughness'].default_value=.87;bs.inputs['Emission Strength'].default_value=emission
    if emission:
        bs.inputs['Base Color'].default_value=(*tint,1);bs.inputs['Emission Color'].default_value=(*tint,1)
    if kind:
        images={}
        for channel,image in TEX[kind].items():
            n=nodes.new('ShaderNodeTexImage');n.image=image;n.interpolation='Linear';n.extension='REPEAT';images[channel]=n
        rgb=images['BaseColor'].outputs['Color']
        multiply=nodes.new('ShaderNodeMixRGB');multiply.blend_type='MULTIPLY';multiply.inputs[0].default_value=1
        links.new(rgb,multiply.inputs[1]);multiply.inputs[2].default_value=(*tint,1)
        rgb=multiply.outputs[0]
        if vertex:
            vc=nodes.new('ShaderNodeVertexColor');vc.layer_name='Color'
            multi=nodes.new('ShaderNodeMixRGB');multi.blend_type='MULTIPLY';multi.inputs[0].default_value=1
            links.new(rgb,multi.inputs[1]);links.new(vc.outputs['Color'],multi.inputs[2]);rgb=multi.outputs[0]
        links.new(rgb,bs.inputs['Base Color'])
        normal=nodes.new('ShaderNodeNormalMap');normal.inputs['Strength'].default_value=.65
        links.new(images['Normal'].outputs['Color'],normal.inputs['Color']);links.new(normal.outputs['Normal'],bs.inputs['Normal'])
        links.new(images['Roughness'].outputs['Color'],bs.inputs['Roughness'])
    elif not emission:bs.inputs['Base Color'].default_value=(*tint,1)
    if foliage:
        bs.inputs['Subsurface Weight'].default_value=.055
        bs.inputs['Subsurface Radius'].default_value=(.5,.8,.25)
    MATMETA[m.name]=dict(rgb=list(tint),roughness=.87,emission=emission,texture_set=kind,
                        vertex_color=vertex,foliage=foliage,two_sided=True,normal_strength=.65)
    return m


bark=material('Bark','Bark',vertex=True)
leaf=material('Living_Leaves','Leaf',vertex=True,foliage=True)
stone=material('Weathered_Granite','Rock')
cutstone=material('Worn_Limestone','Rock',(1.5,1.48,1.3))
moss=material('Moss','Moss')
iron=material('Forged_Iron',None,(.045,.055,.055))
mapping={}
for old in list(bpy.data.materials):
    if old.name.startswith('M_HQ_'):continue
    name=old.name.removeprefix('M_')
    if name=='Terrain_Vertex':new=material('Forest_Floor','Ground',vertex=True)
    elif name.startswith('Bush') or name.startswith('Canopy'):new=leaf
    elif name=='Bark' or name=='Wood_End':new=bark
    elif name=='Canvas':new=material('Woven_Canvas','Canvas')
    elif name=='Moss' or name.startswith('Grass'):new=moss
    elif name.startswith('Cliff') or name=='Earth_Side':new=stone
    elif name=='Ruin_Limestone':new=cutstone
    elif name in ('Embers','Flame'):new=material(name,None,tuple(old.diffuse_color[:3]),emission=2 if name=='Embers' else 3)
    elif name.startswith('Team'):new=material(name,'Canvas',(.22,.68,1.1) if 'Blue' in name else (1.2,.30,.16))
    elif name=='Brass':new=material(name,None,(.40,.27,.08))
    else:new=material(name,'Ground',tuple(v*.8 for v in old.diffuse_color[:3]))
    mapping[old.name]=new
for obj in list(scene.objects):
    if obj.type!='MESH':continue
    for i,m in enumerate(obj.data.materials):
        if m and m.name in mapping:obj.data.materials[i]=mapping[m.name]
    assign_projection(obj)
terrain=bpy.data.objects['SM_R01_Ground_112m']
colors=terrain.data.color_attributes['Color']
for i,vertex in enumerate(terrain.data.vertices):
    old=colors.data[i].color
    blend=max(0,min(1,(old[0]-.13)/.33))
    turf=(.19,.255,.13);dirt=(.42,.34,.23)
    rgb=[a*(1-blend)+b*blend for a,b in zip(turf,dirt)]
    if math.hypot(vertex.co.x,vertex.co.y)<17.7:rgb=(.49,.435,.33)
    colors.data[i].color=(*rgb,1)

# Bevels break the razor-sharp edges of the blockout without moving its reserved paths.
for obj in list(walls.objects)+list(props.objects):
    if obj.type!='MESH':continue
    if ('ForestWall' in obj.name or any(t in obj.name for t in ('Relic','Ruin','Upright','Footing'))):
        modifier=obj.modifiers.new('Weathered_Edge','BEVEL');modifier.width=.11;modifier.segments=3
        modifier.limit_method='ANGLE'
    if any(t in obj.name for t in ('FireStone','BackRock','Outcrop')):
        # Keep the silhouette irregular, but give close-up stones rounded facets.
        modifier=obj.modifiers.new('Stone_Soften','SUBSURF');modifier.levels=1;modifier.render_levels=1
        for poly in obj.data.polygons:poly.use_smooth=True
    if obj.name.endswith('_Cloth') or 'CanvasShelter' in obj.name:
        modifier=obj.modifiers.new('Fabric_Thickness','SOLIDIFY');modifier.thickness=.025

# Layered rock faces sit against the original cliff perimeter, within its reserved clearance.
for wall in layout['walls']:
    if 'polygon_m' not in wall:continue
    poly=wall['polygon_m']
    for edge,(a,b) in enumerate(zip(poly,poly[1:]+poly[:1])):
        direction=Vector((b[0]-a[0],b[1]-a[1],0));length=direction.length
        if length<1.4:continue
        direction.normalize();inside=Vector((-direction.y,direction.x,0))
        for j in range(max(1,int(length/2))):
            t=(j+.5)/max(1,int(length/2));center=Vector((a[0]+(b[0]-a[0])*t,a[1]+(b[1]-a[1])*t,1.15))+inside*.50
            bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2,radius=1,location=center)
            obj=bpy.context.object;obj.name=wall['name']+f'_RockFace_{edge}_{j}'
            for c in list(obj.users_collection):c.objects.unlink(obj)
            rocks.objects.link(obj);obj.data.materials.append(stone)
            obj.scale=(min(length/2,.85)*RNG.uniform(.8,1.1),.52,RNG.uniform(.85,1.55))
            obj.rotation_euler.z=math.atan2(direction.y,direction.x)
            for v in obj.data.vertices:v.co*=RNG.uniform(.91,1.07)
            bevel=obj.modifiers.new('Eroded_Facets','BEVEL');bevel.width=.04;bevel.segments=2
            assign_projection(obj,2)

print('HQ_BUILD_FOLIAGE',flush=True)
roots=[]
for obj in trees.objects:
    if not obj.name.endswith('_Trunk'):continue
    roots.append(dict(name=obj.name.removesuffix('_Trunk'),position=[obj.location.x,obj.location.y,obj.location.z-obj.scale.z/2],
                      scale=obj.scale.x/.22,pine=any(o.name.startswith(obj.name.removesuffix('_Trunk')+'_Needles') for o in trees.objects)))
for obj in list(trees.objects):bpy.data.objects.remove(obj,do_unlink=True)
templates=[]
for i in range(6):
    obj=tree_mesh('SM_HQ_Tree_'+str(i+1),library,[bark,leaf],810+i,i>=4)
    obj['library_prototype']=True;templates.append(obj)
tree_instances=[]
for i,root in enumerate(roots):
    proto=templates[(4+i%2) if root['pine'] else i%4]
    obj=bpy.data.objects.new(root['name']+'_Detailed',proto.data);trees.objects.link(obj)
    obj.location=root['position'];obj.scale=(root['scale'],)*3;obj.rotation_euler.z=RNG.random()*math.tau
    tree_instances.append((obj,proto.name))
fern=fern_mesh('SM_HQ_Fern',library,[bark,leaf]);fern['library_prototype']=True
grass=grass_mesh('SM_HQ_Grass',library,[bark,leaf]);grass['library_prototype']=True

segments=[(Vector(a),Vector(b),p['width_m']/2) for p in layout['paths'] for a,b in zip(p['points'],p['points'][1:])]
clearings=[(Vector((0,0)),18)]+[(Vector(c['xy']),c['r']) for c in layout['camps']+layout['spawns']]
def clearance(p):
    v=Vector(p)
    vals=[(v-c).length-r for c,r in clearings]
    for a,b,r in segments:
        delta=b-a;t=max(0,min(1,(v-a).dot(delta)/delta.length_squared))
        vals.append((v-(a+delta*t)).length-r)
    return min(vals)

for i in range(4400):
    x,y=RNG.uniform(-51,51),RNG.uniform(-51,51);distance=clearance((x,y))
    if not -.25<distance<.85:continue
    proto=fern if i%7==0 else grass
    obj=bpy.data.objects.new('PathEdge_Plant_'+str(i),proto.data);understory.objects.link(obj)
    obj.location=(x,y,.015);size=RNG.uniform(.7,1.2);obj.scale=(size,)*3;obj.rotation_euler.z=RNG.random()*math.tau
# Bush concealment volumes keep their sixteen original footprints. Add fern layers inside them.
for zone in layout['bushes']:
    x,y=zone['center_m'];rx,ry=zone['radii_m'];a=math.radians(zone['rotation_degrees'])
    for i in range(24):
        t=RNG.random()*math.tau;r=math.sqrt(RNG.random())*.8
        dx,dy=math.cos(t)*r*rx,math.sin(t)*r*ry
        obj=bpy.data.objects.new(zone['name']+'_Fern_'+str(i),fern.data);understory.objects.link(obj)
        obj.location=(x+dx*math.cos(a)-dy*math.sin(a),y+dx*math.sin(a)+dy*math.cos(a),.02)
        size=RNG.uniform(1.2,1.65);obj.scale=(size,)*3;obj.rotation_euler.z=RNG.random()*math.tau

# Additional readable camp details: barrel hoops, tent ropes and loose firewood.
for obj in list(props.objects):
    if '_Barrel' in obj.name:
        for z in (.18,.71):
            ring(obj.name+'_IronHoop_'+str(z),obj.location[:2],.345,.375,z,props,iron,40)
for s in layout['spawns']:
    x,y=s['xy'];sign=1 if s['team']=='A' else -1
    tx,ty=x-sign*3.1,y-sign*3.4
    for dx in (-1.35,1.35):
        for dy in (-1.25,1.25):
            beam(s['name']+'_TentRope',(tx+dx,ty,1.78),(tx+dx*1.20,ty+dy,.08),.018,props,bark)
    for j in range(5):
        beam(s['name']+'_Firewood_'+str(j),(x+sign*3.8,y-sign*3+j*.13,.15),
             (x+sign*4.7,y-sign*3+j*.13,.18),.10,props,bark,10)
for obj in props.objects:assign_projection(obj)

# Reference geometry checks happen before coordinate conversion, against the actual evaluated walls.
bpy.context.view_layer.update();dep=bpy.context.evaluated_depsgraph_get();vs=[];faces=[]
for obj in walls.objects:
    evaluated=obj.evaluated_get(dep);data=evaluated.to_mesh();start=len(vs)
    vs.extend(obj.matrix_world@v.co for v in data.vertices)
    faces.extend(tuple(start+i for i in p.vertices) for p in data.polygons);evaluated.to_mesh_clear()
bvh=BVHTree.FromPolygons(vs,faces);errors=[];samples=[]
for p in layout['paths']:
    for a,b in zip(p['points'],p['points'][1:]):
        steps=math.ceil(math.dist(a,b))
        for i in range(steps+1):samples.append([a[k]+(b[k]-a[k])*i/steps for k in range(2)])
samples += [m['position_m'][:2] for m in layout['markers'] if m['kind'] in ('monster_slot','player_spawn','monster_camp','arena')]
for p in samples:
    hit=bvh.ray_cast(Vector((*p,20)),Vector((0,0,-1)),22)[0]
    if hit and hit.z>.2:errors.append(list(p))
assert not errors,errors[:8]
validation=dict(layout_source='Round01_v2',size_m=[112,112],camps=8,bushes=16,trees=len(roots),
                route_wall_rays=len(samples),wall_intrusions=errors,marker_positions_preserved=True,
                method='Original path centerlines and spawn/camp markers against evaluated wall meshes; not gameplay testing')

camera('Camera_HQ_Spawn',(-28,-61,17),(-43,-42,1.5),28,presentation)
camera('Camera_HQ_Camp',(-19,5,21),(-24,19,1.5),34,presentation)
scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.32,.39,.44,1)
scene.world.node_tree.nodes['Background'].inputs[1].default_value=.50
sun=bpy.data.lights.get('Sun');sun.energy=2.6;sun.angle=.14;sun.color=(1,.87,.70)
scene.view_settings.view_transform='AgX'
ensure_north(scene,layout)
bpy.context.view_layer.update()
scene.camera=bpy.data.objects['Camera_Overview'];scene.camera.data.ortho_scale=170
for screen in bpy.data.screens:
    for area in screen.areas:
        if area.type=='VIEW_3D':
            area.spaces.active.region_3d.view_rotation=bpy.data.objects['Camera_Top'].rotation_euler.to_quaternion()
            area.spaces.active.region_3d.view_perspective='ORTHO';area.spaces.active.region_3d.view_distance=140

print('HQ_EXPORT_BEGIN',flush=True)
exports=[]
def export(asset,objects,collision=False,role='environment'):
    bpy.ops.object.select_all(action='DESELECT');copies=[]
    for source in objects:
        if source.type!='MESH':continue
        obj=source.copy();obj.data=source.data.copy();scene.collection.objects.link(obj)
        obj.hide_viewport=False;obj.hide_render=False;obj.select_set(True);copies.append(obj)
    bpy.context.view_layer.objects.active=copies[0]
    # Apply each evaluated modifier before joining; all materials/UVs stay portable to UE.
    bpy.ops.object.convert(target='MESH')
    bpy.ops.object.join();obj=bpy.context.object;obj.name=asset
    scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
    filename=asset+'.fbx'
    bpy.ops.export_scene.fbx(filepath=str(OUT/'Exports'/filename),use_selection=True,
        apply_unit_scale=True,apply_scale_options='FBX_SCALE_NONE',axis_forward='-Y',axis_up='Z',
        object_types={'MESH'},mesh_smooth_type='FACE',use_triangles=True,bake_anim=False,add_leaf_bones=False)
    exports.append(dict(asset=asset,file=filename,collision=collision,role=role,
                        triangles=sum(len(p.vertices)-2 for p in obj.data.polygons)))
    bpy.data.objects.remove(obj,do_unlink=True)
for c,collision in [(ground,True),(walls,True),(props,False),(bushes,False),(fire,False),(rocks,False),(understory,False)]:
    export('SM_R01HQ_'+c.name.split('_',1)[1],list(c.objects),collision)
for proto in templates:export(proto.name,[proto],False,'tree_prototype')
instances=[]
for obj,asset in tree_instances:
    instances.append(dict(name=obj.name,asset=asset,position_m=list(obj.location),
                          rotation_degrees=math.degrees(obj.rotation_euler.z),scale=list(obj.scale)))
layout.update(title='Emberwild — Detailed Forest Art Study',version='HQ.1',art_style='detailed procedural natural forest',
              materials=MATMETA,exports=exports,instances=instances,
              textures={kind:{channel:Path(img.filepath).name for channel,img in images.items()} for kind,images in TEX.items()},
              art_validation=validation,ue_map='/Game/Map/L_Round01_Emberwild_HQ',ue_assets='/Game/Environment/Round01_HQ',
              camera=dict(location_m=list(scene.camera.location),rotation_degrees=[math.degrees(v) for v in scene.camera.rotation_euler],
                          target_m=[0,0,0],ortho_width_m=170),
              scene_stats=dict(tree_instances=len(instances),tree_variants=6,plant_objects=len(understory.objects),
                               rock_details=len(rocks.objects),unique_export_triangles=sum(e['triangles'] for e in exports)))
(OUT/'Round01_Layout.json').write_text(json.dumps(layout,ensure_ascii=False,indent=2),encoding='utf-8')
(OUT/'Geometry_Verification.json').write_text(json.dumps(validation,indent=2),encoding='utf-8')
scene.render.resolution_x=1800;scene.render.resolution_y=1550
bpy.context.preferences.filepaths.save_version=0
for img in bpy.data.images:
    if img.name.startswith('T_HQ_'):img.filepath=str(OUT/'Textures'/(img.name+'.png'))
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Round01_Emberwild_HQ.blend'),relative_remap=False)
bpy.ops.file.make_paths_relative()
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Round01_Emberwild_HQ.blend'),relative_remap=False)
print('HQ_BUILD_SAVED',json.dumps(layout['scene_stats']),flush=True)
if '--no-render' not in sys.argv:render()
print('HQ_COMPLETE',flush=True)
