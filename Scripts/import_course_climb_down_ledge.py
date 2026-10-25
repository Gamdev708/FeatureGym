import unreal
import pathlib,json,traceback

root=pathlib.Path(unreal.Paths.project_dir()).resolve()
name=globals().get('ANIMATION_NAME','AA_ClimbDownLedge_Adjusted')
folder='/AdvancedParkourSystem/Character/MannyClimbing/Animations'
report={'success':False,'engine':unreal.SystemLibrary.get_engine_version(),'asset':folder+'/'+name}
try:
    eal=unreal.EditorAssetLibrary
    if eal.does_asset_exist(folder+'/'+name):raise RuntimeError('Refusing to overwrite existing animation')
    skeleton=unreal.load_asset('/Game/Characters/Mannequins/Meshes/SK_Mannequin')
    if not skeleton:raise RuntimeError('Existing Manny skeleton not found')
    options=unreal.FbxImportUI()
    for key,value in [('automated_import_should_detect_type',False),('mesh_type_to_import',unreal.FBXImportType.FBXIT_ANIMATION),('import_mesh',False),('import_animations',True),('import_materials',False),('import_textures',False),('skeleton',skeleton)]:options.set_editor_property(key,value)
    data=options.get_editor_property('anim_sequence_import_data')
    data.set_editor_property('animation_length',unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
    data.set_editor_property('use_default_sample_rate',False)
    data.set_editor_property('custom_sample_rate',0)
    data.set_editor_property('remove_redundant_keys',False)
    task=unreal.AssetImportTask()
    task.filename=str(root/'Plugins/AdvancedParkourSystem/Resources/AnimationSources/Climbing'/(name+'.fbx'))
    task.destination_path=folder;task.destination_name=name;task.automated=True;task.replace_existing=False;task.save=False
    task.options=options;task.factory=unreal.FbxFactory()
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    animation=unreal.load_asset(folder+'/'+name)
    if not isinstance(animation,unreal.AnimSequence):raise RuntimeError('Animation import failed')
    if animation.get_editor_property('skeleton')!=skeleton:raise RuntimeError('Wrong skeleton assigned')
    animation.set_editor_property('enable_root_motion',True)
    animation.set_editor_property('force_root_lock',False)
    animation.set_editor_property('root_motion_root_lock',unreal.RootMotionRootLock.ANIM_FIRST_FRAME)
    poses=unreal.AnimPoseEvaluationOptions(evaluation_type=unreal.AnimDataEvalType.RAW,extract_root_motion=False,incorporate_root_motion_into_pose=True)
    report['duration']=animation.get_play_length()
    report['skeleton']=skeleton.get_path_name()
    report['samples']=[]
    for alpha in [0,.25,.5,.75,1]:
        pose=unreal.AnimPoseExtensions.get_anim_pose_at_time(animation,animation.get_play_length()*alpha,poses)
        row={'alpha':alpha}
        for bone in ['root','pelvis','hand_l','hand_r','foot_l','foot_r']:
            transform=unreal.AnimPoseExtensions.get_bone_pose(pose,bone,unreal.AnimPoseSpaces.WORLD)
            v=transform.translation;row[bone]=[v.x,v.y,v.z]
        report['samples'].append(row)
    delta=[b-a for a,b in zip(report['samples'][0]['root'],report['samples'][-1]['root'])]
    report['root_displacement_cm']=delta
    report['root_motion']=animation.get_editor_property('enable_root_motion')
    if animation.get_play_length()<=0:raise RuntimeError('Empty animation')
    if sum(v*v for v in delta)<1:raise RuntimeError('No authored root displacement; review before using root motion')
    if not eal.save_loaded_asset(animation,False):raise RuntimeError('Animation save failed')
    report['success']=True
except Exception:
    report['error']=traceback.format_exc();unreal.log_error(report['error'])
(root/'Saved'/(name+'Import.json')).write_text(json.dumps(report,indent=2))
unreal.log('COURSE_CLIMB_DOWN_LEDGE_IMPORT '+str(report['success']))
