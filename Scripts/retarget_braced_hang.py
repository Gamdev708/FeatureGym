import unreal
import pathlib,json,traceback

root=pathlib.Path(unreal.Paths.project_dir())
base='/AdvancedParkourSystem/Character/MannyClimbing'
source='/AdvancedParkourSystem/Character/MixamoImport/Animations'
names=['Idle_To_Braced_Hang','Braced_Hang_Shimmy_Left','Braced_Hang_Shimmy_Right','Braced_Hang_Drop']
r={'success':False,'animations':[]}
try:
    registry=unreal.AssetRegistryHelpers.get_asset_registry()
    registry.scan_paths_synchronous([source],True)
    source_mesh=unreal.load_asset('/AdvancedParkourSystem/Character/MixamoImport/Y_Bot')
    target_mesh=unreal.load_asset('/Game/Characters/Mannequins/Meshes/SKM_Manny')
    retargeter=unreal.load_asset(base+'/Retargeting/RTG_YBot_To_Manny_Climbing')
    if not all([source_mesh,target_mesh,retargeter]):raise RuntimeError('Missing existing retargeting setup')
    inputs=[]
    for name in names:
        a=unreal.load_asset(source+'/'+name)
        if not a:raise RuntimeError('Missing imported animation '+name)
        r['animations'].append({'source':a.get_path_name(),'source_skeleton':a.get_editor_property('skeleton').get_path_name(),'duration':a.get_play_length()})
        if not unreal.EditorAssetLibrary.does_asset_exist(base+'/Animations/RT_'+name):
            inputs.append(registry.get_asset_by_object_path(a.get_path_name()))
    if inputs:
        batch=unreal.IKRetargetBatchOperationInputs()
        for key,value in {'assets_to_retarget':inputs,'source_mesh':source_mesh,'target_mesh':target_mesh,'ik_retarget_asset':retargeter,'prefix':'RT_','target_path':base+'/Animations','include_referenced_assets':False,'overwrite_existing_files':False}.items():batch.set_editor_property(key,value)
        unreal.IKRetargetBatchOperation.run_batch_retarget(batch)
    options=unreal.AnimPoseEvaluationOptions(evaluation_type=unreal.AnimDataEvalType.RAW,extract_root_motion=False,incorporate_root_motion_into_pose=True)
    for name,row in zip(names,r['animations']):
        a=unreal.load_asset(base+'/Animations/RT_'+name)
        if not a or a.get_editor_property('skeleton')!=target_mesh.get_editor_property('skeleton'):raise RuntimeError('Retarget failed '+name)
        if not unreal.EditorAssetLibrary.save_loaded_asset(a,False):raise RuntimeError('Save failed '+name)
        row['output']=a.get_path_name()
        row['samples']=[]
        for t in [0,a.get_play_length()]:
            pose=unreal.AnimPoseExtensions.get_anim_pose_at_time(a,t,options)
            sample={}
            for b in ['root','pelvis','hand_l','hand_r']:
                v=unreal.AnimPoseExtensions.get_bone_pose(pose,b,unreal.AnimPoseSpaces.WORLD).translation
                sample[b]=[v.x,v.y,v.z]
            row['samples'].append(sample)
    r['success']=True
except Exception:
    r['error']=traceback.format_exc();unreal.log_error(r['error'])
(root/'Saved/BracedHangRetargetReport.json').write_text(json.dumps(r,indent=2))
unreal.log('BRACED_HANG_RETARGET '+str(r['success']))
