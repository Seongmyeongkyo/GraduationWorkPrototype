"""Build the Round 01 environment with Blender 5.2 (no gameplay code).

blender --background --factory-startup --threads 4 --python Scripts/Blender/CreateRound01Map.py
Deterministic geometry, editable collections, FBX layers, layout metadata and previews.
"""
import bpy
import math
import random
import json
import sys
import heapq
from pathlib import Path
from mathutils import Vector
sys.path.insert(0,str(Path(__file__).resolve().parent))
from Round01Coordinates import ensure_north

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'Art' / 'Maps' / 'Round01'
OUT.mkdir(parents=True, exist_ok=True)
(OUT / 'Exports').mkdir(exist_ok=True)
(OUT / 'Previews').mkdir(exist_ok=True)
RNG = random.Random(280926)
SIZE, ARENA_R = 112.0, 18.0
PALETTE = {}
COLLECTIONS = {}
WALLS = []
MARKERS = []

def mat(name, rgb, rough=0.87, emission=0):
    m = bpy.data.materials.new(name)
    m.diffuse_color = (*rgb, 1)
    m.use_nodes = True
    bs = m.node_tree.nodes.get('Principled BSDF')
    bs.inputs['Base Color'].default_value = (*rgb, 1)
    bs.inputs['Roughness'].default_value = rough
    if emission:
        bs.inputs['Emission Color'].default_value = (*rgb, 1)
        bs.inputs['Emission Strength'].default_value = emission
    PALETTE[name] = {'rgb': rgb, 'roughness': rough, 'emission': emission}
    return m

def coll(name):
    c = bpy.data.collections.new(name)
    bpy.context.scene.collection.children.link(c)
    COLLECTIONS[name] = c
    return c

def link(obj, c, material=None):
    for old in list(obj.users_collection):
        old.objects.unlink(obj)
    c.objects.link(obj)
    if material:
        obj.data.materials.append(material)
    return obj

def mesh(name, verts, faces, c, materials):
    data = bpy.data.meshes.new(name)
    data.from_pydata(verts, [], faces)
    data.materials.clear()
    for m in materials:
        data.materials.append(m)
    data.update()
    obj = bpy.data.objects.new(name, data)
    c.objects.link(obj)
    return obj

def ico(name, xyz, scale, material, c, subdivisions=1):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=subdivisions, radius=1, location=xyz)
    obj = bpy.context.object
    obj.name = name
    obj.scale = scale
    obj.rotation_euler = (RNG.uniform(-.2,.2), RNG.uniform(-.2,.2), RNG.random()*6.28)
    return link(obj, c, material)

def cylinder(name, xyz, radius, depth, material, c, vertices=12, top=None):
    bpy.ops.mesh.primitive_cone_add(vertices=vertices, radius1=radius,
        radius2=radius if top is None else top, depth=depth, location=xyz)
    obj = bpy.context.object
    obj.name = name
    return link(obj, c, material)

def beam(name, a, b, radius, material, c, vertices=8):
    direction = Vector(b) - Vector(a)
    obj = cylinder(name, (Vector(a)+Vector(b))/2, radius, direction.length, material, c, vertices)
    obj.rotation_euler = direction.to_track_quat('Z','Y').to_euler()
    return obj

def cube(name, xyz, scale, material, c, rot=0):
    bpy.ops.mesh.primitive_cube_add(size=1, location=xyz)
    obj=bpy.context.object
    obj.name=name
    obj.scale=scale
    obj.rotation_euler.z=rot
    return link(obj,c,material)

def ring(name, xy, r1, r2, z, material, c, n=96):
    verts=[]
    for r in (r1,r2):
        verts += [(xy[0]+r*math.cos(i*math.tau/n),xy[1]+r*math.sin(i*math.tau/n),z) for i in range(n)]
    return mesh(name,verts,[(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)],c,[material])

def dist(a,b):
    return math.hypot(a[0]-b[0],a[1]-b[1])

def segment_dist(p,a,b):
    dx,dy=b[0]-a[0],b[1]-a[1]
    t=max(0,min(1,((p[0]-a[0])*dx+(p[1]-a[1])*dy)/(dx*dx+dy*dy)))
    return dist(p,(a[0]+t*dx,a[1]+t*dy))

def mirror(p):
    return (-p[0],-p[1])

def marker(name,xy,radius,kind,z=0.1,team=None):
    obj=bpy.data.objects.new(name,None)
    guides.objects.link(obj)
    obj.location=(*xy,z)
    obj.empty_display_type='CIRCLE'
    obj.empty_display_size=radius
    obj['role']=kind
    obj['radius_m']=radius
    if team: obj['team']=team
    MARKERS.append({'name':name,'kind':kind,'position_m':[xy[0],xy[1],z], 'radius_m':radius,'team':team})
    return obj

bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
for c in list(bpy.data.collections):
    if not c.objects: bpy.data.collections.remove(c)
ground=coll('01_Ground_and_Trails')
walls=coll('02_Cliffs_and_Boundary')
props=coll('03_Camps_and_Ruins')
trees=coll('04_Trees')
bushes=coll('05_Bushes_NoCollision')
fire=coll('06_Fire_Visual_Placeholders')
guides=coll('07_Gameplay_Layout_Guides')
presentation=coll('08_Presentation_Only')

grass=mat('M_Grass',(0.145,0.255,0.155))
grasslight=mat('M_Grass_Light',(0.20,0.32,0.18))
grassdark=mat('M_Grass_Shade',(0.11,0.215,0.145))
terrainmat=mat('M_Terrain_Vertex',(0.20,0.32,0.18))
vcolor=terrainmat.node_tree.nodes.new('ShaderNodeVertexColor')
vcolor.layer_name='Color'
terrainmat.node_tree.links.new(vcolor.outputs['Color'],terrainmat.node_tree.nodes['Principled BSDF'].inputs['Base Color'])
dirt=mat('M_Trail',(0.46,0.365,0.22))
dirtlight=mat('M_Trail_Light',(0.51,0.405,0.255))
dirtdark=mat('M_Trail_Shade',(0.41,0.325,0.195))
arena=mat('M_Arena_Sand',(0.54,0.475,0.34))
arenalight=mat('M_Arena_Sand_Light',(0.59,0.52,0.375))
earth=mat('M_Earth_Side',(0.15,0.18,0.16))
stone=mat('M_Cliff_Slate',(0.24,0.30,0.28))
stonelight=mat('M_Cliff_Light',(0.34,0.395,0.345))
stonedark=mat('M_Cliff_Dark',(0.18,0.235,0.235))
moss=mat('M_Moss',(0.265,0.36,0.19))
cutstone=mat('M_Ruin_Limestone',(0.54,0.55,0.43))
wood=mat('M_Bark',(0.225,0.15,0.09))
woodend=mat('M_Wood_End',(0.52,0.37,0.20))
leafmats=[mat('M_Canopy_Deep',(0.07,0.235,0.185)),mat('M_Canopy_Teal',(0.085,0.315,0.245)),mat('M_Canopy_Light',(0.18,0.38,0.245))]
bushmats=[mat('M_Bush_Base',(0.24,0.34,0.075)),mat('M_Bush_Leaf',(0.37,0.46,0.105)),mat('M_Bush_Tips',(0.49,0.54,0.16))]
blue=mat('M_Team_A_Blue',(0.085,0.49,0.66))
red=mat('M_Team_B_Coral',(0.73,0.22,0.15))
gold=mat('M_Brass',(0.70,0.50,0.18),.55)
cloth=mat('M_Canvas',(0.55,0.48,0.31))
ember=mat('M_Embers',(1,.16,.01),emission=2)
flame=mat('M_Flame',(1,.55,.06),emission=3)

spawns=[{'name':'Spawn_A','xy':(-44,-44),'r':6.3,'team':'A'}, {'name':'Spawn_B','xy':(44,44),'r':6.3,'team':'B'}]
campbase=[(-34,-23,4.2),(-23,-34,4.2),(-39,-9,4.7),(-9,-39,4.7)]
camps=[]
for team,sgn in [('A',1),('B',-1)]:
    for i,(x,y,r) in enumerate(campbase,1):
        camps.append({'name':f'Camp_{team}{i}','xy':(x*sgn,y*sgn),'r':r,'kind':'team_side','team':team})
for prefix,sgn in [('NW',1),('SE',-1)]:
    for i,(x,y,r) in enumerate([(-35,33,5.1),(-18,37,4.6)],1):
        camps.append({'name':f'Camp_{prefix}{i}','xy':(x*sgn,y*sgn),'r':r,'kind':'contested','team':None})

paths=[]
def path(points,width=5.5):
    paths.append({'points':points,'width_m':width})
def paired_path(points,width=5.5):
    path(points,width)
    path([mirror(p) for p in points],width)
paired_path([(-44,-44),(-42,-34),(-34,-23),(-27,-19),(-21,-9),(-18,0),(0,0)],6)
paired_path([(-44,-44),(-34,-42),(-23,-34),(-19,-27),(-9,-21),(0,-18),(0,0)],6)
paired_path([(-34,-23),(-41,-20),(-39,-9),(-44,9),(-35,33),(-18,37),(0,43),(9,39)],5.5)
paired_path([(-23,-34),(-20,-41),(-9,-39),(9,-44),(18,-37),(35,-33),(40,-20),(39,9)],5.5)
paired_path([(-39,-9),(-31,-7),(-21,-9)],5)
paired_path([(-9,-39),(-7,-31),(-9,-21)],5)
paired_path([(-35,33),(-28,23),(-22,16),(-12,10),(0,0)],6)
paired_path([(-18,37),(-12,29),(-5,24),(0,18)],5.5)

segments=[(a,b,p['width_m']/2) for p in paths for a,b in zip(p['points'],p['points'][1:])]
clearings=[((0,0),ARENA_R)]+[(c['xy'],c['r']) for c in camps+spawns]
def clearance(p):
    values=[segment_dist(p,a,b)-r for a,b,r in segments]
    values += [dist(p,c)-r for c,r in clearings]
    values += [52-abs(p[0]),52-abs(p[1])]
    return min(values)

# A flat, continuous walking surface; color boundaries do not create collision steps.
verts=[]
faces=[]
n=224
for iy in range(n+1):
    for ix in range(n+1):
        verts.append((-56+112*ix/n,-56+112*iy/n,0))
for iy in range(n):
    for ix in range(n):
        a=iy*(n+1)+ix
        faces.append((a,a+1,a+n+2,a+n+1))
terrain=mesh('SM_R01_Ground_112m',verts,faces,ground,[terrainmat])
colors=terrain.data.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='POINT')
for i,p in enumerate(verts):
    noise=.5*math.sin(p[0]*.32+math.sin(p[1]*.41))+.5*math.cos(p[1]*.35+p[0]*.19)
    edge=min([dist(p,c)-r for c,r in clearings[1:]]+[segment_dist(p,a,b)-r for a,b,r in segments])
    blend=max(0,min(1,(.40-edge+.12*noise)/.80))
    turf=[.13+.023*noise,.265+.030*noise,.15+.018*noise]
    sand=[.46+.016*noise,.365+.016*noise,.22+.013*noise]
    rgb=[a*(1-blend)+b*blend for a,b in zip(turf,sand)]
    ar=max(0,min(1,(18.2-dist(p,(0,0)))/.45))
    rgb=[a*(1-ar)+b*ar for a,b in zip(rgb,[.54+.009*noise,.475+.009*noise,.34+.008*noise])]
    colors.data[i].color=(*rgb,1)
cube('SM_R01_Earth_Foundation',(0,0,-1.6),(112,112,3.1),earth,ground)
ring('Arena_Outer_Inlay',(0,0),17.55,17.80,.035,cutstone,ground)
ring('Arena_Inner_Inlay',(0,0),16.98,17.08,.04,stonelight,ground)
ring('Arena_Center_Inlay',(0,0),2.35,2.48,.04,stonelight,ground)
# Subtle compass paving, flush with the arena.
for a in range(4):
    theta=a*math.pi/2
    v=[(0,0,.045),(1.15*math.cos(theta+.16),1.15*math.sin(theta+.16),.045),(2.05*math.cos(theta),2.05*math.sin(theta),.045),(1.15*math.cos(theta-.16),1.15*math.sin(theta-.16),.045)]
    mesh(f'Arena_Compass_{a}',v,[(0,1,2,3)],ground,[cutstone])
marker('Arena_FinalFight_Center',(0,0),18,'arena')

def cliff(name,xy,r,h=2.6):
    count=11
    outline=[RNG.uniform(.83,1.02) for _ in range(count)]
    vs=[]
    for z,shrink in [(-.1,1),(h*.58,.94),(h,.77)]:
        for i in range(count):
            a=i*math.tau/count
            vs.append((xy[0]+math.cos(a)*r*outline[i]*shrink,xy[1]+math.sin(a)*r*outline[i]*shrink,z+RNG.uniform(-.14,.14)))
    fs=[]
    for level in range(2):
        for i in range(count):
            j=(i+1)%count
            fs.append((level*count+i,level*count+j,(level+1)*count+j,(level+1)*count+i))
    fs.append(tuple(range(2*count,3*count)))
    obj=mesh(name,vs,fs,walls,[stone,stonelight,stonedark,moss])
    for f in obj.data.polygons: f.material_index=RNG.choice([0,0,1,2])
    obj.data.polygons[-1].material_index=3
    WALLS.append({'name':name,'center_m':xy,'radius_m':r,'height_m':h})
    return obj

def tree(name,x,y,z,s=1,pine=False):
    height=4.8*s
    cylinder(name+'_Trunk',(x,y,z+height*.40),.22*s,height*.8,wood,trees,7,top=.11*s)
    if pine:
        for i in range(3):
            cylinder(name+f'_Needles{i}',(x,y,z+height*(.48+.21*i)),(1.5-.30*i)*s,2.4*s,leafmats[i%3],trees,7,top=.04*s)
    else:
        for i in range(3):
            a=i*math.tau/3+.3
            end=(x+math.cos(a)*s*.85,y+math.sin(a)*s*.85,z+height*.75)
            beam(name+f'_Limb{i}',(x,y,z+height*.45),end,.13*s,wood,trees,6)
            ico(name+f'_Crown{i}',(end[0],end[1],end[2]+s*.5),(1.4*s,1.35*s,1.45*s),leafmats[i],trees,2)
        ico(name+'_CrownTop',(x+.1*s,y,z+height),(1.45*s,1.3*s,1.2*s),leafmats[1],trees,1)

# Deliberately reserve corridors first, then fit paired cliff islands between them.
islands=[]
candidates=[]
for _ in range(1400):
    p=(RNG.uniform(-47,47),RNG.uniform(-47,47))
    if p[0]+p[1]>0: continue
    r=min(5.6,clearance(p)-.7)
    if r>=2.5: candidates.append((r,p))
candidates.sort(reverse=True)
for r,p in candidates:
    if any(dist(p,q)<r+qr+2.2 or dist(mirror(p),q)<r+qr+2.2 for q,qr in islands): continue
    if dist(p,mirror(p))<r*2+2.2: continue
    islands += [(p,r),(mirror(p),r)]
    if len(islands)>=30: break
for i,(xy,r) in enumerate(islands):
    h=2.1+(i//2%3)*.30
    cliff(f'SM_CliffIsland_{i+1:02}',xy,r,h)
    tree(f'Tree_Island_{i+1:02}_A',xy[0]-.35,xy[1]+.25,h,.8+(r-2.5)*.12,pine=i%4==0)
    if r>4:
        tree(f'Tree_Island_{i+1:02}_B',xy[0]+1.5,xy[1]-1.2,h,.7,True)
        tree(f'Tree_Island_{i+1:02}_C',xy[0]-1.5,xy[1]-1.35,h,.73,False)
        ico(f'Island_{i}_MossRock',(*xy,h+.18),(1.1,1,.5),moss,props)

# Unbroken square boundary, with a continuous rock core behind individual outcrops.
for side in range(4):
    angle=side*math.pi/2
    def edgepoint(t,inset=54):
        return (t*math.cos(angle)-inset*math.sin(angle),t*math.sin(angle)+inset*math.cos(angle))
    center=edgepoint(0)
    cube(f'SM_Boundary_Core_{side}',(*center,.65),(112,3.6,1.5),stonedark,walls,angle)
    for j in range(21):
        xy=edgepoint(-53+j*5.3)
        cliff(f'SM_Boundary_Rock_{side}_{j:02}',xy,2.25,RNG.uniform(1.8,2.7))
        if j%2==0:
            tree(f'Tree_Edge_{side}_{j}',xy[0],xy[1],2.0,RNG.uniform(.75,1.05),j%4==0)

def camp_rocks(c):
    x,y=c['xy']; r=c['r']
    # Back arc faces away from the center. The front remains a wide open entrance.
    theta=math.atan2(y,x)
    for i in range(5):
        a=theta+(i-2)*.26
        px,py=x+(r+.40)*math.cos(a),y+(r+.40)*math.sin(a)
        ico(c['name']+f'_BackRock{i}',(px,py,.38),(.64,.57,.52),stonelight,props)
    # Broken low masonry, bone stones and a nest depression are only scenery.
    ring(c['name']+'_Nest',c['xy'],.85,1.08,.028,dirtdark,ground,32)
    for i in range(3):
        a=theta+.5*(i-1)
        px,py=x+(r-1.1)*math.cos(a),y+(r-1.1)*math.sin(a)
        cube(c['name']+f'_Relic{i}',(px,py,.3),(.5,.8,.6),cutstone,props,a+.1)
    if c['kind']=='contested':
        px,py=x+(r-.7)*math.cos(theta),y+(r-.7)*math.sin(theta)
        cylinder(c['name']+'_RuinedColumn',(px,py,1.25),.45,2.5,cutstone,props,7,top=.30)
        ring(c['name']+'_NeutralSeal',c['xy'],1.5,1.66,.031,gold,ground,48)
    marker(c['name'],c['xy'],r,'monster_camp',team=c['team'])
    for i,a in enumerate([0,math.tau/3,2*math.tau/3]):
        marker(c['name']+f'_MonsterSlot_{i+1}',(x+1.45*math.cos(a),y+1.45*math.sin(a)),.45,'monster_slot')
for c in camps: camp_rocks(c)

def banner(name,x,y,z,material,angle):
    beam(name+'_Pole',(x,y,z),(x,y,z+3.2),.07,wood,props)
    ico(name+'_Finial',(x,y,z+3.28),(.13,.13,.20),gold,props)
    u=(math.cos(angle),math.sin(angle))
    points=[(x,y,z+2.95),(x+u[0]*1.1,y+u[1]*1.1,z+2.85),(x+u[0]*1.05,y+u[1]*1.05,z+1.55),(x+u[0]*.53,y+u[1]*.53,z+1.85),(x,y,z+1.7)]
    mesh(name+'_Cloth',points,[(0,1,2,3,4)],props,[material])

def campfire(name,x,y):
    for i in range(11):
        a=i*math.tau/11
        ico(name+f'_FireStone{i}',(x+1.06*math.cos(a),y+1.06*math.sin(a),.20),(.34,.25,.23),stonelight,props)
    cylinder(name+'_Ash',(x,y,.035),.87,.06,stonedark,props,16)
    for i in range(3):
        a=i*math.pi/3
        beam(name+f'_Log{i}',(x-.72*math.cos(a),y-.72*math.sin(a),.17),(x+.72*math.cos(a),y+.72*math.sin(a),.23),.15,wood,props)
    ico(name+'_Ember',(x,y,.30),(.51,.48,.26),ember,fire,1)
    for i in range(5):
        a=i*2.4
        cylinder(name+f'_Flame{i}',(x+math.cos(a)*.26,y+math.sin(a)*.26,.70+(.18 if i==0 else 0)),.25,1.05 if i==0 else .7,flame,fire,5,top=0)
    lamp=bpy.data.lights.new(name+'_WarmLight','POINT'); lamp.energy=85; lamp.color=(1,.35,.07); lamp.shadow_soft_size=1.5
    ob=bpy.data.objects.new(name+'_WarmLight',lamp); presentation.objects.link(ob); ob.location=(x,y,1.7)

for s in spawns:
    x,y=s['xy']; color=blue if s['team']=='A' else red
    marker(s['name'],s['xy'],s['r'],'team_spawn',team=s['team'])
    campfire(s['name'],x,y)
    ring(s['name']+'_CampEdging',s['xy'],6.0,6.2,.035,stonelight,ground)
    for i in range(3):
        # Slots are off the central fire, facing into the map.
        a=(math.pi/4 if s['team']=='A' else 5*math.pi/4)+(i-1)*.9
        xy=(x+3.1*math.cos(a),y+3.1*math.sin(a))
        ring(s['name']+f'_SpawnPaver{i+1}',xy,.64,.77,.04,color,ground,24)
        marker(s['name']+f'_Player_{i+1}',xy,.5,'player_spawn',.96,s['team'])
    sign=1 if s['team']=='A' else -1
    banner(s['name']+'_Banner',x-sign*3.6,y+sign*2.9,0,color,0)
    # A canvas lean-to and supplies at the rear, outside the three spawn slots.
    tx,ty=x-sign*3.1,y-sign*3.4
    vs=[(tx-1.25,ty-.9,.05),(tx+1.25,ty-.9,.05),(tx+1.25,ty+.9,.05),(tx-1.25,ty+.9,.05),(tx-1.25,ty,1.8),(tx+1.25,ty,1.8)]
    mesh(s['name']+'_CanvasShelter',vs,[(0,1,5,4),(4,5,2,3),(0,4,3)],props,[cloth])
    beam(s['name']+'_TentRidge',vs[4],vs[5],.07,wood,props)
    for j in range(2):
        cylinder(s['name']+f'_Barrel{j}',(x+sign*(1.6+j*.8),y-sign*4.2,.45),.35,.9,wood,props,10)

# Reeds identify future concealment zones. No blocking geometry is added here.
bushlist=[]
basebush=[(-29,-13,3.1,1.35,.7),(-13,-29,3.1,1.35,.87),(-45,-.5,2.6,1.3,1.35),(-.5,-45,2.6,1.3,.20),(-26,27,3.1,1.4,-.8),(-9,32,2.7,1.35,.9),(-23,7,2.8,1.35,1.2),(-7,-23,2.8,1.35,.3)]
def bush_cluster(name,x,y,rx,ry,theta):
    vs=[]; fs=[]; mi=[]
    for k in range(145):
        a=RNG.random()*math.tau; r=math.sqrt(RNG.random())
        xx=math.cos(a)*r*rx; yy=math.sin(a)*r*ry
        px=x+xx*math.cos(theta)-yy*math.sin(theta)
        py=y+xx*math.sin(theta)+yy*math.cos(theta)
        h=RNG.uniform(.6,1.3)*(1-.17*r)
        for blade in range(3):
            ang=RNG.random()*math.tau
            w=RNG.uniform(.10,.19); lean=RNG.uniform(.20,.5)
            base=len(vs)
            vs += [(px-w*math.cos(ang),py-w*math.sin(ang),.03),(px+w*math.cos(ang),py+w*math.sin(ang),.03),(px+lean*math.cos(ang),py+lean*math.sin(ang),h)]
            fs += [(base,base+1,base+2)]
            mi += [k%3]
    obj=mesh(name,vs,fs,bushes,bushmats)
    for poly,idx in zip(obj.data.polygons,mi): poly.material_index=idx
    ob=marker(name+'_Zone',(x,y),rx,'bush')
    ob['half_width_m']=ry; ob['rotation_degrees']=math.degrees(theta)
    MARKERS[-1].update({'half_extents_m':[rx,ry,1.4],'rotation_degrees':math.degrees(theta),'shape':'ellipse'})
    bushlist.append({'name':name,'center_m':[x,y],'radii_m':[rx,ry],'rotation_degrees':math.degrees(theta)})
for i,(x,y,rx,ry,theta) in enumerate(basebush):
    bush_cluster(f'Bush_{i*2+1:02}',x,y,rx,ry,theta)
    bush_cluster(f'Bush_{i*2+2:02}',-x,-y,rx,ry,theta+math.pi)

# Small plants are kept in non-walkable islands; no random clutter on paths.
for i,(xy,r) in enumerate(islands):
    for j in range(4):
        a=j*math.pi/2+.3
        p=(xy[0]+math.cos(a)*r*.65,xy[1]+math.sin(a)*r*.65,2.2+(i//2%3)*.3)
        ico(f'Groundcover_{i}_{j}',p,(.65,.5,.25),moss,props)

scene=bpy.context.scene
scene.unit_settings.system='METRIC'
scene.unit_settings.scale_length=1
scene.render.use_file_extension=True
bpy.context.preferences.filepaths.save_version=0
scene.render.engine='CYCLES'
scene.cycles.samples=24
scene.cycles.use_denoising=True
scene.render.threads_mode='FIXED'; scene.render.threads=4
scene.render.resolution_percentage=100
scene.world.color=(.28,.28,.28)
scene.world.use_nodes=True
scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.46,.57,.64,1)
scene.world.node_tree.nodes['Background'].inputs[1].default_value=.6
sun=bpy.data.lights.new('Sun','SUN'); sun.energy=2.1; sun.angle=math.radians(18)
ob=bpy.data.objects.new('Sun',sun); presentation.objects.link(ob); ob.rotation_euler=(.4,-.35,-.5)
scene.view_settings.view_transform='AgX'
scene.render.image_settings.file_format='PNG'
scene.render.film_transparent=False

def camera(name,loc,target,ortho):
    data=bpy.data.cameras.new(name); data.type='ORTHO'; data.ortho_scale=ortho; data.clip_end=1000
    obj=bpy.data.objects.new(name,data); presentation.objects.link(obj)
    obj.location=loc; obj.rotation_euler=(Vector(target)-obj.location).to_track_quat('-Z','Y').to_euler()
    return obj
camhero=camera('Camera_Overview',(104,-136,160),(0,0,0),163)
camtop=camera('Camera_Top',(0,0,190),(0,0,0),124)
camspawn=camera('Camera_Spawn_Detail',(-27,-65,29),(-40,-37,0),37)
camplay=camera('Camera_Camp_Detail',(-16,1,31),(-32,24,0),44)

for screen in bpy.data.screens:
    for area in screen.areas:
        if area.type=='VIEW_3D':
            area.spaces.active.region_3d.view_distance=135
            area.spaces.active.region_3d.view_location=(0,0,0)
            area.spaces.active.region_3d.view_rotation=camhero.rotation_euler.to_quaternion()
            area.spaces.active.clip_end=1000
            area.spaces.active.shading.type='MATERIAL'
            area.spaces.active.overlay.show_extras=False

# Conservative path validation includes a player radius and extra cliff margin.
def blocked(x,y):
    if max(abs(x),abs(y))>51: return True
    return any(dist((x,y),w['center_m']) < w['radius_m']+.6 for w in WALLS if 'Island' in w['name'])
grid={(x,y) for x in range(-51,52) for y in range(-51,52) if not blocked(x,y)}
def shortest(start,end):
    start=tuple(round(v) for v in start); end=tuple(round(v) for v in end)
    q=[(0,start)]; costs={start:0}
    while q:
        cost,p=heapq.heappop(q)
        if p==end: return round(cost,2)
        if cost>costs[p]: continue
        for dx,dy in [(1,0),(-1,0),(0,1),(0,-1),(1,1),(1,-1),(-1,1),(-1,-1)]:
            n=(p[0]+dx,p[1]+dy)
            if n not in grid: continue
            if dx and dy and ((p[0]+dx,p[1]) not in grid or (p[0],p[1]+dy) not in grid): continue
            new=cost+math.hypot(dx,dy)
            if new<costs.get(n,1e9): costs[n]=new; heapq.heappush(q,(new,n))
    raise RuntimeError(f'Unreachable layout marker: {start} -> {end}')
validation={'method':'1m grid; cliff bounding circles + 0.6m player clearance; no UE NavMesh simulation', 'travel_speed_m_s':6,'paths':[]}
for s in spawns:
    for c in camps+[{'name':'Arena','xy':(0,0)}]:
        d=shortest(s['xy'],c['xy'])
        validation['paths'].append({'from':s['name'],'to':c['name'],'distance_m':d,'seconds_at_6m_s':round(d/6,2)})
assert len(camps)==12 and len(bushlist)==16
assert all(not blocked(*m['position_m'][:2]) for m in MARKERS if m['kind'] in ('monster_camp','player_spawn','arena'))

layout={'title':'Emberwild / Round 01','version':'1.0','seed':280926,'units':'meters','size_m':[112,112],
    'arena_radius_m':18,'coordinate_convention':'Blender XY: +X right/east, +Y up/north; Z up. UE export: X=x*100, Y=-y*100, Z=z*100.',
    'camps':camps,'spawns':spawns,'bushes':bushlist,'paths':paths,'walls':WALLS,'markers':MARKERS,
    'materials':PALETTE,'validation':validation,'gameplay_implemented':False}
ensure_north(scene,layout)
(OUT/'Round01_Layout.json').write_text(json.dumps(layout,ensure_ascii=False,indent=2),encoding='utf-8')
(OUT/'Validation.json').write_text(json.dumps(validation,indent=2),encoding='utf-8')

scene.camera=camhero
scene.render.resolution_x=1600; scene.render.resolution_y=1400
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Round01_Emberwild.blend'))
print('R01_BLEND_SAVED',flush=True)

exportcols=[ground,walls,props,trees,bushes,fire]
# Keep individual editable objects in Blender. FBX layers are combined to make UE placement simple.
exportmeta=[]
for collection in exportcols:
    bpy.ops.object.select_all(action='DESELECT')
    originals=[o for o in collection.objects if o.type=='MESH']
    copies=[]
    for original in originals:
        obj=original.copy(); obj.data=original.data.copy(); scene.collection.objects.link(obj)
        obj.select_set(True); copies.append(obj)
    bpy.context.view_layer.objects.active=copies[0]
    bpy.ops.object.join()
    obj=bpy.context.object
    obj.name='SM_R01_'+collection.name.split('_',1)[1]
    scene.cursor.location=(0,0,0)
    bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
    # Face projection provides nondegenerate UV tangents, even for procedural reeds.
    uv=obj.data.uv_layers.active or obj.data.uv_layers.new(name='UVMap')
    for polygon in obj.data.polygons:
        axis=max(range(3),key=lambda k:abs(polygon.normal[k]))
        axes=[k for k in range(3) if k!=axis]
        for li in polygon.loop_indices:
            co=obj.data.vertices[obj.data.loops[li].vertex_index].co
            uv.data[li].uv=(co[axes[0]]/4,co[axes[1]]/4)
    filename=obj.name+'.fbx'
    bpy.ops.export_scene.fbx(filepath=str(OUT/'Exports'/filename),use_selection=True,
        apply_unit_scale=True,apply_scale_options='FBX_SCALE_NONE',axis_forward='-Y',axis_up='Z',
        object_types={'MESH'},use_mesh_modifiers=True,mesh_smooth_type='FACE',use_triangles=True,
        bake_anim=False,add_leaf_bones=False,path_mode='AUTO')
    exportmeta.append({'file':filename,'asset':obj.name,'materials':[m.name for m in obj.data.materials],
        'collision':collection in (ground,walls),'triangles':sum(len(p.vertices)-2 for p in obj.data.polygons)})
    bpy.data.objects.remove(obj,do_unlink=True)
layout['exports']=exportmeta
layout['scene_stats']={'mesh_objects':len([o for o in scene.objects if o.type=='MESH']),
    'triangles':sum(e['triangles'] for e in exportmeta),'cliff_islands':len(islands),'camps':12,'bushes':16}
(OUT/'Round01_Layout.json').write_text(json.dumps(layout,ensure_ascii=False,indent=2),encoding='utf-8')
print('R01_EXPORT_DONE',layout['scene_stats'],flush=True)

if '--no-render' not in sys.argv:
    for name,cam,w,h in [('Overview',camhero,1600,1400),('TopDown',camtop,1600,1600),('Spawn',camspawn,1400,1050),('Camp',camplay,1400,1050)]:
        scene.camera=cam; scene.render.resolution_x=w; scene.render.resolution_y=h
        scene.render.filepath=str(OUT/'Previews'/('Round01_'+name+'.png'))
        bpy.ops.render.render(write_still=True)
        print('R01_RENDER_DONE',name,flush=True)
print('R01_COMPLETE',flush=True)
