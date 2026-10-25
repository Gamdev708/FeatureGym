import unreal
import json
import pathlib
import traceback

ROOT=pathlib.Path(unreal.Paths.project_dir())
BASE='/AdvancedParkourSystem/Character/MannyClimbing'
SOURCE='/AdvancedParkourSystem/Character/MixamoImport'
NAMES=['Climbing_Up_Wall','Climbing_Down_Wall','Climbing_To_Top','Sprint_To_Wall_Climb','Braced_Hang_To_Crouch']
report={'success':False,'rigs':{},'retargeted':[]}
tools=unreal.AssetToolsHelpers.get_asset_tools()
eal=unreal.EditorAssetLibrary
def load(path):
    a=unreal.load_asset(path)
    if a is None:raise RuntimeError('Required asset missing: '+path)
    return a
try:
    source_mesh=load(SOURCE+'/Y_Bot')
    target_mesh=load('/Game/Characters/Mannequins/Meshes/SKM_Manny')
    rigs=[]
    for name,mesh in [('IK_YBot_Climbing',source_mesh),('IK_Manny_Climbing',target_mesh)]:
        path=BASE+'/Retargeting/'+name
        rig=unreal.load_asset(path) if eal.does_asset_exist(path) else tools.create_asset(name,BASE+'/Retargeting',unreal.IKRigDefinition,unreal.IKRigDefinitionFactory())
        ctrl=unreal.IKRigController.get_controller(rig)
        if not ctrl.set_skeletal_mesh(mesh):raise RuntimeError('Cannot set mesh for '+name)
        if not ctrl.apply_auto_generated_retarget_definition():raise RuntimeError('Auto-characterization failed for '+name)
        report['rigs'][name]={'pelvis':str(ctrl.get_retarget_root()),'root_motion_bone':str(ctrl.get_root_motion_bone()),'chains':str(ctrl.get_retarget_chains())}
        eal.save_loaded_asset(rig,False)
        rigs.append(rig)
    path=BASE+'/Retargeting/RTG_YBot_To_Manny_Climbing'
    rtg=unreal.load_asset(path) if eal.does_asset_exist(path) else tools.create_asset('RTG_YBot_To_Manny_Climbing',BASE+'/Retargeting',unreal.IKRetargeter,unreal.IKRetargetFactory())
    ctrl=unreal.IKRetargeterController.get_controller(rtg)
    ctrl.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE,rigs[0])
    ctrl.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET,rigs[1])
    ctrl.set_preview_mesh(unreal.RetargetSourceOrTarget.SOURCE,source_mesh)
    ctrl.set_preview_mesh(unreal.RetargetSourceOrTarget.TARGET,target_mesh)
    if ctrl.get_num_retarget_ops()==0:ctrl.add_default_ops()
    ctrl.assign_ik_rig_to_all_ops(unreal.RetargetSourceOrTarget.SOURCE,rigs[0])
    ctrl.assign_ik_rig_to_all_ops(unreal.RetargetSourceOrTarget.TARGET,rigs[1])
    ctrl.auto_map_chains(unreal.AutoMapChainType.FUZZY,True)
    ctrl.auto_align_all_bones(unreal.RetargetSourceOrTarget.TARGET,unreal.RetargetAutoAlignMethod.CHAIN_TO_CHAIN)
    report['ops']=[]
    for i in range(ctrl.get_num_retarget_ops()):
        op=ctrl.get_op_controller(i)
        row={'index':i,'name':str(ctrl.get_op_name(i)),'class':op.get_class().get_name(),'methods':[n for n in dir(op) if 'setting' in n]}
        try:row['settings']=str(op.get_settings())
        except Exception:pass
        report['ops'].append(row)
    eal.save_loaded_asset(rtg,False)
    registry=unreal.AssetRegistryHelpers.get_asset_registry()
    registry.scan_paths_synchronous([SOURCE+'/Animations'],True)
    assets=[registry.get_asset_by_object_path(load(SOURCE+'/Animations/'+n).get_path_name()) for n in NAMES if not eal.does_asset_exist(BASE+'/Animations/RT_'+n)]
    if assets:
        inputs=unreal.IKRetargetBatchOperationInputs()
        for key,value in {'assets_to_retarget':assets,'source_mesh':source_mesh,'target_mesh':target_mesh,'ik_retarget_asset':rtg,'prefix':'RT_','target_path':BASE+'/Animations','include_referenced_assets':False,'overwrite_existing_files':False}.items():inputs.set_editor_property(key,value)
        result=unreal.IKRetargetBatchOperation.run_batch_retarget(inputs)
        report['batch_results']=[str(a.package_name) for a in result]
    for name in NAMES:
        a=load(BASE+'/Animations/RT_'+name)
        if a.get_editor_property('skeleton')!=target_mesh.get_editor_property('skeleton'):raise RuntimeError('Wrong output skeleton')
        report['retargeted'].append({'path':a.get_path_name(),'length':a.get_play_length(),'root_samples':[str(unreal.AnimationLibrary.get_bone_pose_for_time(a,'root',t,False)) for t in [0,a.get_play_length()/2,a.get_play_length()-.001]]})
        eal.save_loaded_asset(a,False)
    report['success']=True
except Exception:
    report['error']=traceback.format_exc()
    unreal.log_error(report['error'])
(ROOT/'Saved/ClimbingRetargetReport.json').write_text(json.dumps(report,indent=2))
unreal.log('CLIMBING_RETARGET '+str(report['success']))
