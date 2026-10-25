import unreal
import pathlib
import json
from editor_toolset.toolsets.blueprint import BlueprintTools

root=pathlib.Path(unreal.Paths.project_dir())
report={}
try:
    source='/ComboSystemPlugin/Character/ABP_Ninja'
    dest='/Game/Showcase/Animations/ABP_ShowcaseCombat'
    bp=unreal.load_asset(dest)
    if bp is None:
        original=unreal.load_asset(source)
        if original is None: raise RuntimeError('Cannot load original ninja animation')
        bp=unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset('ABP_ShowcaseCombat','/Game/Showcase/Animations',original)
    if bp is None: raise RuntimeError('Animation duplication failed')
    graph=BlueprintTools.get_graph(bp,'EventGraph')
    report['original_event_graph']=BlueprintTools.read_graph_dsl(graph)
    # An empty DSL document preserves unmentioned events; clear this copied
    # EventGraph explicitly before removing its obsolete variables.
    for node in BlueprintTools.find_nodes(graph):
        BlueprintTools.delete_node(node)
    for name in ['Character','MovementComponent','Velocity']:
        if name in BlueprintTools.list_variables(bp):
            BlueprintTools.remove_variable(bp,name)
    unreal.BlueprintEditorLibrary.reparent_blueprint(bp,unreal.FGShowcaseCombatAnim)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    report['animation_status']=str(bp.get_editor_property('status'))
    if 'BS_ERROR' in report['animation_status']: raise RuntimeError('Animation Blueprint failed compilation')
    unreal.EditorAssetLibrary.save_loaded_asset(bp,False)
    pawn=unreal.load_asset('/Game/Showcase/Blueprints/BP_ShowcaseCombat')
    pawn.modify()
    cdo=unreal.get_default_object(pawn.generated_class())
    cdo.get_component_by_class(unreal.SkeletalMeshComponent).modify()
    cdo.get_component_by_class(unreal.SkeletalMeshComponent).set_anim_instance_class(bp.generated_class())
    unreal.BlueprintEditorLibrary.compile_blueprint(pawn)
    unreal.EditorAssetLibrary.save_loaded_asset(pawn,False)
    report['pawn_status']=str(pawn.get_editor_property('status'))
    report['success']=True
except Exception:
    import traceback
    report['success']=False;report['error']=traceback.format_exc()
    unreal.log_error(report['error'])
(root/'Saved'/'ShowcaseAnimationRepair.json').write_text(json.dumps(report,indent=2))
