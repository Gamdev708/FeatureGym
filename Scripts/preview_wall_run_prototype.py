import unreal,json,pathlib,math
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
p='/Game/Showcase/AnimationExperiments/WallRun/AA_WallRun_Left_Prototype';a=unreal.load_asset(p)
opt=unreal.AnimPoseEvaluationOptions(evaluation_type=unreal.AnimDataEvalType.RAW,extract_root_motion=False,incorporate_root_motion_into_pose=True)
first=unreal.AnimPoseExtensions.get_anim_pose_at_time(a,0,opt);last=unreal.AnimPoseExtensions.get_anim_pose_at_time(a,a.get_play_length(),opt)
def length(v):return math.sqrt(v.x*v.x+v.y*v.y+v.z*v.z)
r={'asset':p,'skeleton':a.get_editor_property('skeleton').get_path_name(),'root_motion_enabled':a.get_editor_property('enable_root_motion'),'seam_error_cm':{},'root_max_travel_cm':0}
for n in ['root','pelvis','foot_l','foot_r','hand_l','hand_r']:
 x=unreal.AnimPoseExtensions.get_bone_pose(first,n,unreal.AnimPoseSpaces.WORLD);y=unreal.AnimPoseExtensions.get_bone_pose(last,n,unreal.AnimPoseSpaces.WORLD);r['seam_error_cm'][n]=length(x.translation-y.translation)
for i in range(58):
 pose=unreal.AnimPoseExtensions.get_anim_pose_at_time(a,a.get_play_length()*i/57,opt);v=unreal.AnimPoseExtensions.get_bone_pose(pose,'root',unreal.AnimPoseSpaces.WORLD).translation;r['root_max_travel_cm']=max(r['root_max_travel_cm'],length(v))
r['success']=r['root_max_travel_cm']<.01 and max(r['seam_error_cm'].values())<.01 and not r['root_motion_enabled']
(pathlib.Path(unreal.Paths.project_dir())/'Saved/WallRunPrototypeValidation.json').write_text(json.dumps(r,indent=2))
unreal.get_editor_subsystem(unreal.AssetEditorSubsystem).open_editor_for_assets([a])


