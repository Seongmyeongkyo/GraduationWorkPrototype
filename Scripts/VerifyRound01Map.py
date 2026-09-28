"""Read back the saved level; verify scale, vertex colors, collision and FBX orientation.

Run with PythonScriptPlugin, EditorScriptingUtilities and ProceduralMeshComponent enabled.
Does not save or alter project assets.
"""
from pathlib import Path
import json
from collections import Counter
import unreal

root=Path(__file__).resolve().parents[1]
variant=globals().get('ROUND01_VARIANT_OVERRIDE') or ('v2' if '-round01variant=v2' in unreal.SystemLibrary.get_command_line().lower() else 'v1')
suffix='_v2' if variant=='v2' else ''
base='/Game/Environment/Round01'+suffix
source=root/'Art'/'Maps'/('Round01'+suffix)
layout=json.loads((source/'Round01_Layout.json').read_text(encoding='utf-8'))
north=layout.get('coordinate_frame')=='X_NORTH_Y_WEST'
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level('/Game/Map/L_Round01_Emberwild'+suffix)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
static=[a for a in actors if isinstance(a,unreal.StaticMeshActor)]
assert len(static)==6
counts=Counter()
for a in actors:
    for tag in a.tags:
        s=str(tag)
        if s.startswith('Role='): counts[s[5:]]+=1
assert dict(counts)=={'arena':1,'monster_camp':8 if variant=='v2' else 12,'monster_slot':24 if variant=='v2' else 36,'team_spawn':2,'player_spawn':6,'bush':16},counts
# StaticMeshEditor is not initialized as an interactive subsystem in a commandlet.
# Its read-only utility does not depend on instance state.
subsystem=unreal.get_default_object(unreal.StaticMeshEditorSubsystem)
floor=unreal.load_asset(base+'/Meshes/SM_R01_Ground_and_Trails')
assert subsystem.has_vertex_colors(floor),'Terrain lost its vertex colors'
assert abs(floor.get_bounds().box_extent.x*2-11200)<1
assert abs(floor.get_bounds().box_extent.y*2-11200)<1
for a in static:
    c=a.static_mesh_component
    mesh=c.static_mesh
    for slot in mesh.static_materials:
        assert slot.material_interface is not None,'Missing saved material'
    if 'Ground' in a.get_actor_label() or 'Cliffs' in a.get_actor_label():
        assert c.get_collision_enabled()!=unreal.CollisionEnabled.NO_COLLISION
        assert mesh.get_editor_property('body_setup').get_editor_property('collision_trace_flag')==unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE
    else:
        assert c.get_collision_enabled()==unreal.CollisionEnabled.NO_COLLISION,(a.get_actor_label(),str(c.get_collision_enabled()))

# The blue banner exists only at A's camp; read its actual imported vertex positions.
propmesh=unreal.load_asset(base+'/Meshes/SM_R01_Camps_and_Ruins')
propmesh.set_editor_property('allow_cpu_access',True)  # Transient inspection only; never saved.
flag=None
for index,slot in enumerate(propmesh.static_materials):
    if slot.material_interface.get_name()=='M_Team_A_Blue':
        vertices,*_=unreal.ProceduralMeshLibrary.get_section_from_static_mesh(propmesh,0,index)
        assert vertices,'Banner section did not retain geometry'
        flag=[sum(getattr(v,k) for v in vertices)/len(vertices) for k in ('x','y','z')]
        assert flag[0]<-4000 and (flag[1]<-4000 if north else flag[1]>4000),flag
assert flag is not None
report={'saved_level_reloaded':True,'mesh_layers':len(static),'marker_counts':dict(counts),
    'terrain_size_cm':[floor.get_bounds().box_extent.x*2,floor.get_bounds().box_extent.y*2],
    'terrain_vertex_colors':True,'materials_resolved':True,'collision_settings_verified':True,
    'team_A_banner_centroid_cm':flag,'coordinate_transform_verified':True,
    'coordinate_convention':layout['coordinate_convention'],
    'render_validation':'Blender previews inspected; Unreal commandlet used NullRHI, not a rendered or gameplay test'}
(source/'Unreal_Verification.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
unreal.log('ROUND01_VERIFIED '+json.dumps(report))
