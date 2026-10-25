import unreal
import pathlib
import json
from editor_toolset.toolsets.blueprint import BlueprintTools

report={}
eal=unreal.EditorAssetLibrary
# The copied animation asset also needs its obsolete test-character dependency
# removed so editor-wide Blueprint compilation cannot block PIE.
for path in ['/ComboSystemPlugin/Character/ABP_Ninja','/Game/Showcase/Animations/ABP_ShowcaseCombat']:
    bp=unreal.load_asset(path)
    bp.modify()
    graph=BlueprintTools.get_graph(bp,'EventGraph')
    for node in BlueprintTools.find_nodes(graph): BlueprintTools.delete_node(node)
    for name in ['Character','MovementComponent','Velocity']:
        if name in BlueprintTools.list_variables(bp): BlueprintTools.remove_variable(bp,name)
    unreal.BlueprintEditorLibrary.reparent_blueprint(bp,unreal.FGShowcaseCombatAnim)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    status=str(bp.get_editor_property('status'))
    if 'BS_ERROR' in status: raise RuntimeError(path+' did not compile')
    eal.save_loaded_asset(bp,False)
    report[path]=status
old=unreal.load_asset('/Game/Blueprints/BP_TestChar')
if old:
    old.modify()
    unreal.BlueprintEditorLibrary.reparent_blueprint(old,unreal.FGShowcaseCharacter)
    unreal.BlueprintEditorLibrary.compile_blueprint(old)
    eal.save_loaded_asset(old,False)
    report['legacy_test_character']=str(old.get_editor_property('status'))
for name,combat in [('BP_ShowcaseTraversal',False),('BP_ShowcaseCombat',True)]:
    bp=unreal.load_asset('/Game/Showcase/Blueprints/'+name)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    bp.modify()
    cdo=unreal.get_default_object(bp.generated_class());cdo.modify()
    cdo.set_editor_property('bCombatMode',combat)
    mesh=cdo.get_component_by_class(unreal.SkeletalMeshComponent);mesh.modify()
    path='/Game/Showcase/Animations/ABP_ShowcaseCombat.ABP_ShowcaseCombat_C' if combat else '/AdvancedParkourSystem/ABP_Manny_ADS.ABP_Manny_ADS_C'
    mesh.set_editor_property('anim_class',unreal.load_class(None,path))
    mesh.set_relative_rotation(unreal.Rotator(pitch=0,yaw=-90,roll=0),False,False)
    eal.save_loaded_asset(bp,False)
    report[name]={'anim':str(mesh.get_editor_property('anim_class')),'rotation':str(mesh.get_relative_transform()),'combat':combat}
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
pathlib.Path(unreal.Paths.project_dir(),'Saved','ShowcaseFinalization.json').write_text(json.dumps(report,indent=2))
unreal.log('SHOWCASE_FINALIZATION '+json.dumps(report))
