import unreal
import pathlib
import json
from editor_toolset.toolsets.blueprint import BlueprintTools
r={}
a=unreal.load_asset('/AdvancedParkourSystem/Character/MannyClimbing/Animations/RT_Climbing_To_Top')
for obj,name in [(a,'sequence'),(a.get_editor_property('controller'),'controller'),(a.get_editor_property('data_model_interface'),'model')]:
    r[name]={'methods':[n for n in dir(obj) if any(x in n for x in ['frame','key','track','bracket'])]}
for obj,name in [(a.get_editor_property('controller'),'set_bone_track_keys'),(a.get_editor_property('controller'),'resize_in_frames'),(a.get_editor_property('controller'),'set_number_of_frames')]:
    r[name]=str(getattr(obj,name,None).__doc__)
p=unreal.AnimPoseExtensions.get_anim_pose_at_time(a,0.0,unreal.AnimPoseEvaluationOptions())
pelvis=unreal.AnimPoseExtensions.get_bone_pose(p,'pelvis',unreal.AnimPoseSpaces.WORLD)
r['pelvis']=str(pelvis)
r['transform_methods']=[n for n in dir(pelvis) if 'relative' in n or 'transform' in n]
bp=unreal.load_asset('/AdvancedParkourSystem/ABP_Manny_ADS')
r['slots']=[]
for graph in BlueprintTools.list_graphs(bp):
    for node in BlueprintTools.find_nodes(graph):
        if 'Slot' in node.get_class().get_name():r['slots'].append({'node':node.get_path_name(),'settings':str(node.get_editor_property('node'))})
pathlib.Path(unreal.Paths.project_dir(),'Saved/ClimbingProcessingAPI.json').write_text(json.dumps(r,indent=2))
