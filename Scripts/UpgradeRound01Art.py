"""Import the detailed forest and apply the requested north-axis convention to round one.

UnrealEditor-Cmd, PythonScriptPlugin + EditorScriptingUtilities + ProceduralMeshComponent.
Existing prototypes keep their art/layout; only orientation and inspection cameras change.
Use -Round01HQRefresh to refresh this generated HQ art while preserving the saved level.
"""
import unreal
import json
import math
import runpy
import shutil
from pathlib import Path
from collections import Counter

ROOT=Path(__file__).resolve().parents[1]
COMMON=runpy.run_path(str(ROOT/'Scripts'/'CreateRound02Concepts.py'),run_name='round02_library')
LEVELS=COMMON['LEVELS'];ACTORS=COMMON['ACTORS'];ASSETS=COMMON['ASSETS'];MEL=COMMON['MEL']
spawn=COMMON['spawn'];to_ue=COMMON['to_ue'];import_meshes=COMMON['import_meshes']


def north_camera():
    existing=next((a for a in ACTORS.get_all_level_actors() if a.get_actor_label()=='Round01_NorthTopCamera'),None)
    cam=existing or spawn(unreal.CameraActor,'Round01_NorthTopCamera',(0,0,16000),'Round01/Presentation')
    cam.set_actor_location(unreal.Vector(0,0,16000),False,False)
    cam.set_actor_rotation(unreal.Rotator(pitch=-90,yaw=0,roll=0),False)
    cam.camera_component.set_editor_property('projection_mode',unreal.CameraProjectionMode.ORTHOGRAPHIC)
    cam.camera_component.set_editor_property('ortho_width',12600)
    cam.camera_component.set_editor_property('aspect_ratio',1)
    up,right=cam.get_actor_up_vector(),cam.get_actor_right_vector()
    assert up.x>.9999 and right.y>.9999,(up,right)
    # Save a north-up opening perspective. An already-open editor can retain its per-user view;
    # the explicit orthographic CameraActor always supplies the requested north-up view.
    views_saved=False
    try:
        world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        views=world.get_editor_property('editor_views')
        for index,view in enumerate(views):
            view.set_editor_property('cam_rotation',unreal.Rotator(pitch=-90,yaw=0,roll=0))
            view.set_editor_property('cam_position',unreal.Vector(0,0,14500 if index==3 else 0))
            view.set_editor_property('cam_ortho_zoom',15000)
            view.set_editor_property('cam_updated',True)
        world.set_editor_property('editor_views',views)
        views_saved=True
    except Exception as exc:
        unreal.log('North camera saved; editor-view metadata unavailable: '+str(exc))
    return dict(camera_up=[up.x,up.y,up.z],camera_right=[right.x,right.y,right.z],saved_editor_views=views_saved)


def verify(layout,folder,hq=False):
    assert LEVELS.load_level(layout['ue_map'])
    all_actors=ACTORS.get_all_level_actors()
    labels={a.get_actor_label():a for a in all_actors}
    markers={a.get_actor_label():a for a in all_actors if isinstance(a,unreal.TargetPoint)}
    for marker in layout['markers']:
        loc=markers[marker['name']].get_actor_location();expected=to_ue(marker['position_m'])
        assert max(abs(getattr(loc,k)-v) for k,v in zip(('x','y','z'),expected))<.25,marker['name']
    spawn_a=markers['Spawn_A'].get_actor_location();spawn_b=markers['Spawn_B'].get_actor_location()
    assert spawn_a.x<0 and spawn_a.y<0 and spawn_b.x>0 and spawn_b.y>0
    static=[a for a in all_actors if isinstance(a,unreal.StaticMeshActor)]
    floor_name='SM_R01HQ_Ground_and_Trails' if hq else 'SM_R01_Ground_and_Trails'
    floor=labels[floor_name].static_mesh_component.static_mesh
    bounds=floor.get_bounds()
    assert abs(bounds.box_extent.x*2-11200)<1 and abs(bounds.box_extent.y*2-11200)<1,(layout['ue_map'],bounds.origin,bounds.box_extent)
    for actor in static:
        c=actor.static_mesh_component;mesh=c.static_mesh
        assert mesh and all(s.material_interface for s in mesh.static_materials)
        solid=actor.get_actor_label() in (floor_name,'SM_R01HQ_Cliffs_and_Boundary' if hq else 'SM_R01_Cliffs_and_Boundary')
        assert str(c.get_collision_profile_name())==('BlockAll' if solid else 'NoCollision'),actor.get_actor_label()
    cam=labels['Round01_NorthTopCamera'];up,right=cam.get_actor_up_vector(),cam.get_actor_right_vector()
    assert up.x>.9999 and right.y>.9999
    report=dict(map=layout['ue_map'],reloaded=True,coordinate_convention='Unreal +X north/up, +Y east/right',
                map_size_cm=[bounds.box_extent.x*2,bounds.box_extent.y*2],static_mesh_actors=len(static),
                marker_counts=dict(Counter(m['kind'] for m in layout['markers'])),
                spawn_A_cm=[spawn_a.x,spawn_a.y,spawn_a.z],spawn_B_cm=[spawn_b.x,spawn_b.y,spawn_b.z],
                north_camera_up=[up.x,up.y,up.z],north_camera_right=[right.x,right.y,right.z],
                material_references=True,collision_profiles=True,gameplay_tested=False)
    if hq:
        assert len(static)==7+216 and len(markers)==57
        report['textures']=18;report['tree_instances']=216
        for instance in layout['instances']:
            actor=labels[instance['name']];p=actor.get_actor_location();expected=to_ue(instance['position_m'])
            assert max(abs(getattr(p,k)-v) for k,v in zip(('x','y','z'),expected))<.25
        # Check an actual imported banner section, independently from marker metadata.
        mesh=labels['SM_R01HQ_Camps_and_Ruins'].static_mesh_component.static_mesh
        mesh.set_editor_property('allow_cpu_access',True)
        for index,slot in enumerate(mesh.static_materials):
            if 'Team_A_Blue' in slot.material_interface.get_name():
                vertices,*_=unreal.ProceduralMeshLibrary.get_section_from_static_mesh(mesh,0,index)
                assert vertices
                p=[sum(getattr(v,k) for v in vertices)/len(vertices) for k in ('x','y','z')]
                assert p[0]<-3900 and p[1]<-3900,p
                report['actual_blue_banner_centroid_cm']=p
        assert 'actual_blue_banner_centroid_cm' in report
    (folder/'NorthAxis_Unreal_Verification.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    if hq:(folder/'Unreal_Verification.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    unreal.log('ROUND01_NORTH_VERIFIED '+layout['ue_map'])


def prototypes():
    for suffix in ('','_v2'):
        folder=ROOT/'Art'/'Maps'/('Round01'+suffix)
        layout=json.loads((folder/'Round01_Layout.json').read_text(encoding='utf-8'))
        assert layout['coordinate_frame']=='X_NORTH_Y_WEST'
        layout['ue_map']='/Game/Map/L_Round01_Emberwild'+suffix
        layout['ue_assets']='/Game/Environment/Round01'+suffix
        assert LEVELS.load_level(layout['ue_map'])
        world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        # FBX files bake the meter-to-centimeter factor into object transforms.
        # Prefer explicit FbxImportUI settings for these deterministic generated assets.
        unreal.SystemLibrary.execute_console_command(world,'Interchange.FeatureFlags.Import.FBX 0')
        existing=ACTORS.get_all_level_actors()
        orientation_owner=next(a for a in existing if a.get_actor_label()=='SM_R01_Ground_and_Trails')
        if 'Round01NorthAxisV1' in [str(t) for t in orientation_owner.tags]:
            bounds=orientation_owner.static_mesh_component.static_mesh.get_bounds()
            if abs(bounds.box_extent.x*2-11200)>1:
                materials={name:unreal.load_asset(layout['ue_assets']+'/Materials/'+name) for name in layout['materials']}
                import_meshes(folder,layout,materials)
            north_camera()
            assert LEVELS.save_current_level()
            verify(layout,folder)
            runpy.run_path(str(ROOT/'Scripts'/'VerifyRound01Map.py'),
                           init_globals={'ROUND01_VARIANT_OVERRIDE':'v2' if suffix else 'v1'},run_name='__main__')
            continue
        backup=ROOT/'Saved'/'Round01_BeforeNorthAxis'/('Round01'+suffix)
        backup.mkdir(parents=True,exist_ok=True)
        mapfile=ROOT/'Content'/'Map'/('L_Round01_Emberwild'+suffix+'.umap')
        if not (backup/mapfile.name).exists():shutil.copy2(mapfile,backup/mapfile.name)
        materials={name:unreal.load_asset(layout['ue_assets']+'/Materials/'+name) for name in layout['materials']}
        assert all(materials.values())
        import_meshes(folder,layout,materials)
        # FBX geometry already contains the north rotation. Rotate point actors/lights/cameras once.
        for actor in existing:
            if isinstance(actor,(unreal.StaticMeshActor,unreal.WorldSettings,unreal.Brush)):continue
            p=actor.get_actor_location();r=actor.get_actor_rotation()
            actor.set_actor_location(unreal.Vector(-p.y,p.x,p.z),False,False)
            actor.set_actor_rotation(unreal.Rotator(pitch=r.pitch,yaw=r.yaw+90,roll=r.roll),False)
        by_name={a.get_actor_label():a for a in existing}
        for marker in layout['markers']:
            actor=by_name[marker['name']]
            actor.set_actor_location(unreal.Vector(*to_ue(marker['position_m'])),False,False)
            if 'rotation_degrees' in marker:
                actor.set_actor_rotation(unreal.Rotator(pitch=0,yaw=-marker['rotation_degrees'],roll=0),False)
                tags=[str(t) for t in actor.tags if not str(t).startswith('BlenderRotationDegrees=')]
                tags.append('BlenderRotationDegrees='+str(marker['rotation_degrees']))
                actor.set_editor_property('tags',[unreal.Name(t) for t in tags])
        north_camera()
        orientation_owner.set_editor_property('tags',list(orientation_owner.tags)+[unreal.Name('Round01NorthAxisV1')])
        assert LEVELS.save_current_level()
        verify(layout,folder)
        runpy.run_path(str(ROOT/'Scripts'/'VerifyRound01Map.py'),
                       init_globals={'ROUND01_VARIANT_OVERRIDE':'v2' if suffix else 'v1'},run_name='__main__')


def hq_materials(layout,folder):
    imported={}
    for kind,channels in layout['textures'].items():
        for channel,filename in channels.items():
            task=unreal.AssetImportTask()
            for key,value in dict(filename=str(folder/'Textures'/filename),destination_path=layout['ue_assets']+'/Textures',
                                  destination_name=Path(filename).stem,automated=True,replace_existing=True,save=True).items():
                task.set_editor_property(key,value)
            ASSETS.import_asset_tasks([task])
            texture=unreal.load_asset(layout['ue_assets']+'/Textures/'+Path(filename).stem)
            assert isinstance(texture,unreal.Texture2D)
            texture.set_editor_property('srgb',channel=='BaseColor')
            if channel=='Normal':
                texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_NORMALMAP)
                texture.set_editor_property('flip_green_channel',True)
            elif channel=='Roughness':texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_MASKS)
            unreal.EditorAssetLibrary.save_loaded_asset(texture)
            imported[kind,channel]=texture
    mats={}
    for name,desc in layout['materials'].items():
        base=layout['ue_assets']+'/Materials';path=base+'/'+name
        mat=unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else ASSETS.create_asset(name,base,unreal.Material,unreal.MaterialFactoryNew())
        MEL.delete_all_material_expressions(mat)
        tint=MEL.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector,-600,0)
        tint.set_editor_property('constant',unreal.LinearColor(*desc['rgb']))
        color=tint;color_output=''
        kind=desc['texture_set']
        if kind:
            for channel,prop in [('BaseColor',unreal.MaterialProperty.MP_BASE_COLOR),('Normal',unreal.MaterialProperty.MP_NORMAL),('Roughness',unreal.MaterialProperty.MP_ROUGHNESS)]:
                sample=MEL.create_material_expression(mat,unreal.MaterialExpressionTextureSample,-800,200 if channel=='Normal' else 400 if channel=='Roughness' else 0)
                sample.set_editor_property('texture',imported[kind,channel])
                if channel=='Normal':sample.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
                elif channel=='Roughness':sample.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
                else:sample.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
                if channel=='BaseColor':
                    mult=MEL.create_material_expression(mat,unreal.MaterialExpressionMultiply,-350,0)
                    MEL.connect_material_expressions(sample,'RGB',mult,'A');MEL.connect_material_expressions(tint,'',mult,'B')
                    color=mult;color_output=''
                else:MEL.connect_material_property(sample,'RGB' if channel=='Normal' else 'R',prop)
        if desc['vertex_color']:
            vc=MEL.create_material_expression(mat,unreal.MaterialExpressionVertexColor,-600,-180)
            mult=MEL.create_material_expression(mat,unreal.MaterialExpressionMultiply,-180,0)
            MEL.connect_material_expressions(color,color_output,mult,'A');MEL.connect_material_expressions(vc,'RGB',mult,'B')
            color=mult;color_output=''
        MEL.connect_material_property(color,color_output,unreal.MaterialProperty.MP_BASE_COLOR)
        if desc['emission']:
            glow=MEL.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector,-200,300)
            glow.set_editor_property('constant',unreal.LinearColor(*[v*desc['emission'] for v in desc['rgb']]))
            MEL.connect_material_property(glow,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        mat.set_editor_property('two_sided',True)
        if desc['foliage']:
            mat.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
            subsurface=MEL.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector,-200,500)
            subsurface.set_editor_property('constant',unreal.LinearColor(.12,.22,.045))
            MEL.connect_material_property(subsurface,'',unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
        MEL.recompile_material(mat);unreal.EditorAssetLibrary.save_loaded_asset(mat);mats[name]=mat
    return mats


def high_quality():
    folder=ROOT/'Art'/'Maps'/'Round01_HQ';layout=json.loads((folder/'Round01_Layout.json').read_text(encoding='utf-8'))
    exists=unreal.EditorAssetLibrary.does_asset_exist(layout['ue_map'])
    refresh='-round01hqrefresh' in unreal.SystemLibrary.get_command_line().lower()
    if exists and (folder/'Unreal_Import_Report.json').exists() and not refresh:
        verify(layout,folder,True);return
    materials=hq_materials(layout,folder)
    meshes=import_meshes(folder,layout,materials)
    if exists:
        assert LEVELS.load_level(layout['ue_map'])
        actors=ACTORS.get_all_level_actors()
        if any(isinstance(a,unreal.StaticMeshActor) for a in actors):
            if not refresh:raise RuntimeError('Preserving existing HQ level; use explicit refresh for generated art')
            # A refresh updates shared meshes/materials; actor placement remains untouched.
            verify(layout,folder,True);return
    else:assert LEVELS.new_level(layout['ue_map'])
    for entry in layout['exports']:
        if entry['role']=='tree_prototype':continue
        actor=spawn(unreal.StaticMeshActor,entry['asset'],(0,0,0),'Round01/Environment')
        c=actor.static_mesh_component;c.set_static_mesh(meshes[entry['asset']]);c.set_mobility(unreal.ComponentMobility.STATIC)
        c.set_collision_profile_name('BlockAll' if entry['collision'] else 'NoCollision')
    for entry in layout['instances']:
        actor=spawn(unreal.StaticMeshActor,entry['name'],to_ue(entry['position_m']),'Round01/Trees')
        actor.set_actor_rotation(unreal.Rotator(pitch=0,yaw=-entry['rotation_degrees'],roll=0),False)
        actor.set_actor_scale3d(unreal.Vector(*entry['scale']))
        c=actor.static_mesh_component;c.set_static_mesh(meshes[entry['asset']]);c.set_mobility(unreal.ComponentMobility.STATIC)
        c.set_collision_profile_name('NoCollision')
    for marker in layout['markers']:
        actor=spawn(unreal.TargetPoint,marker['name'],to_ue(marker['position_m']),'Round01/Layout/'+marker['kind'])
        tags=['Role='+marker['kind'],'RadiusMeters='+str(marker['radius_m'])]
        if marker.get('team'):tags.append('Team='+marker['team'])
        if 'rotation_degrees' in marker:actor.set_actor_rotation(unreal.Rotator(pitch=0,yaw=-marker['rotation_degrees'],roll=0),False)
        actor.set_editor_property('tags',[unreal.Name(t) for t in tags])
    sun=spawn(unreal.DirectionalLight,'Round01_Sun',(0,0,12000),'Round01/Lighting')
    sun.set_actor_rotation(unreal.Rotator(pitch=-52,yaw=55,roll=0),False)
    sun.light_component.set_mobility(unreal.ComponentMobility.MOVABLE);sun.light_component.set_intensity(3.0)
    sun.light_component.set_light_color(unreal.LinearColor(1,.89,.74))
    sun.light_component.set_editor_property('atmosphere_sun_light',True)
    sky=spawn(unreal.SkyLight,'Round01_SkyLight',(0,0,6000),'Round01/Lighting')
    sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE);sky.light_component.set_intensity(.85)
    sky.light_component.set_editor_property('real_time_capture',True)
    spawn(unreal.SkyAtmosphere,'Round01_Atmosphere',(0,0,0),'Round01/Lighting')
    post=spawn(unreal.PostProcessVolume,'Round01_Exposure',(0,0,0),'Round01/Lighting');post.set_editor_property('unbound',True)
    settings=post.get_editor_property('settings')
    for key in ('min','max'):
        settings.set_editor_property('override_auto_exposure_'+key+'_brightness',True)
        settings.set_editor_property('auto_exposure_'+key+'_brightness',0.0)
    post.set_editor_property('settings',settings)
    for s in layout['spawns']:
        light=spawn(unreal.PointLight,s['name']+'_FireLight',to_ue([*s['xy'],1.6]),'Round01/Lighting')
        light.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
        light.light_component.set_light_color(unreal.LinearColor(1,.35,.045));light.light_component.set_intensity(1400)
        light.light_component.set_editor_property('attenuation_radius',700)
    view=spawn(unreal.CameraActor,'Round01_OverviewCamera',to_ue(layout['camera']['location_m']),'Round01/Presentation')
    view.set_actor_rotation(unreal.MathLibrary.make_rot_from_x(-view.get_actor_location()),False)
    view.camera_component.set_editor_property('projection_mode',unreal.CameraProjectionMode.ORTHOGRAPHIC)
    view.camera_component.set_editor_property('ortho_width',17000)
    view.camera_component.set_editor_property('aspect_ratio',1800/1550)
    view_info=north_camera()
    assert LEVELS.save_current_level()
    (folder/'Unreal_Import_Report.json').write_text(json.dumps(dict(map=layout['ue_map'],view_info=view_info,
        mesh_assets=len(meshes),tree_instances=216,textures=18,gameplay_implemented=False),indent=2),encoding='utf-8')
    verify(layout,folder,True)


prototypes()
high_quality()
unreal.log('ROUND01_ART_UPGRADE_COMPLETE')
