import unreal,json,pathlib,math,traceback
ROOT=pathlib.Path(unreal.Paths.project_dir());BASE='/Game/Showcase/AnimationExperiments/WallRun';E=unreal.EditorAssetLibrary;r={'success':False}
def qt(q):return q.x,q.y,q.z,q.w
def mul(a,b):
 x,y,z,w=a;X,Y,Z,W=b;return (w*X+x*W+y*Z-z*Y,w*Y-x*Z+y*W+z*X,w*Z+x*Y-y*X+z*W,w*W-x*X-y*Y-z*Z)
def inv(q):return -q[0],-q[1],-q[2],q[3]
def mq(q):return q[0],-q[1],-q[2],q[3]
def mv(v):return unreal.Vector(-v.x,v.y,v.z)
def length(v):return math.sqrt(v.x*v.x+v.y*v.y+v.z*v.z)
def swap(n):
 if n.endswith('_l'):return n[:-2]+'_r'
 if n.endswith('_r'):return n[:-2]+'_l'
 return {'L':'R','R':'L','Left':'Right','Right':'Left'}.get(n,n)
def get(p,n,space=unreal.AnimPoseSpaces.WORLD):return unreal.AnimPoseExtensions.get_bone_pose(p,n,space)
try:
 src=unreal.load_asset(BASE+'/AA_WallRun_Left_PalmContact');name='AA_WallRun_Right_PalmContact';path=BASE+'/'+name
 if E.does_asset_exist(path):raise RuntimeError('Right animation already exists; will not overwrite.')
 model=src.get_editor_property('data_model_interface');names=[str(n) for n in model.get_bone_track_names()];num=model.get_number_of_keys();dur=src.get_play_length()
 ref=unreal.AnimPoseExtensions.get_reference_pose(src.get_editor_property('skeleton'));comp=unreal.SkeletalMeshComponent();comp.set_skeletal_mesh_asset(unreal.load_asset('/Game/Characters/Mannequins/Meshes/SKM_Manny'));parents={n:str(comp.get_parent_bone(n)) for n in names}
 corrections={n:mul(inv(mq(qt(get(ref,swap(n)).rotation))),qt(get(ref,n).rotation)) for n in names}
 options=unreal.AnimPoseEvaluationOptions(evaluation_type=unreal.AnimDataEvalType.RAW,extract_root_motion=False,incorporate_root_motion_into_pose=True);keys={n:([],[],[]) for n in names}
 for i in range(num):
  pose=unreal.AnimPoseExtensions.get_anim_pose_at_time(src,dur*i/(num-1),options);world={}
  for n in names:
   original=get(pose,swap(n));t=unreal.Transform();t.translation=mv(original.translation);t.rotation=unreal.Quat(*mul(mq(qt(original.rotation)),corrections[n]));t.scale3d=original.scale3d;world[n]=t
  for n in names:
   parent=parents[n];t=world[n].make_relative(world[parent]) if parent in world else world[n];keys[n][0].append(t.translation);keys[n][1].append(t.rotation);keys[n][2].append(t.scale3d)
 for pos,rot,scale in keys.values():pos[-1]=pos[0];rot[-1]=rot[0];scale[-1]=scale[0]
 out=unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset(name,BASE,src);out.modify();ctrl=unreal.AnimSequencerController();ctrl.set_model(out.get_editor_property('data_model_interface'));ctrl.open_bracket('Bake right-side wall run mirror',False)
 try:
  for n,(pos,rot,scale) in keys.items():
   if not ctrl.set_bone_track_keys(n,pos,rot,scale,False):raise RuntimeError('Track write failed: '+n)
 finally:ctrl.close_bracket(False)
 # Swap foot sync labels as well as the pose.
 markers=unreal.AnimationLibrary.get_animation_sync_markers(src);tracks=unreal.AnimationLibrary.get_animation_notify_track_names(src);unreal.AnimationLibrary.remove_all_animation_sync_markers(out);marker_rows=[]
 for track in tracks:
  for marker in unreal.AnimationLibrary.get_animation_sync_markers_for_track(src,track):
   label=swap(str(marker.get_editor_property('marker_name')));time=marker.get_editor_property('time')
   unreal.AnimationLibrary.add_animation_sync_marker(out,label,time,track);marker_rows.append({'name':label,'time':time})
 out.set_editor_property('enable_root_motion',False);out.set_editor_property('force_root_lock',True)
 assert E.save_loaded_asset(out,False)
 # Check the stored pose data, not just the intended keys.
 root_max=0;mirror_error=0;seam={}
 first=unreal.AnimPoseExtensions.get_anim_pose_at_time(out,0,options);last=unreal.AnimPoseExtensions.get_anim_pose_at_time(out,dur,options)
 for n in ['root','pelvis','foot_l','foot_r','hand_l','hand_r']:seam[n]=length(get(first,n).translation-get(last,n).translation)
 for i in range(num):
  a=unreal.AnimPoseExtensions.get_anim_pose_at_time(src,dur*i/(num-1),options);b=unreal.AnimPoseExtensions.get_anim_pose_at_time(out,dur*i/(num-1),options);root_max=max(root_max,length(get(b,'root').translation))
  for n in ['pelvis','foot_l','foot_r','hand_l','hand_r']:mirror_error=max(mirror_error,length(get(b,n).translation-mv(get(a,swap(n)).translation)))
 r.update(success=root_max<.01 and max(seam.values())<.01 and mirror_error<.1,asset=path,source=src.get_path_name(),root_max_travel_cm=root_max,loop_seam_error_cm=seam,max_mirrored_position_error_cm=mirror_error,frames=num,sync_markers=marker_rows,wall_side='Right: component -X',notes='Reference-pose corrected skeletal reflection; extended open palm swaps to right hand. Not connected to gameplay.')
except Exception:r['error']=traceback.format_exc();unreal.log_error(r['error'])
(ROOT/'Saved/WallRunRightPalmContactReport.json').write_text(json.dumps(r,indent=2));unreal.log('RIGHT_WALL_RUN '+json.dumps(r))

