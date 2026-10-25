import unreal,json,pathlib
src=unreal.load_asset('/Game/Characters/Mannequins/Animations/Manny/MM_Run_Fwd')
model=src.get_editor_property('data_model_interface')
opt=unreal.AnimPoseEvaluationOptions(evaluation_type=unreal.AnimDataEvalType.RAW,extract_root_motion=False,incorporate_root_motion_into_pose=True)
r={'source':src.get_path_name(),'duration':src.get_play_length(),'keys':model.get_number_of_keys(),'tracks':[str(n) for n in model.get_bone_track_names()],'skeleton':src.get_editor_property('skeleton').get_path_name(),'poses':[]}
for t in [0,src.get_play_length()*.25,src.get_play_length()*.5,src.get_play_length()*.75,src.get_play_length()]:
 p=unreal.AnimPoseExtensions.get_anim_pose_at_time(src,t,opt);row={'time':t}
 for n in ['root','pelvis','spine_01','spine_03','thigh_l','calf_l','foot_l','ball_l','thigh_r','calf_r','foot_r','upperarm_l','lowerarm_l','hand_l','hand_r']:
  tr=unreal.AnimPoseExtensions.get_bone_pose(p,n,unreal.AnimPoseSpaces.WORLD);v=tr.translation;row[n]={'xyz':[v.x,v.y,v.z],'rotation':str(tr.rotation.rotator())}
 r['poses'].append(row)
(pathlib.Path(unreal.Paths.project_dir())/'Saved/WallRunInspection.json').write_text(json.dumps(r,indent=2))
