"""Import the Blender environment and create a new Unreal map. No gameplay changes.

Run using UE 5.8 UnrealEditor-Cmd with -EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities
-run=pythonscript -script=<absolute path to this file> -unattended -nullrhi
Tested with the UE 5.8 FBX/Interchange import pipeline and the supplied exports.
Refuses to overwrite an existing level. Materials/meshes belong to this generated environment.
"""
import json
import runpy
from pathlib import Path
import unreal

ROOT=Path(__file__).resolve().parents[1]
VARIANT='v2' if '-round01variant=v2' in unreal.SystemLibrary.get_command_line().lower() else 'v1'
SUFFIX='_v2' if VARIANT=='v2' else ''
SOURCE=ROOT/'Art'/'Maps'/('Round01'+SUFFIX)
LAYOUT=json.loads((SOURCE/'Round01_Layout.json').read_text(encoding='utf-8'))
BASE='/Game/Environment/Round01'+SUFFIX
MAP='/Game/Map/L_Round01_Emberwild'+SUFFIX
existing_map=unreal.EditorAssetLibrary.does_asset_exist(MAP)
if existing_map and (SOURCE/'Unreal_Import_Report.json').exists():
    raise RuntimeError('Level already exists; preserve manual edits: '+MAP)

assets=unreal.AssetToolsHelpers.get_asset_tools()
mel=unreal.MaterialEditingLibrary
materials={}
for name,description in LAYOUT['materials'].items():
    matpath=BASE+'/Materials/'+name
    material=unreal.load_asset(matpath) if unreal.EditorAssetLibrary.does_asset_exist(matpath) else None
    if material is None:
        material=assets.create_asset(name,BASE+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(material)
    if name=='M_Terrain_Vertex':
        color=mel.create_material_expression(material,unreal.MaterialExpressionVertexColor,-320,0)
        mel.connect_material_property(color,'RGB',unreal.MaterialProperty.MP_BASE_COLOR)
    else:
        color=mel.create_material_expression(material,unreal.MaterialExpressionConstant3Vector,-320,0)
        color.set_editor_property('constant',unreal.LinearColor(*description['rgb']))
        mel.connect_material_property(color,'',unreal.MaterialProperty.MP_BASE_COLOR)
    rough=mel.create_material_expression(material,unreal.MaterialExpressionConstant,-320,180)
    rough.set_editor_property('r',description['roughness'])
    mel.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
    if description['emission']:
        glow=mel.create_material_expression(material,unreal.MaterialExpressionConstant3Vector,-320,320)
        glow.set_editor_property('constant',unreal.LinearColor(*[v*description['emission'] for v in description['rgb']]))
        mel.connect_material_property(glow,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    if name in ('M_Canvas','M_Team_A_Blue','M_Team_B_Coral') or name.startswith('M_Bush'):
        material.set_editor_property('two_sided',True)
    mel.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    materials[name]=material

meshes={}
report={'map':MAP,'meshes':[],'markers':0,'gameplay_implemented':False}
for e in LAYOUT['exports']:
    task=unreal.AssetImportTask()
    task.set_editor_property('filename',str(SOURCE/'Exports'/e['file']))
    task.set_editor_property('destination_path',BASE+'/Meshes')
    task.set_editor_property('destination_name',e['asset'])
    task.set_editor_property('automated',True)
    task.set_editor_property('replace_existing',True)
    task.set_editor_property('save',False)
    task.set_editor_property('factory',unreal.FbxFactory())
    opts=unreal.FbxImportUI()
    opts.set_editor_property('import_mesh',True)
    opts.set_editor_property('import_as_skeletal',False)
    opts.set_editor_property('mesh_type_to_import',unreal.FBXImportType.FBXIT_STATIC_MESH)
    opts.set_editor_property('automated_import_should_detect_type',False)
    opts.set_editor_property('import_materials',False)
    opts.set_editor_property('import_textures',False)
    data=opts.get_editor_property('static_mesh_import_data')
    for key,value in [('combine_meshes',True),('auto_generate_collision',False),('convert_scene',True),
            ('convert_scene_unit',True),('force_front_x_axis',False),('transform_vertex_to_absolute',True),
            ('generate_lightmap_u_vs',False),('remove_degenerates',True)]:
        data.set_editor_property(key,value)
    data.set_editor_property('vertex_color_import_option',unreal.VertexColorImportOption.REPLACE)
    data.set_editor_property('normal_import_method',unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
    task.set_editor_property('options',opts)
    assets.import_asset_tasks([task])
    mesh=unreal.load_asset(BASE+'/Meshes/'+e['asset'])
    if not isinstance(mesh,unreal.StaticMesh):
        raise RuntimeError('Static mesh import failed: '+e['asset'])
    slots=mesh.get_editor_property('static_materials')
    for i,slot in enumerate(slots):
        name=str(slot.get_editor_property('imported_material_slot_name'))
        if name not in materials: name=str(slot.get_editor_property('material_slot_name'))
        if name not in materials:
            raise RuntimeError('Unmapped material slot '+name+' on '+e['asset'])
        mesh.set_material(i,materials[name])
    body=mesh.get_editor_property('body_setup')
    if VARIANT=='v2':
        # Solid-color prototype materials do not use tangent-space normal maps.
        # Built-in tangents avoid MikkTSpace sensitivity on thin cliff cap triangles.
        editor=unreal.get_default_object(unreal.StaticMeshEditorSubsystem)
        build=editor.get_lod_build_settings(mesh,0)
        build.set_editor_property('use_mikk_t_space',False)
        editor.set_lod_build_settings(mesh,0,build)
    if e['collision']:
        body.set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    meshes[e['asset']]=mesh
    bounds=mesh.get_bounds()
    report['meshes'].append({'name':e['asset'],'material_slots':len(slots),
        'extent_cm':[bounds.box_extent.x,bounds.box_extent.y,bounds.box_extent.z],'collision':e['collision']})
groundbounds=meshes['SM_R01_Ground_and_Trails'].get_bounds()
if not (5590<groundbounds.box_extent.x<5610 and 5590<groundbounds.box_extent.y<5610):
    raise RuntimeError('Import scale is wrong; expected a 11200 cm square.')

levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if existing_map:
    # Recover only an empty level left by an interrupted generation, never a finished map.
    if not levels.load_level(MAP): raise RuntimeError('Could not load incomplete level')
    for actor in actors.get_all_level_actors():
        if not isinstance(actor,(unreal.WorldSettings,unreal.Brush)):
            raise RuntimeError('Incomplete level has actors; preserve it for manual review.')
else:
    if not levels.new_level(MAP): raise RuntimeError('Could not create new level')
def spawn(cls,name,location,folder):
    obj=actors.spawn_actor_from_class(cls,unreal.Vector(*location))
    if obj is None: raise RuntimeError('Could not spawn '+name)
    obj.set_actor_label(name)
    obj.set_folder_path(folder)
    return obj

for e in LAYOUT['exports']:
    obj=spawn(unreal.StaticMeshActor,e['asset'],(0,0,0),'Round01/Environment')
    component=obj.static_mesh_component
    component.set_static_mesh(meshes[e['asset']])
    component.set_mobility(unreal.ComponentMobility.STATIC)
    if e['collision']:
        component.set_collision_profile_name('BlockAll')
    else:
        component.set_collision_profile_name('NoCollision')

def to_ue(p): return (p[0]*100,-p[1]*100,p[2]*100)
for m in LAYOUT['markers']:
    obj=spawn(unreal.TargetPoint,m['name'],to_ue(m['position_m']),'Round01/Layout/'+m['kind'])
    tags=['Round01Layout','Role='+m['kind'],'RadiusMeters='+str(m['radius_m'])]
    if m.get('team'): tags.append('Team='+m['team'])
    if m.get('half_extents_m'): tags.append('HalfExtentsMeters='+','.join(str(v) for v in m['half_extents_m']))
    if m.get('rotation_degrees') is not None: tags.append('BlenderRotationDegrees='+str(m['rotation_degrees']))
    obj.set_editor_property('tags',[unreal.Name(t) for t in tags])
    report['markers']+=1

sun=spawn(unreal.DirectionalLight,'Round01_Sun',(0,0,12000),'Round01/Lighting')
sun.set_actor_rotation(unreal.Rotator(-55,-35,0),False)
sun.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
sun.light_component.set_intensity(3.0)
sun.light_component.set_editor_property('atmosphere_sun_light',True)
sky=spawn(unreal.SkyLight,'Round01_SkyLight',(0,0,6000),'Round01/Lighting')
sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
sky.light_component.set_intensity(.8)
sky.light_component.set_editor_property('real_time_capture',True)
spawn(unreal.SkyAtmosphere,'Round01_Atmosphere',(0,0,0),'Round01/Lighting')
post=spawn(unreal.PostProcessVolume,'Round01_Exposure',(0,0,0),'Round01/Lighting')
post.set_editor_property('unbound',True)
settings=post.get_editor_property('settings')
settings.set_editor_property('override_auto_exposure_min_brightness',True)
settings.set_editor_property('override_auto_exposure_max_brightness',True)
settings.set_editor_property('auto_exposure_min_brightness',0.0)
settings.set_editor_property('auto_exposure_max_brightness',0.0)
post.set_editor_property('settings',settings)
for s in LAYOUT['spawns']:
    light=spawn(unreal.PointLight,s['name']+'_FireLight',to_ue([*s['xy'],1.6]),'Round01/Lighting')
    light.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    light.light_component.set_light_color(unreal.LinearColor(1,.40,.09))
    light.light_component.set_intensity(1000)
    light.light_component.set_editor_property('attenuation_radius',650)

# Editor inspection camera; leaves the project's game mode and default maps unchanged.
view=spawn(unreal.CameraActor,'Round01_OverviewCamera',(10400,13600,16000),'Round01/Presentation')
direction=unreal.Vector(0,0,0)-view.get_actor_location()
view.set_actor_rotation(unreal.MathLibrary.make_rot_from_x(direction),False)
view.camera_component.set_editor_property('projection_mode',unreal.CameraProjectionMode.ORTHOGRAPHIC)
view.camera_component.set_editor_property('ortho_width',16300)
view.camera_component.set_editor_property('aspect_ratio',1600/1400)
# A commandlet has no interactive viewport; the saved CameraActor is the inspection view.
if not levels.save_current_level(): raise RuntimeError('Could not save level')
unreal.EditorAssetLibrary.save_directory(BASE,only_if_is_dirty=False,recursive=True)
(SOURCE/'Unreal_Import_Report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
unreal.log('ROUND01_MAP_CREATED '+MAP)
if VARIANT=='v2':
    runpy.run_path(str(ROOT/'Scripts'/'VerifyRound01Map.py'),run_name='__main__')
