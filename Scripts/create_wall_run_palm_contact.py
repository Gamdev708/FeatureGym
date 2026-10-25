import unreal,math,json,pathlib,traceback
ROOT=pathlib.Path(unreal.Paths.project_dir());BASE='/Game/Showcase/AnimationExperiments/WallRun';E=unreal.EditorAssetLibrary;r={'success':False}
def dot(a,b):return a.x*b.x+a.y*b.y+a.z*b.z
def norm(v):return v/max(math.sqrt(dot(v,v)),1.e-8)
def mul(a,b):
 x,y,z,w=a;X,Y,Z,W=b;return (w*X+x*W+y*Z-z*Y,w*Y-x*Z+y*W+z*X,w*Z+x*Y-y*X+z*W,w*W-x*X-y*Y-z*Z)
def qt(q):return q.x,q.y,q.z,q.w
def cross(a,b):return unreal.Vector(a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x)
def rotate(q,v):
 z=mul(mul(q,(v.x,v.y,v.z,0)),(-q[0],-q[1],-q[2],q[3]));return unreal.Vector(*z[:3])
def between(a,b):
 a=norm(a);b=norm(b);c=cross(a,b);w=1+dot(a,b)
 if w<1.e-6:
  c=norm(cross(a,unreal.Vector(1,0,0) if abs(a.x)<.8 else unreal.Vector(0,1,0)));return c.x,c.y,c.z,0
 d=math.sqrt(dot(c,c)+w*w);return c.x/d,c.y/d,c.z/d,w/d
try:
 src=unreal.load_asset(BASE+'/AA_WallRun_Left_ExtendedArm');name='AA_WallRun_Left_PalmContact';path=BASE+'/'+name
 if E.does_asset_exist(path):raise RuntimeError('Palm revision already exists; will not overwrite.')
 model=src.get_editor_property('data_model_interface');names=[str(n) for n in model.get_bone_track_names()];num=model.get_number_of_keys();dur=src.get_play_length()
 ref=unreal.AnimPoseExtensions.get_reference_pose(src.get_editor_property('skeleton'))
 def get(p,n,space=unreal.AnimPoseSpaces.WORLD):return unreal.AnimPoseExtensions.get_bone_pose(p,n,space)
 handref=get(ref,'hand_l');forward=norm(get(ref,'middle_01_l').translation-handref.translation);width=get(ref,'index_01_l').translation-get(ref,'pinky_01_l').translation;width=norm(width-forward*dot(width,forward))
 q1=between(forward,unreal.Vector(0,0,1));q2=between(rotate(q1,width),unreal.Vector(0,1,0));align=mul(q2,q1);handrot=unreal.Quat(*mul(align,qt(handref.rotation)))
 comp=unreal.SkeletalMeshComponent();comp.set_skeletal_mesh_asset(unreal.load_asset('/Game/Characters/Mannequins/Meshes/SKM_Manny'))
 digits=[n for n in names if n.endswith('_l') and n.startswith(('thumb_','index_','middle_','ring_','pinky_'))];edited=['hand_l','ik_hand_l']+digits;keys={n:([],[],[]) for n in edited}
 opt=unreal.AnimPoseEvaluationOptions(evaluation_type=unreal.AnimDataEvalType.RAW,extract_root_motion=False,incorporate_root_motion_into_pose=True)
 for i in range(num):
  p=unreal.AnimPoseExtensions.get_anim_pose_at_time(src,dur*i/(num-1),opt);world=get(p,'hand_l');world.rotation=handrot
  for n in edited:
   if n in digits:t=get(ref,n,unreal.AnimPoseSpaces.LOCAL)
   else:t=world.make_relative(get(p,str(comp.get_parent_bone(n))))
   keys[n][0].append(t.translation);keys[n][1].append(t.rotation);keys[n][2].append(t.scale3d)
 for pos,rot,sc in keys.values():pos[-1]=pos[0];rot[-1]=rot[0];sc[-1]=sc[0]
 out=unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset(name,BASE,src);out.modify();ctrl=unreal.AnimSequencerController();ctrl.set_model(out.get_editor_property('data_model_interface'));ctrl.open_bracket('Open left hand and orient palm toward wall',False)
 try:
  for n,(pos,rot,sc) in keys.items():
   if not ctrl.set_bone_track_keys(n,pos,rot,sc,False):raise RuntimeError('Track write failed: '+n)
 finally:ctrl.close_bracket(False)
 assert E.save_loaded_asset(out,False)
 palmnormal=rotate(align,cross(width,forward));fingerdirection=rotate(align,forward)
 r.update(success=True,asset=path,source=src.get_path_name(),edited_tracks=edited,palm_normal=[palmnormal.x,palmnormal.y,palmnormal.z],finger_direction=[fingerdirection.x,fingerdirection.y,fingerdirection.z],notes='Reference-pose open fingers and thumb; palm faces component +X wall, fingers point up. No gameplay/plugin edits.')
except Exception:r['error']=traceback.format_exc();unreal.log_error(r['error'])
(ROOT/'Saved/WallRunPalmContactReport.json').write_text(json.dumps(r,indent=2));unreal.log('WALL_RUN_PALM '+json.dumps(r))
