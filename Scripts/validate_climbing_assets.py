import unreal
import pathlib
import json

base='/AdvancedParkourSystem/Character/MannyClimbing'
r={'success':True,'animations':{}}
target=unreal.load_asset('/Game/Characters/Mannequins/Meshes/SKM_Manny').get_editor_property('skeleton')
options=unreal.AnimPoseEvaluationOptions(extract_root_motion=False,incorporate_root_motion_into_pose=True)
for name,loop in [('AA_ClimbUp_InPlace',True),('AA_ClimbDown_InPlace',True),('AA_ClimbIdle_Pose',True),('AA_ClimbEntry_RootMotion',False),('AA_ClimbToTop_RootMotion',False),('AA_BracedHangToCrouch_RootMotion',False)]:
    a=unreal.load_asset(base+'/Animations/'+name)
    first=unreal.AnimPoseExtensions.get_anim_pose_at_time(a,0.0,options)
    last=unreal.AnimPoseExtensions.get_anim_pose_at_time(a,a.get_play_length(),options)
    roots=[unreal.AnimPoseExtensions.get_bone_pose(p,'root',unreal.AnimPoseSpaces.WORLD) for p in [first,last]]
    hips=[unreal.AnimPoseExtensions.get_bone_pose(p,'pelvis',unreal.AnimPoseSpaces.WORLD).translation for p in [first,last]]
    root_drift=(roots[1].translation-roots[0].translation).length()
    hip_drift=(hips[1]-hips[0]).length()
    valid=a.get_editor_property('skeleton')==target
    valid=valid and (root_drift<.01 and hip_drift<.01 and not a.get_editor_property('enable_root_motion') if loop else root_drift>10 and a.get_editor_property('enable_root_motion'))
    r['animations'][name]={'valid':valid,'root_travel_cm':root_drift,'pelvis_travel_cm':hip_drift,'skeleton':target.get_path_name()}
    r['success']=r['success'] and valid
pathlib.Path(unreal.Paths.project_dir(),'Saved/ClimbingAssetValidation.json').write_text(json.dumps(r,indent=2))
unreal.log('CLIMBING_ASSET_VALIDATION '+str(r['success']))
