import unreal
import pathlib
import json
import traceback

ROOT=pathlib.Path(unreal.Paths.project_dir())
BASE='/AdvancedParkourSystem/Character/MannyClimbing'
tools=unreal.AssetToolsHelpers.get_asset_tools()
eal=unreal.EditorAssetLibrary
report={'success':False,'sequences':{},'montages':{}}
OPTIONS=unreal.AnimPoseEvaluationOptions(evaluation_type=unreal.AnimDataEvalType.RAW,extract_root_motion=False,incorporate_root_motion_into_pose=True)
def pose(a,t):return unreal.AnimPoseExtensions.get_anim_pose_at_time(a,t,OPTIONS)
def bone(p,name):return unreal.AnimPoseExtensions.get_bone_pose(p,name,unreal.AnimPoseSpaces.WORLD)
def duplicate(a,name,folder):
    path=folder+'/'+name
    return unreal.load_asset(path) if eal.does_asset_exist(path) else tools.duplicate_asset(name,folder,a)
def vec(v):return [v.x,v.y,v.z]
try:
    first_up=unreal.load_asset(BASE+'/Animations/RT_Climbing_Up_Wall')
    base_hip=bone(pose(first_up,0),'pelvis').translation
    rows=[('Climbing_Up_Wall','AA_ClimbUp_InPlace',True),('Climbing_Down_Wall','AA_ClimbDown_InPlace',True),('Climbing_To_Top','AA_ClimbToTop_RootMotion',False),('Sprint_To_Wall_Climb','AA_ClimbEntry_RootMotion',False),('Braced_Hang_To_Crouch','AA_BracedHangToCrouch_RootMotion',False)]
    outputs={}
    for srcname,name,inplace in rows:
        src=unreal.load_asset(BASE+'/Animations/RT_'+srcname)
        model=src.get_editor_property('data_model_interface')
        num=model.get_number_of_keys()
        duration=src.get_play_length()
        # Cache source poses before modifying any output animation data.
        poses=[pose(src,duration*i/(num-1)) for i in range(num)]
        hips0=bone(poses[0],'pelvis').translation
        hips1=bone(poses[-1],'pelvis').translation
        drift=hips1-hips0
        names=[str(n) for n in model.get_bone_track_names()]
        root_children=[n for n in ['pelvis','ik_foot_root','ik_hand_root'] if n in names]
        keys={n:([],[],[]) for n in ['root']+root_children}
        for i,p in enumerate(poses):
            alpha=i/(num-1)
            hips=bone(p,'pelvis').translation
            if inplace:
                offset=(hips0-base_hip)+drift*alpha
                root_location=unreal.Vector()
            else:
                offset=unreal.Vector()
                root_location=hips-hips0
            newroot=unreal.Transform()
            newroot.set_editor_property('translation',root_location)
            keys['root'][0].append(root_location)
            keys['root'][1].append(unreal.Quat())
            keys['root'][2].append(unreal.Vector(1,1,1))
            for name_bone in root_children:
                g=bone(p,name_bone)
                g.set_editor_property('translation',g.translation-offset)
                local=g.make_relative(newroot)
                keys[name_bone][0].append(local.translation)
                keys[name_bone][1].append(local.rotation)
                keys[name_bone][2].append(local.scale3d)
        out=duplicate(src,name,BASE+'/Animations')
        out.modify()
        controller=unreal.AnimSequencerController();controller.set_model(out.get_editor_property('data_model_interface'))
        controller.open_bracket('Prepare Manny climbing root motion',False)
        try:
            for n,(positions,rotations,scales) in keys.items():
                if not controller.set_bone_track_keys(n,positions,rotations,scales,False):raise RuntimeError('Cannot set '+n)
        finally:controller.close_bracket(False)
        out.set_editor_property('enable_root_motion',not inplace)
        out.set_editor_property('force_root_lock',inplace)
        out.set_editor_property('root_motion_root_lock',unreal.RootMotionRootLock.ANIM_FIRST_FRAME)
        eal.save_loaded_asset(out,False)
        outputs[srcname]=out
        report['sequences'][name]={'path':out.get_path_name(),'in_place':inplace,'authored_pelvis_displacement_cm':vec(drift),'root_motion':out.get_editor_property('enable_root_motion')}
    # A stable hanging pose is extracted from the climb cycle; no new clip is invented.
    src=outputs['Climbing_Up_Wall']
    idle=duplicate(src,'AA_ClimbIdle_Pose',BASE+'/Animations')
    names=[str(n) for n in src.get_editor_property('data_model_interface').get_bone_track_names()]
    p=pose(src,0)
    idle.modify()
    controller=unreal.AnimSequencerController();controller.set_model(idle.get_editor_property('data_model_interface'))
    controller.open_bracket('Extract stationary climb pose',False)
    try:
        controller.set_number_of_frames(unreal.FrameNumber(1),False)
        for n in names:
            t=unreal.AnimPoseExtensions.get_bone_pose(p,n,unreal.AnimPoseSpaces.LOCAL)
            if not controller.set_bone_track_keys(n,[t.translation,t.translation],[t.rotation,t.rotation],[t.scale3d,t.scale3d],False):raise RuntimeError('Idle pose track failed')
    finally:controller.close_bracket(False)
    eal.save_loaded_asset(idle,False)
    outputs['Idle']=idle
    for key,name,warp in [('Climbing_Up_Wall','AM_ClimbUp_Loop',None),('Climbing_Down_Wall','AM_ClimbDown_Loop',None),('Idle','AM_ClimbIdle_Loop',None),('Sprint_To_Wall_Climb','AM_ClimbEntry','ClimbEntryPoint'),('Climbing_To_Top','AM_ClimbToTop','ClimbTopPoint'),('Braced_Hang_To_Crouch','AM_BracedHangToCrouch','ClimbTopPoint')]:
        path=BASE+'/Montages/'+name
        if eal.does_asset_exist(path):
            montage=unreal.load_asset(path)
        else:
            fac=unreal.AnimMontageFactory()
            fac.set_editor_property('target_skeleton',outputs[key].get_editor_property('skeleton'))
            fac.set_editor_property('source_animation',outputs[key])
            montage=tools.create_asset(name,BASE+'/Montages',unreal.AnimMontage,fac)
        montage.modify()
        if warp:
            unreal.AnimationLibrary.add_animation_notify_track(montage,'ClimbWarp')
            unreal.AnimationLibrary.remove_animation_notify_events_by_track(montage,'ClimbWarp')
            notify=unreal.AnimationLibrary.add_animation_notify_state_event(montage,'ClimbWarp',0.0,max(.01,montage.get_play_length()-.05),unreal.AnimNotifyState_MotionWarping)
            modifier=unreal.RootMotionModifier_SkewWarp(outer=notify)
            for n,v in [('warp_target_name',warp),('warp_translation',True),('ignore_z_axis',False),('warp_rotation',True)]:modifier.set_editor_property(n,v)
            notify.set_editor_property('root_motion_modifier',modifier)
        eal.save_loaded_asset(montage,False)
        report['montages'][name]=montage.get_path_name()
    report['success']=True
except Exception:
    report['error']=traceback.format_exc();unreal.log_error(report['error'])
(ROOT/'Saved/ClimbingPreparationReport.json').write_text(json.dumps(report,indent=2))
unreal.log('CLIMBING_PREPARATION '+str(report['success']))
