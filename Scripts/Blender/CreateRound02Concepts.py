"""Two editable, low-poly arena concepts; no gameplay implementation.

blender --background --factory-startup --threads 4 --python-exit-code 1 --python <this file>
Optional script arguments: --no-render, --concept=A, --concept=B, --render-only.
"""
import bpy
import math
import json
import sys
from pathlib import Path
from collections import Counter
from mathutils import Vector
from mathutils.bvhtree import BVHTree

sys.path.insert(0,str(Path(__file__).resolve().parent))
from ArenaPrototypeGeometry import box, cylinder, arc, ring, beam, text_mesh, camera

ROOT = Path(__file__).resolve().parents[2]


class Concept:
    def __init__(self, variant):
        self.variant = variant
        self.slug = 'Round02_A_CrownArena' if variant == 'A' else 'Round02_B_TriadArena'
        self.prefix = 'R02'+variant
        self.out = ROOT/'Art'/'Maps'/self.slug
        for p in (self.out,self.out/'Exports',self.out/'Previews'):
            p.mkdir(parents=True,exist_ok=True)
        bpy.ops.object.select_all(action='SELECT')
        bpy.ops.object.delete(use_global=False)
        for c in list(bpy.data.collections):
            bpy.data.collections.remove(c)
        for m in list(bpy.data.materials):
            bpy.data.materials.remove(m)
        self.groups = []
        self.materials = {}
        self.markers = []
        self.arenas = []
        self.lobbies = []
        self.corridors = []
        self.gates = []
        self.paths = []
        self.samples = []
        self.guides = self.collection('Layout_Guides',export=False)
        self.presentation = self.collection('Presentation_Only',export=False)
        self.floor = self.collection('Arena_Floors',collision=True)
        self.walls = self.collection('Arena_Walls',collision=True)
        self.cover = self.collection('Optional_Cover',collision=True)
        self.detail = self.collection('Inlays_and_Decoration')
        self.shop_props = self.collection('Shop_Props',collision=True)
        self.labels = self.collection('Layout_Labels')
        self.mat('Sand',(.48,.40,.28))
        self.mat('SandLight',(.55,.47,.33))
        self.mat('Basalt',(.12,.19,.20))
        self.mat('Stone',(.26,.34,.34))
        self.mat('CutStone',(.49,.55,.50))
        self.mat('Gold',(.72,.48,.16),.5)
        self.mat('Grass',(.16,.27,.19))
        self.mat('Leaf',(.13,.30,.22))
        self.mat('Wood',(.28,.17,.09))
        self.mat('Canvas',(.75,.67,.48))
        self.mat('TeamA',(.05,.43,.70),.6)
        self.mat('TeamB',(.75,.20,.14),.6)
        self.mat('GateRed',(.95,.12,.06),.6,.5)
        self.mat('Arena1',(.74,.51,.19),.6,.1)
        self.mat('Arena2',(.13,.56,.49),.6,.1)
        self.mat('Arena3',(.52,.30,.64),.6,.1)
        self.mat('Ink',(.08,.13,.14))
        self.mat('Fire',(1,.40,.04),.6,2)

    def mat(self,name,rgb,rough=.85,emission=0):
        m = bpy.data.materials.new('M_'+self.prefix+'_'+name)
        m.diffuse_color = (*rgb,1)
        m.use_nodes = True
        bs = m.node_tree.nodes.get('Principled BSDF')
        bs.inputs['Base Color'].default_value = (*rgb,1)
        bs.inputs['Roughness'].default_value = rough
        bs.inputs['Emission Strength'].default_value = emission
        if emission:
            bs.inputs['Emission Color'].default_value = (*rgb,1)
            bs.inputs['Emission Strength'].default_value = emission
        self.materials[name] = m
        return m

    def m(self,name):
        return self.materials[name]

    def collection(self,name,collision=False,origin=(0,0,0),export=True,role=None):
        c = bpy.data.collections.new(name)
        bpy.context.scene.collection.children.link(c)
        if export:
            self.groups.append({'collection':c,'collision':collision,'origin_m':list(origin),
                                'role':role or name.lower(),'asset':'SM_'+self.prefix+'_'+name})
        return c

    def marker(self,name,xy,kind,radius=.6,z=.1,**metadata):
        obj = bpy.data.objects.new(name,None)
        self.guides.objects.link(obj)
        obj.location = (*xy,z)
        obj.empty_display_type = 'CIRCLE'
        obj.empty_display_size = radius
        row = dict(name=name,position_m=[*xy,z],kind=kind,radius_m=radius,**metadata)
        self.markers.append(row)
        for k,v in row.items():
            if k not in ('position_m',): obj[k]=v
        return row

    def disk(self,name,xy,r,collection,material):
        cylinder(name+'_Foundation',(*xy,-1.45),r+.45,2.5,collection,self.m('Basalt'),96,top=r+.3)
        cylinder(name+'_WalkingSurface',(*xy,-.10),r,.20,collection,material,96)
        ring(name+'_Rim',xy,r-.4,r-.1,.022,self.detail,self.m('Gold'))

    def wall_circle(self,name,xy,r,open_angles,halfwidth,collection,height=2.4):
        # Intervals are exact radial cutouts: no small leftover facets across doorways.
        cut = math.asin(halfwidth/r)
        cuts = []
        for a in open_angles:
            a %= math.tau
            lo,hi = a-cut,a+cut
            if lo < 0: cuts.extend([(0,hi),(lo+math.tau,math.tau)])
            elif hi > math.tau: cuts.extend([(lo,math.tau),(0,hi-math.tau)])
            else: cuts.append((lo,hi))
        cuts.sort()
        start=0
        intervals=[]
        for lo,hi in cuts:
            if lo>start: intervals.append((start,lo))
            start=max(start,hi)
        if start<math.tau: intervals.append((start,math.tau))
        for i,(a,b) in enumerate(intervals):
            arc(name+f'_Wall_{i}',xy,r,r+.55,0,height,a,b,collection,self.m('Stone'))
            arc(name+f'_Cap_{i}',xy,r-.04,r+.61,height,height+.16,a,b,self.detail,self.m('CutStone'))
        for i in range(16):
            a = i*math.tau/16
            if any(abs(math.atan2(math.sin(a-o),math.cos(a-o))) < cut+.10 for o in open_angles):
                continue
            p=(xy[0]+(r+.28)*math.cos(a),xy[1]+(r+.28)*math.sin(a))
            cylinder(name+f'_Buttress_{i}',(*p,(height+.6)/2),.52,height+.6,collection,self.m('Basalt'),6)
            cylinder(name+f'_GoldCap_{i}',(*p,height+.67),.54,.14,self.detail,self.m('Gold'),6)

    def gate(self,name,xy,angle,width,arena,role='transfer_gate'):
        # Own asset and center pivot: can later be replaced/animated independently.
        c = self.collection(name,True,(*xy,0),role=role)
        normal=(math.cos(angle),math.sin(angle))
        tangent=(-normal[1],normal[0])
        for s in (-1,1):
            p=(xy[0]+tangent[0]*s*(width/2+.34),xy[1]+tangent[1]*s*(width/2+.34))
            box(name+'_Frame'+str(s),(*p,1.6),(.8,.8,3.2),self.walls,self.m('Basalt'),angle)
            cylinder(name+'_Beacon'+str(s),(*p,3.33),.27,.22,self.detail,self.m('GateRed'),8)
        # Low foundation plus bars unmistakably indicate the blocked state; all are visual placeholders.
        box(name+'_Threshold',(*xy,.035),(.75,width,.07),self.detail,self.m('GateRed'),angle)
        for i in range(11):
            offset=-width/2+width*i/10
            p=(xy[0]+tangent[0]*offset,xy[1]+tangent[1]*offset)
            box(name+f'_Bar{i:02}',(*p,1.4),(.28,.22,2.8),c,self.m('GateRed'),angle)
        for z in (.55,2.35):
            box(name+'_Crossbar'+str(z),(*xy,z),(.32,width,.18),c,self.m('GateRed'),angle)
        # A thin solid collision surface avoids squeezing between decorative bars.
        box(name+'_SolidLowerPanel',(*xy,.55),(.22,width,1.10),c,self.m('GateRed'),angle)
        row=dict(name=name,position_m=[*xy,0],angle_degrees=math.degrees(angle),width_m=width,
                 arena=arena,asset='SM_'+self.prefix+'_'+name,initial_state='closed_static_placeholder',role=role)
        self.gates.append(row)
        self.marker(name+'_LogicAnchor',xy,role,width/2,arena=arena,
                    rotation_degrees=math.degrees(angle),gate_asset=row['asset'])

    def arena(self,name,xy,r,open_angles,accent,duel_only=False,rotation=0):
        def point(x,y):
            return (xy[0]+x*math.cos(rotation)-y*math.sin(rotation),
                    xy[1]+x*math.sin(rotation)+y*math.cos(rotation))
        self.disk(name,xy,r,self.floor,self.m('Sand'))
        self.wall_circle(name,xy,r,open_angles,2.9 if duel_only else 3.2,self.walls)
        ring(name+'_OuterInlay',xy,r-1.5,r-1.30,.026,self.detail,self.m(accent))
        ring(name+'_InnerInlay',xy,2.35,2.48,.028,self.detail,self.m('CutStone'))
        for a in range(4):
            angle=a*math.pi/2
            box(name+f'_Compass{a}',(xy[0]+math.cos(angle)*1.5,xy[1]+math.sin(angle)*1.5,.032),
                (1.25,.12,.028),self.detail,self.m('Gold'),angle)
        text_mesh(name+'_Number',name[-1] if duel_only else 'CROWN',
                  point(0,4 if duel_only else 5),1.15,self.labels,self.m('Ink'))
        spawn_offset=7.5 if duel_only else 13
        for team,sign in [('A',-1),('B',1)]:
            p=point(sign*spawn_offset,0)
            ring(name+'_DuelPad_'+team,p,.66,.85,.032,self.detail,self.m('Team'+team),32)
            self.marker(name+'_DuelSpawn_'+team,p,'duel_spawn',.65,z=1,team=team,arena=name)
            self.samples.append(p)
            if not duel_only:
                for i,dy in enumerate((-3,0,3),1):
                    p=point(sign*spawn_offset,dy)
                    self.marker(name+f'_TeamSpawn_{team}{i}',p,'team_combat_spawn',.65,z=1,team=team,arena=name)
        # Sparse, mirrored edge cover; center stays completely open.
        for sign in (-1,1):
            p=point(0,sign*(r-(3.5 if duel_only else 5)))
            box(name+'_Cover_'+str(sign),(*p,1.05),(3.6,1.35,2.1),self.cover,self.m('Basalt'),rotation)
            box(name+'_CoverCap_'+str(sign),(*p,2.18),(3.8,1.5,.16),self.cover,self.m('CutStone'),rotation)
        self.marker(name+'_Center',xy,'arena',r,arena=name)
        self.arenas.append(dict(name=name,center_m=list(xy),radius_m=r,duel_spawn_separation_m=spawn_offset*2,
                               rotation_degrees=math.degrees(rotation)))
        for x in range(-int(r-2),int(r-1),2):
            self.samples.append(point(x,0))

    def shop(self,team,xy,r=6,opening=None,choices=3):
        c=self.collection('Team'+team+'_Lobby',True,(*xy,0),role='team_lobby_geometry')
        self.disk('Lobby'+team,xy,r,c,self.m('Stone'))
        self.wall_circle('Lobby'+team,xy,r,[] if opening is None else [opening],2.1,c,height=1.25)
        ring('Lobby'+team+'_TeamRing',xy,r-.9,r-.65,.031,self.detail,self.m('Team'+team))
        text_mesh('Lobby'+team+'_Label','TEAM '+team,(xy[0],xy[1]+.2),.65,self.labels,self.m('Canvas'))
        # Small stall occupies the rear edge, leaving the center and selection pads walkable.
        sx,sy=xy[0],xy[1]+3.45
        box('Shop'+team+'_Counter',(sx,sy,.60),(3.1,1.05,1.2),self.shop_props,self.m('Wood'))
        box('Shop'+team+'_Top',(sx,sy,1.26),(3.3,1.18,.12),self.shop_props,self.m('Gold'))
        for sign in (-1,1):
            cylinder('Shop'+team+'_Post'+str(sign),(sx+sign*1.75,sy+.25,1.65),.09,3.3,self.shop_props,self.m('Wood'),8)
        # Roof pitches away from the player; simple prism, deliberately low-poly.
        roof=box('Shop'+team+'_Awning',(sx,sy+.05,2.90),(3.9,2,.15),self.shop_props,self.m('Team'+team))
        roof.rotation_euler.x=.15
        for i in range(3):
            box('Shop'+team+f'_Crate{i}',(sx-1.0+i,.0+sy,1.48),(.48,.5,.32),self.shop_props,self.m('Canvas'))
        text_mesh('Shop'+team+'_ShopLabel','SHOP',(sx,sy-1.0),.5,self.labels,self.m('Canvas'))
        self.marker('Shop_'+team,(sx,sy-1.65),'shop_interaction',1,team=team)
        self.marker('Lobby_'+team,xy,'team_lobby',r,team=team)
        self.lobbies.append(dict(team=team,center_m=list(xy),radius_m=r,physical_connection=opening is not None))
        for i,dx in enumerate((-1.5,0,1.5),1):
            p=(xy[0]+dx,xy[1]-1.1)
            self.marker(f'Lobby_{team}_Player{i}',p,'lobby_spawn',.55,z=1,team=team)
            self.samples.append(p)
        for i in range(choices):
            dx=0 if choices==1 else (i-1)*2.45
            p=(xy[0]+dx,xy[1]-3.1)
            cylinder(f'Choose_{team}_{i+1}',(*p,.025),.90,.05,self.detail,self.m('Arena'+str(i+1)),32)
            ring(f'Choose_{team}_{i+1}_Rim',p,.9,1.04,.04,self.detail,self.m('Gold'),32)
            text_mesh(f'Choose_{team}_{i+1}_Text',str(i+1) if choices>1 else 'GO',p,.72,self.labels,self.m('Ink'),z=.061)
            self.marker(f'Select_{team}_Arena{i+1}',p,'arena_selection',1.0,team=team,target_arena='Arena_'+str(i+1))
            self.samples.append(p)

    def bridge(self,name,a,b,width=5):
        a,b=Vector(a),Vector(b)
        d=b-a
        length=d.length
        angle=math.atan2(d.y,d.x)
        center=(a+b)/2
        c=self.collection(name,True,(center.x,center.y,0),role='replaceable_corridor')
        box(name+'_Foundation',(center.x,center.y,-.72),(length,width+1.1,1.4),c,self.m('Basalt'),angle)
        box(name+'_Deck',(center.x,center.y,-.05),(length,width,.1),c,self.m('SandLight'),angle)
        tangent=Vector((-math.sin(angle),math.cos(angle)))
        for side in (-1,1):
            p=center+tangent*side*(width/2+.28)
            box(name+'_Rail'+str(side),(p.x,p.y,.75),(length,.5,1.5),c,self.m('Stone'),angle)
            box(name+'_RailCap'+str(side),(p.x,p.y,1.55),(length,.58,.12),c,self.m('Gold'),angle)
        for i in range(1,math.ceil(length/2)):
            p=a+d*(i/math.ceil(length/2))
            box(name+f'_Paving{i}',(p.x,p.y,.013),(.06,width-.15,.025),c,self.m('CutStone'),angle)
        for i in range(math.ceil(length)+1):
            t=i/math.ceil(length)
            p=a+d*t
            self.samples.append((p.x,p.y))
        self.paths.append(dict(name=name,start_m=list(a),end_m=list(b),clear_width_m=width))
        return length

    def build(self):
        if self.variant=='A':
            # Large outer precinct matches the first storyboard's outer circular boundary.
            outer=self.collection('Outer_Precinct',True)
            self.disk('OuterPrecinct',(0,0),40,outer,self.m('Grass'))
            # Lower the landscape 8cm to prevent overlapping floor faces; combat floors remain Z=0.
            for obj in outer.objects: obj.location.z-=.08
            self.arena('Arena_1',(0,0),20,[0,math.pi],'Gold')
            self.wall_circle('OuterPrecinct',(0,0),39.4,[],1,outer,height=1.6)
            for team,sign in [('A',-1),('B',1)]:
                xy=(sign*30,0)
                opening=0 if sign<0 else math.pi
                self.shop(team,xy,opening=opening,choices=1)
                self.bridge('Entry_'+team,(sign*19.7,0),(sign*24.4,0),4)
                self.gate('ArenaEntry_'+team,(sign*19.76,0),math.pi if sign<0 else 0,6.2,'Arena_1','arena_entry_gate')
            # Landscape-only bands fill the outer precinct; no extra combat routes implied.
            for i in range(28):
                a=i*math.tau/28
                if abs(math.sin(a))<.38: continue
                x,y=33*math.cos(a),33*math.sin(a)
                cylinder('Planter'+str(i),(x,y,.32),1.75,.64,self.detail,self.m('Basalt'),8)
                cylinder('TreeTrunk'+str(i),(x,y,1.5),.22,2.4,self.detail,self.m('Wood'),7)
                cylinder('TreeCrown'+str(i),(x,y,3.45),1.65,3.1,self.detail,self.m('Leaf'),7,top=.12)
            self.camera_target=(0,0,0)
            self.camera_scale=97
            self.top_scale=96
        else:
            h=42*math.sqrt(3)/2
            centers=[(0,2*h/3),(-21,-h/3),(21,-h/3)]
            for i,center in enumerate(centers):
                others=[j for j in range(3) if j!=i]
                angles=[math.atan2(centers[j][1]-center[1],centers[j][0]-center[0]) for j in others]
                self.arena('Arena_'+str(i+1),center,12,angles,'Arena'+str(i+1),True,i*math.tau/3)
            for i,j in [(0,1),(0,2),(1,2)]:
                a,b=Vector(centers[i]),Vector(centers[j])
                direction=(b-a).normalized()
                angle=math.atan2(direction.y,direction.x)
                # Openings have 5.8m width. Deck penetrates the circle slightly to remove corner gaps.
                p=a+direction*11.5
                q=b-direction*11.5
                name=f'Corridor_{i+1}_{j+1}'
                length=self.bridge(name,p,q,5)
                self.corridors.append(dict(name=name,arenas=[i+1,j+1],deck_length_m=length,
                                            edge_to_edge_m=18,clear_width_m=5,replaceable=True))
                for arena_index,point,rotation in [(i,p,angle),(j,q,angle+math.pi)]:
                    self.gate(f'Gate_{arena_index+1}_to_{j+1 if arena_index==i else i+1}',point,rotation,5.6,'Arena_'+str(arena_index+1))
                    self.paths.append(dict(name=f'Approach_{arena_index+1}_{name}',start_m=list(centers[arena_index]),
                                           end_m=list(point),clear_width_m=5))
            for team,sign in [('A',-1),('B',1)]:
                self.shop(team,(sign*43,24),choices=3)
            self.camera_target=(0,7,0)
            self.camera_scale=119
            self.top_scale=110
        self.setup_scene()
        self.validate()
        self.export()
        self.save_and_render()

    def setup_scene(self):
        scene=bpy.context.scene
        scene.unit_settings.system='METRIC'
        scene.unit_settings.scale_length=1
        scene.render.engine='CYCLES'
        scene.cycles.samples=24
        scene.cycles.use_denoising=True
        scene.render.threads_mode='FIXED'
        scene.render.threads=4
        scene.render.resolution_percentage=100
        scene.render.image_settings.file_format='PNG'
        scene.world.use_nodes=True
        scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.20,.27,.32,1)
        scene.world.node_tree.nodes['Background'].inputs[1].default_value=.7
        light=bpy.data.lights.new('Sun','SUN')
        light.energy=2.4
        light.angle=.22
        obj=bpy.data.objects.new('Sun',light)
        self.presentation.objects.link(obj)
        obj.rotation_euler=(.36,-.42,-.55)
        scene.view_settings.view_transform='AgX'
        self.hero=camera('Camera_Overview',(65,-106,132),self.camera_target,self.camera_scale,self.presentation)
        self.top=camera('Camera_Top',(0,self.camera_target[1],180),self.camera_target,self.top_scale,self.presentation)
        sx,sy=self.lobbies[0]['center_m']
        self.shop_camera=camera('Camera_Shop',(sx+11,sy-18,22),(sx,sy,0),21,self.presentation)
        if self.variant=='B':
            self.detail_camera=camera('Camera_Gate_Detail',(-2,-33,22),(-5,-12.12,0),31,self.presentation)
        else:
            self.detail_camera=camera('Camera_Arena_Detail',(24,-36,48),(0,0,0),51,self.presentation)
        scene.camera=self.hero
        scene.render.resolution_x=1600
        scene.render.resolution_y=1250
        for screen in bpy.data.screens:
            for area in screen.areas:
                if area.type=='VIEW_3D':
                    space=area.spaces.active
                    space.region_3d.view_distance=self.camera_scale
                    space.region_3d.view_location=self.camera_target
                    space.region_3d.view_rotation=self.hero.rotation_euler.to_quaternion()
                    space.shading.type='MATERIAL'
                    space.clip_end=1000
                    space.overlay.show_extras=False

    def validate(self):
        bpy.context.view_layer.update()
        def bvh(objects):
            vs,fs=[],[]
            for obj in objects:
                n=len(vs)
                vs.extend(obj.matrix_world@v.co for v in obj.data.vertices)
                fs.extend(tuple(n+i for i in p.vertices) for p in obj.data.polygons)
            return BVHTree.FromPolygons(vs,fs)
        solid=[o for g in self.groups if g['collision'] and g['role'] not in ('transfer_gate','arena_entry_gate')
               for o in g['collection'].objects if o.type=='MESH']
        tree=bvh(solid)
        # Sample centerlines and a capsule-clearance band across every transfer passage.
        for path in self.paths:
            a,b=Vector(path['start_m']),Vector(path['end_m'])
            d=b-a
            tangent=Vector((-d.y,d.x)).normalized()
            for i in range(math.ceil(d.length)+1):
                p=a+d*i/math.ceil(d.length)
                for offset in (-1.8,-.6,0,.6,1.8):
                    q=p+tangent*offset
                    self.samples.append((q.x,q.y))
        errors=[]
        for p in self.samples:
            hit=tree.ray_cast(Vector((*p,10)),Vector((0,0,-1)),20)[0]
            if hit is None or hit.z>.15 or hit.z<-.15:
                errors.append({'point_m':list(p),'surface_z':None if hit is None else hit.z})
        assert not errors,errors[:12]
        assert len(self.arenas)==(1 if self.variant=='A' else 3)
        assert len(self.lobbies)==2
        assert len(self.gates)==(2 if self.variant=='A' else 6)
        assert Counter(m['team'] for m in self.markers if m['kind']=='lobby_spawn')=={'A':3,'B':3}
        # Transfer gates: actual mesh ray must block at standing torso height.
        gate_checks=[]
        for gate in self.gates:
            g=next(g for g in self.groups if g['asset']==gate['asset'])
            gt=bvh(g['collection'].objects)
            angle=math.radians(gate['angle_degrees'])
            n=Vector((math.cos(angle),math.sin(angle),0))
            center=Vector(gate['position_m'])+Vector((0,0,.7))
            assert gt.ray_cast(center-n*1.5,n,3)[0] is not None,gate['name']
            gate_checks.append(gate['name'])
        self.validation={'geometry_samples':len(self.samples),'floor_and_clearance_errors':errors,
                         'gate_blocking_rays_passed':gate_checks,'gates_excluded_from_open_route_check':True,
                         'corridor_width_tested_m':3.6,'method':'Actual Blender mesh rays; not UE navigation or gameplay testing',
                         'marker_counts':dict(Counter(m['kind'] for m in self.markers))}
        access=[]
        for arena in self.arenas:
            spawn=[m for m in self.markers if m['kind']=='duel_spawn' and m['arena']==arena['name']]
            gates=[g for g in self.gates if g['arena']==arena['name']]
            distances=[sorted(math.dist(m['position_m'][:2],g['position_m'][:2]) for g in gates) for m in spawn]
            assert all(abs(a-b)<.0001 for a,b in zip(*distances)),(arena['name'],distances)
            access.append(dict(arena=arena['name'],team_A_gate_distances_m=distances[0],team_B_gate_distances_m=distances[1]))
        self.validation['mirrored_duel_gate_access']=access
        (self.out/'Geometry_Verification.json').write_text(json.dumps(self.validation,indent=2),encoding='utf-8')
        print(self.slug+'_GEOMETRY_VERIFIED '+str(len(self.samples)),flush=True)

    def export(self):
        scene=bpy.context.scene
        exports=[]
        for g in self.groups:
            originals=[o for o in g['collection'].objects if o.type=='MESH']
            if not originals: continue
            bpy.ops.object.select_all(action='DESELECT')
            copies=[]
            for source in originals:
                obj=source.copy()
                obj.data=source.data.copy()
                scene.collection.objects.link(obj)
                obj.select_set(True)
                copies.append(obj)
            bpy.context.view_layer.objects.active=copies[0]
            bpy.ops.object.join()
            obj=bpy.context.object
            obj.name=g['asset']
            scene.cursor.location=g['origin_m']
            bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
            bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
            obj.location=(0,0,0)
            uv=obj.data.uv_layers.active or obj.data.uv_layers.new(name='UVMap')
            for poly in obj.data.polygons:
                axis=max(range(3),key=lambda k:abs(poly.normal[k]))
                axes=[k for k in range(3) if k!=axis]
                for li in poly.loop_indices:
                    co=obj.data.vertices[obj.data.loops[li].vertex_index].co
                    uv.data[li].uv=(co[axes[0]]/4,co[axes[1]]/4)
            filename=g['asset']+'.fbx'
            local_min=[min(v.co[i] for v in obj.data.vertices) for i in range(3)]
            local_max=[max(v.co[i] for v in obj.data.vertices) for i in range(3)]
            bpy.ops.export_scene.fbx(filepath=str(self.out/'Exports'/filename),use_selection=True,
                apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',axis_forward='-Y',axis_up='Z',
                object_types={'MESH'},mesh_smooth_type='FACE',use_triangles=True,bake_anim=False,add_leaf_bones=False)
            exports.append({k:v for k,v in g.items() if k!='collection'} | dict(file=filename,
                local_min_m=local_min,local_max_m=local_max,triangles=sum(len(p.vertices)-2 for p in obj.data.polygons)))
            bpy.data.objects.remove(obj,do_unlink=True)
        palette={m.name:dict(rgb=list(m.diffuse_color[:3]),roughness=m.node_tree.nodes['Principled BSDF'].inputs['Roughness'].default_value,
                            emission=m.node_tree.nodes['Principled BSDF'].inputs['Emission Strength'].default_value)
                 for m in self.materials.values()}
        self.layout=dict(title=self.slug,variant=self.variant,version='0.1',units='meters',
            coordinate_convention='Blender X east, Y north, Z up. UE: X=x*100, Y=-y*100, Z=z*100.',
            art_style='low-poly spatial prototype',gameplay_implemented=False,
            arenas=self.arenas,lobbies=self.lobbies,corridors=self.corridors,gates=self.gates,
            markers=self.markers,materials=palette,exports=exports,validation=self.validation,
            ue_map='/Game/Map/L_'+self.slug,ue_assets='/Game/Environment/'+self.slug,
            camera=dict(location_m=list(self.hero.location),target_m=list(self.camera_target),ortho_width_m=self.camera_scale),
            design_notes=['Shop, selection, spawning, victory, admission and storm logic are deferred.',
                          'Gate meshes are closed static placeholders with independent center pivots.',
                          'For concept B a winner must be admitted at the destination even while its duel continues.',
                          'Concept A supports round 2 succession duels and possible round 3 team combat.',
                          'Concept B lobbies are separate islands: selection/transfer will be game logic.'])
        (self.out/'Layout.json').write_text(json.dumps(self.layout,ensure_ascii=False,indent=2),encoding='utf-8')
        print(self.slug+'_EXPORT_COMPLETE '+str(len(exports)),flush=True)

    def save_and_render(self):
        bpy.context.scene.cursor.location=(0,0,0)
        bpy.context.preferences.filepaths.save_version=0
        bpy.ops.object.select_all(action='DESELECT')
        bpy.ops.wm.save_as_mainfile(filepath=str(self.out/(self.slug+'.blend')))
        if '--no-render' in sys.argv: return
        render_views(self.out,self.hero,self.top,self.shop_camera,self.detail_camera)


def render_views(out,hero,top,shop,detail):
    scene=bpy.context.scene
    for name,cam,w,h in [('Overview',hero,1600,1250),('TopDown',top,1600,1400),
                         ('Shop',shop,1280,1000),('ArenaDetail',detail,1400,1050)]:
        scene.camera=cam
        scene.render.resolution_x=w
        scene.render.resolution_y=h
        scene.render.filepath=str(out/'Previews'/(name+'.png'))
        bpy.ops.render.render(write_still=True)
        print(str(out.name)+'_RENDERED_'+name,flush=True)


if __name__=='__main__':
    for variant in ('A','B'):
        other='B' if variant=='A' else 'A'
        if '--concept='+other in sys.argv: continue
        if '--render-only' in sys.argv:
            slug='Round02_A_CrownArena' if variant=='A' else 'Round02_B_TriadArena'
            out=ROOT/'Art'/'Maps'/slug
            bpy.ops.wm.open_mainfile(filepath=str(out/(slug+'.blend')))
            cams=bpy.data.objects
            render_views(out,cams['Camera_Overview'],cams['Camera_Top'],cams['Camera_Shop'],
                         cams['Camera_Arena_Detail' if variant=='A' else 'Camera_Gate_Detail'])
        else:
            Concept(variant).build()
    print('ROUND02_CONCEPTS_COMPLETE',flush=True)
