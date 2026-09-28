"""Import both low-poly round-two concepts into new Unreal levels.

Run in UnrealEditor-Cmd with PythonScriptPlugin and EditorScriptingUtilities.
Only creates the named generated maps/assets. Completed levels are preserved.
"""
import json
from pathlib import Path
from collections import Counter
import unreal

ROOT=Path(__file__).resolve().parents[1]
ASSETS=unreal.AssetToolsHelpers.get_asset_tools()
MEL=unreal.MaterialEditingLibrary
LEVELS=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
ACTORS=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
MESH_EDITOR=unreal.get_default_object(unreal.StaticMeshEditorSubsystem)


def to_ue(p):
    return (p[0]*100,-p[1]*100,p[2]*100)


def spawn(cls,name,location,folder):
    actor=ACTORS.spawn_actor_from_class(cls,unreal.Vector(*location))
    if actor is None: raise RuntimeError('Could not spawn '+name)
    actor.set_actor_label(name)
    actor.set_folder_path(folder)
    return actor


def create_materials(layout):
    materials={}
    base=layout['ue_assets']+'/Materials'
    for name,description in layout['materials'].items():
        path=base+'/'+name
        mat=unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
        if mat is None:
            mat=ASSETS.create_asset(name,base,unreal.Material,unreal.MaterialFactoryNew())
        MEL.delete_all_material_expressions(mat)
        color=MEL.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector,-320,0)
        color.set_editor_property('constant',unreal.LinearColor(*description['rgb']))
        MEL.connect_material_property(color,'',unreal.MaterialProperty.MP_BASE_COLOR)
        rough=MEL.create_material_expression(mat,unreal.MaterialExpressionConstant,-320,180)
        rough.set_editor_property('r',description['roughness'])
        MEL.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
        if description['emission']:
            glow=MEL.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector,-320,320)
            glow.set_editor_property('constant',unreal.LinearColor(*[v*description['emission'] for v in description['rgb']]))
            MEL.connect_material_property(glow,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        mat.set_editor_property('two_sided',True)
        MEL.recompile_material(mat)
        unreal.EditorAssetLibrary.save_loaded_asset(mat)
        materials[name]=mat
    return materials


def import_meshes(source,layout,materials):
    meshes={}
    for entry in layout['exports']:
        task=unreal.AssetImportTask()
        for key,value in dict(filename=str(source/'Exports'/entry['file']),destination_path=layout['ue_assets']+'/Meshes',
                              destination_name=entry['asset'],automated=True,replace_existing=True,save=False).items():
            task.set_editor_property(key,value)
        task.set_editor_property('factory',unreal.FbxFactory())
        options=unreal.FbxImportUI()
        for key,value in dict(import_mesh=True,import_as_skeletal=False,
                              mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH,
                              automated_import_should_detect_type=False,import_materials=False,import_textures=False).items():
            options.set_editor_property(key,value)
        data=options.get_editor_property('static_mesh_import_data')
        for key,value in dict(combine_meshes=True,auto_generate_collision=False,convert_scene=True,
                              convert_scene_unit=True,force_front_x_axis=False,transform_vertex_to_absolute=True,
                              generate_lightmap_u_vs=False,remove_degenerates=True).items():
            data.set_editor_property(key,value)
        data.set_editor_property('normal_import_method',unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
        task.set_editor_property('options',options)
        ASSETS.import_asset_tasks([task])
        mesh=unreal.load_asset(layout['ue_assets']+'/Meshes/'+entry['asset'])
        if not isinstance(mesh,unreal.StaticMesh): raise RuntimeError('Failed mesh '+entry['asset'])
        for i,slot in enumerate(mesh.get_editor_property('static_materials')):
            name=str(slot.get_editor_property('imported_material_slot_name'))
            if name not in materials: name=str(slot.get_editor_property('material_slot_name'))
            if name not in materials: raise RuntimeError('Unmapped material '+name)
            mesh.set_material(i,materials[name])
        build=MESH_EDITOR.get_lod_build_settings(mesh,0)
        build.set_editor_property('use_mikk_t_space',False)
        MESH_EDITOR.set_lod_build_settings(mesh,0,build)
        if entry['collision']:
            mesh.get_editor_property('body_setup').set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        unreal.EditorAssetLibrary.save_loaded_asset(mesh)
        meshes[entry['asset']]=mesh
    return meshes


def verify(source,layout):
    if not LEVELS.load_level(layout['ue_map']): raise RuntimeError('Cannot reload saved map')
    all_actors=ACTORS.get_all_level_actors()
    static={a.get_actor_label():a for a in all_actors if isinstance(a,unreal.StaticMeshActor)}
    assert len(static)==len(layout['exports']),(len(static),len(layout['exports']))
    checks=[]
    for entry in layout['exports']:
        actor=static[entry['asset']]
        mesh=actor.static_mesh_component.static_mesh
        actual=actor.get_actor_location()
        expected=to_ue(entry['origin_m'])
        assert max(abs(getattr(actual,k)-v) for k,v in zip(('x','y','z'),expected))<.2,entry['asset']
        bounds=mesh.get_bounds()
        lo=entry['local_min_m']; hi=entry['local_max_m']
        center=to_ue([(a+b)/2 for a,b in zip(lo,hi)])
        extent=[(b-a)*50 for a,b in zip(lo,hi)]
        error=max([abs(getattr(bounds.origin,k)-v) for k,v in zip(('x','y','z'),center)]+
                  [abs(getattr(bounds.box_extent,k)-v) for k,v in zip(('x','y','z'),extent)])
        assert error<1.0,(entry['asset'],error)
        expected_profile='BlockAll' if entry['collision'] else 'NoCollision'
        assert str(actor.static_mesh_component.get_collision_profile_name())==expected_profile,entry['asset']
        if entry['collision']:
            assert mesh.get_editor_property('body_setup').get_editor_property('collision_trace_flag')==unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE
        assert all(s.material_interface is not None for s in mesh.static_materials),entry['asset']
        checks.append(dict(asset=entry['asset'],bounds_error_cm=round(error,4),collision=expected_profile))
    markers={a.get_actor_label():a for a in all_actors if isinstance(a,unreal.TargetPoint)}
    assert len(markers)==len(layout['markers'])
    for marker in layout['markers']:
        actor=markers[marker['name']]
        expected=to_ue(marker['position_m'])
        actual=actor.get_actor_location()
        assert max(abs(getattr(actual,k)-v) for k,v in zip(('x','y','z'),expected))<.2
        assert 'Role='+marker['kind'] in [str(t) for t in actor.tags]
    report=dict(map=layout['ue_map'],saved_level_reloaded=True,mesh_count=len(static),
                marker_counts=dict(Counter(m['kind'] for m in layout['markers'])),
                material_collision_scale_pivot_checks=checks,
                independent_gate_actors=len(layout['gates']),
                independent_corridor_actors=len(layout['corridors']),
                gameplay_implemented=False,
                limitations='NullRHI commandlet verifies stored assets, not rendered Unreal output or gameplay.')
    (source/'Unreal_Verification.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    unreal.log('ROUND02_VERIFIED '+layout['ue_map'])


def create(source):
    layout=json.loads((source/'Layout.json').read_text(encoding='utf-8'))
    level_exists=unreal.EditorAssetLibrary.does_asset_exist(layout['ue_map'])
    if level_exists and (source/'Unreal_Import_Report.json').exists():
        unreal.log('Preserving completed level: '+layout['ue_map'])
        verify(source,layout)
        return
    if level_exists:
        if not LEVELS.load_level(layout['ue_map']): raise RuntimeError('Could not inspect existing level')
        if any(not isinstance(a,(unreal.WorldSettings,unreal.Brush)) for a in ACTORS.get_all_level_actors()):
            raise RuntimeError('Existing level contains unsummarized work; preserve it: '+layout['ue_map'])
    materials=create_materials(layout)
    meshes=import_meshes(source,layout,materials)
    if not level_exists and not LEVELS.new_level(layout['ue_map']): raise RuntimeError('Could not create new level')
    if level_exists and not LEVELS.load_level(layout['ue_map']): raise RuntimeError('Could not load empty level')
    for entry in layout['exports']:
        actor=spawn(unreal.StaticMeshActor,entry['asset'],to_ue(entry['origin_m']),'Round02/Geometry/'+entry['role'])
        c=actor.static_mesh_component
        c.set_static_mesh(meshes[entry['asset']])
        c.set_mobility(unreal.ComponentMobility.STATIC)
        c.set_collision_profile_name('BlockAll' if entry['collision'] else 'NoCollision')
        actor.set_editor_property('tags',[unreal.Name('Role='+entry['role']),unreal.Name('PrototypeOnly')])
    for marker in layout['markers']:
        actor=spawn(unreal.TargetPoint,marker['name'],to_ue(marker['position_m']),'Round02/Layout/'+marker['kind'])
        tags=['Round02Layout','Role='+marker['kind'],'RadiusMeters='+str(marker['radius_m'])]
        for key in ('team','arena','target_arena','gate_asset'):
            if key in marker: tags.append(key+'='+str(marker[key]))
        if 'rotation_degrees' in marker:
            actor.set_actor_rotation(unreal.Rotator(0,-marker['rotation_degrees'],0),False)
        actor.set_editor_property('tags',[unreal.Name(t) for t in tags])
    sun=spawn(unreal.DirectionalLight,'Round02_Sun',(0,0,10000),'Round02/Lighting')
    sun.set_actor_rotation(unreal.Rotator(-55,-35,0),False)
    sun.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sun.light_component.set_intensity(3)
    sun.light_component.set_editor_property('atmosphere_sun_light',True)
    sky=spawn(unreal.SkyLight,'Round02_SkyLight',(0,0,6000),'Round02/Lighting')
    sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sky.light_component.set_intensity(.8)
    sky.light_component.set_editor_property('real_time_capture',True)
    spawn(unreal.SkyAtmosphere,'Round02_Atmosphere',(0,0,0),'Round02/Lighting')
    post=spawn(unreal.PostProcessVolume,'Round02_Exposure',(0,0,0),'Round02/Lighting')
    post.set_editor_property('unbound',True)
    settings=post.get_editor_property('settings')
    for name in ('min','max'):
        settings.set_editor_property('override_auto_exposure_'+name+'_brightness',True)
        settings.set_editor_property('auto_exposure_'+name+'_brightness',0.0)
    post.set_editor_property('settings',settings)
    cam=layout['camera']
    view=spawn(unreal.CameraActor,'Round02_OverviewCamera',to_ue(cam['location_m']),'Round02/Presentation')
    view.set_actor_rotation(unreal.MathLibrary.make_rot_from_x(unreal.Vector(*to_ue(cam['target_m']))-view.get_actor_location()),False)
    view.camera_component.set_editor_property('projection_mode',unreal.CameraProjectionMode.ORTHOGRAPHIC)
    view.camera_component.set_editor_property('ortho_width',cam['ortho_width_m']*100)
    view.camera_component.set_editor_property('aspect_ratio',1600/1250)
    if not LEVELS.save_current_level(): raise RuntimeError('Could not save new level')
    unreal.EditorAssetLibrary.save_directory(layout['ue_assets'],only_if_is_dirty=False,recursive=True)
    (source/'Unreal_Import_Report.json').write_text(json.dumps(dict(map=layout['ue_map'],
        mesh_count=len(meshes),marker_count=len(layout['markers']),gameplay_implemented=False),indent=2),encoding='utf-8')
    verify(source,layout)


if __name__=='__main__':
    for slug in ('Round02_A_CrownArena','Round02_B_TriadArena'):
        source=ROOT/'Art'/'Maps'/slug
        command=unreal.SystemLibrary.get_command_line().lower()
        if '-round02materialsonly' in command or '-round02refreshassets' in command:
            layout=json.loads((source/'Layout.json').read_text(encoding='utf-8'))
            if not unreal.EditorAssetLibrary.does_asset_exist(layout['ue_map']):
                raise RuntimeError('Material-only refresh requires an existing concept level')
            materials=create_materials(layout)
            if '-round02refreshassets' in command:
                # Explicit refresh for generated assets, preserving level actors and their transforms.
                import_meshes(source,layout,materials)
            verify(source,layout)
        else:
            create(source)
    unreal.log('ROUND02_CONCEPTS_COMPLETE')
