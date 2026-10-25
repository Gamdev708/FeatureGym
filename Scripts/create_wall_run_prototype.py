import unreal,math,json,pathlib,traceback
ROOT=pathlib.Path(unreal.Paths.project_dir());BASE='/Game/Showcase/AnimationExperiments/WallRun'
r={'success':False};E=unreal.EditorAssetLibrary
# Quaternion helpers use XYZW and component-space vectors.
def mul(a,b):
 x,y,z,w=a;X,Y,Z,W=b;return (w*X+x*W+y*Z-z*Y,w*Y-x*Z+y*W+z*X,w*Z+x*Y-y*X+z*W,w*W-x*X-y*Y-z*Z)
def quat(q):return (q.x,q.y,q.z,q.w)
def uq(q):return unreal.Quat(*q)
def axis(v,deg):
 s=math.sin(math.radians(deg)/2);return (v.x*s,v.y*s,v.z*s,math.cos(math.radians(deg)/2))
def norm(v):return v/max(math.sqrt(v.x*v.x+v.y*v.y+v.z*v.z),1.e-8)
def dot(a,b):return a.x*b.x+a.y*b.y+a.z*b.z
def cross(a,b):return unreal.Vector(a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x)
def length(v):return math.sqrt(dot(v,v))
def between(a,b):
 a=norm(a);b=norm(b);c=cross(a,b);w=1+dot(a,b)
 if w<1.e-6:return axis(norm(cross(a,unreal.Vector(1,0,0) if abs(a.x)<.8 else unreal.Vector(0,1,0))),180)
 m=math.sqrt(dot(c,c)+w*w);return (c.x/m,c.y/m,c.z/m,w/m)
def rotate(q,v):
 t=mul(mul(q,(v.x,v.y,v.z,0)),(-q[0],-q[1],-q[2],q[3]));return unreal.Vector(*t[:3])
def tr(pos,rot,scale=None):
 t=unreal.Transform();t.translation=pos;t.rotation=uq(rot);t.scale3d=scale or unreal.Vector(1,1,1);return t
try:
 src=unreal.load_asset('/Game/Characters/Mannequins/Animations/Manny/MM_Run_Fwd');model=src.get_editor_property('data_model_interface');names=[str(n) for n in model.get_bone_track_names()];num=model.get_number_of_keys();duration=src.get_play_length()
 path=BASE+'/AA_WallRun_Left_Prototype'
 if E.does_asset_exist(path):raise RuntimeError('Prototype already exists; refusing to overwrite it.')
 comp=unreal.SkeletalMeshComponent();comp.set_skeletal_mesh_asset(unreal.load_asset('/Game/Characters/Mannequins/Meshes/SKM_Manny'))
 parents={n:str(comp.get_parent_bone(n)) for n in names}
 options=unreal.AnimPoseEvaluationOptions(evaluation_type=unreal.AnimDataEvalType.RAW,extract_root_motion=False,incorporate_root_motion_into_pose=True)
 poses=[unreal.AnimPoseExtensions.get_anim_pose_at_time(src,duration*i/(num-1),options) for i in range(num)]
 keys={n:([],[],[]) for n in names};metrics=[]
 for i,p in enumerate(poses):
  old={n:unreal.AnimPoseExtensions.get_bone_pose(p,n,unreal.AnimPoseSpaces.WORLD) for n in names};root=old['root'];local={n:unreal.AnimPoseExtensions.get_bone_pose(p,n,unreal.AnimPoseSpaces.LOCAL) for n in names}
  old={n:g.make_relative(root) for n,g in old.items()};hip=old['pelvis'].translation;lean=axis(unreal.Vector(0,1,0),-16)
  newhip=unreal.Vector(-9,hip.y,hip.z+3)
  base={n:tr(rotate(lean,g.translation-hip)+newhip,mul(lean,quat(g.rotation)),g.scale3d) for n,g in old.items()};base['root']=unreal.Transform()
  override={'root':base['root'],'pelvis':base['pelvis']}
  def solve(a,b,c,target,hint,foot_angle=None):
   A=base[a].translation;B=base[b].translation;C=base[c].translation;L1=length(B-A);L2=length(C-B);d=target-A;dist=max(abs(L1-L2)+.1,min(length(d),L1+L2-.2));direction=norm(d);T=A+direction*dist
   along=(L1*L1-L2*L2+dist*dist)/(2*dist);height=math.sqrt(max(0,L1*L1-along*along));bend=hint-A;plane=norm(bend-direction*dot(bend,direction));K=A+direction*along+plane*height
   qa=mul(between(B-A,K-A),quat(base[a].rotation));qb=mul(between(C-B,T-K),quat(base[b].rotation));qc=quat(base[c].rotation)
   if foot_angle is not None:qc=mul(axis(unreal.Vector(0,1,0),foot_angle),quat(old[c].rotation))
   override[a]=tr(A,qa);override[b]=tr(K,qb);override[c]=tr(T,qc)
   return T
  feet={}
  for side in ['l','r']:
   f=old['foot_'+side].translation
   # Original foot lift becomes withdrawal from the wall; stride timing is retained.
   contact=max(0.,min(1.,(f.z-12.)/32.))
   target=unreal.Vector(35.-contact*23.,f.y,hip.z-38.+(7 if side=='l' else -7)+contact*9.)
   feet[side]=solve('thigh_'+side,'calf_'+side,'foot_'+side,target,base['calf_'+side].translation+unreal.Vector(-12,38,12),-85)
  for side in ['l','r']:
   h=old['hand_'+side].translation
   target=unreal.Vector(24 if side=='l' else -35,12+h.y*.18,hip.z+(47 if side=='l' else 32)+(h.z-110)*.2)
   solve('upperarm_'+side,'lowerarm_'+side,'hand_'+side,target,base['lowerarm_'+side].translation+unreal.Vector(-12,-15,0))
  new={}
  for n in names:
   par=parents[n]
   if n in override:g=override[n]
   elif par in new:g=local[n]*new[par]
   else:g=base[n]
   if n=='ik_foot_l':g=new['foot_l']
   if n=='ik_foot_r':g=new['foot_r']
   if n=='ik_hand_l':g=new['hand_l']
   if n=='ik_hand_r':g=new['hand_r']
   new[n]=g;l=g.make_relative(new[par]) if par in new else g
   keys[n][0].append(l.translation);keys[n][1].append(l.rotation);keys[n][2].append(l.scale3d)
  metrics.append({'frame':i,'left_ankle':[feet['l'].x,feet['l'].y,feet['l'].z],'right_ankle':[feet['r'].x,feet['r'].y,feet['r'].z]})
 # Explicitly close the cycle so constant offsets introduce no seam.
 for positions,rotations,scales in keys.values():positions[-1]=positions[0];rotations[-1]=rotations[0];scales[-1]=scales[0]
 E.make_directory(BASE);out=unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset('AA_WallRun_Left_Prototype',BASE,src);out.modify();controller=unreal.AnimSequencerController();controller.set_model(out.get_editor_property('data_model_interface'));controller.open_bracket('Author left wall run prototype',False)
 try:
  for n,(positions,rotations,scales) in keys.items():
   if not controller.set_bone_track_keys(n,positions,rotations,scales,False):raise RuntimeError('Track write failed: '+n)
 finally:controller.close_bracket(False)
 out.set_editor_property('enable_root_motion',False);out.set_editor_property('force_root_lock',True);out.set_editor_property('root_motion_root_lock',unreal.RootMotionRootLock.ANIM_FIRST_FRAME)
 assert E.save_loaded_asset(out,False)
 r.update(success=True,asset=path,source=src.get_path_name(),frames=num,duration=duration,root_motion=False,wall_plane='Manny component +X (character left after mesh yaw -90)',notes='First-pass two-bone solved feet and hands; visual review required, not wired into gameplay.',samples=metrics)
except Exception:r['error']=traceback.format_exc();unreal.log_error(r['error'])
(ROOT/'Saved/WallRunPrototypeReport.json').write_text(json.dumps(r,indent=2))
unreal.log('WALL_RUN_PROTOTYPE '+json.dumps({k:v for k,v in r.items() if k!='samples'}))
