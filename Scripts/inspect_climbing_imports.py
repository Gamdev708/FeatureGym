import unreal
import json
import pathlib

report={'animations':{},'api':{}}
for name in ['Climbing_Up_Wall','Climbing_Down_Wall','Climbing_To_Top','Sprint_To_Wall_Climb','Braced_Hang_To_Crouch']:
    a=unreal.load_asset('/AdvancedParkourSystem/Character/MixamoImport/Animations/'+name)
    row={'class':a.get_class().get_name(),'skeleton':a.get_editor_property('skeleton').get_path_name(),'length':a.get_play_length(),'root_motion':a.get_editor_property('enable_root_motion')}
    try:
        row['tracks']=[str(n) for n in unreal.AnimationLibrary.get_animation_track_names(a)]
        first=row['tracks'][0]
        row['first_bone']=first
        row['root_samples']=[str(unreal.AnimationLibrary.get_bone_pose_for_time(a,first,t,False)) for t in [0,a.get_play_length()/2,max(0,a.get_play_length()-.001)]]
    except Exception as e:row['pose_error']=str(e)
    report['animations'][name]=row
for clsname in ['IKRigController','IKRetargeterController','IKRetargetBatchOperation','IKRetargetBatchOperationInputs','AnimPoseExtensions','AnimPoseEvaluationOptions','AnimDataController']:
    cls=getattr(unreal,clsname,None)
    report['api'][clsname]={'available':cls is not None,'doc':str(getattr(cls,'__doc__',''))[:7000],'methods':[n for n in dir(cls) if any(w in n for w in ['auto','root','retarget','pose','track','bone','controller'])]} if cls else {}
for c,m in [('IKRetargeterController','auto_align_all_bones'),('IKRetargetBatchOperation','run_batch_retarget'),('AnimPoseExtensions','get_anim_pose_at_time')]:
    try:report['api'][c+'.'+m]=getattr(getattr(unreal,c),m).__doc__
    except Exception as e:report['api'][c+'.'+m]=str(e)
pathlib.Path(unreal.Paths.project_dir(),'Saved/ClimbingInspection.json').write_text(json.dumps(report,indent=2))
unreal.log('CLIMBING_INSPECTION_COMPLETE')
