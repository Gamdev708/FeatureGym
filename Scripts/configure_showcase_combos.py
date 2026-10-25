import unreal
import pathlib
import json

root=pathlib.Path(unreal.Paths.project_dir())
report={}
for file in (root/'Plugins/ComboSystemPlugin/Content/Combos').rglob('*.uasset'):
    path='/ComboSystemPlugin/'+file.relative_to(root/'Plugins/ComboSystemPlugin/Content').with_suffix('').as_posix()
    bp=unreal.load_asset(path)
    if not isinstance(bp,unreal.Blueprint): continue
    cdo=unreal.get_default_object(bp.generated_class())
    if not isinstance(cdo,unreal.AttackActionState): continue
    bp.modify();cdo.modify()
    if cdo.get_editor_property('Damage')<=0:
        damage=40.0 if 'Heavy' in file.stem else 28.0 if 'Medium' in file.stem else 20.0
        cdo.set_editor_property('Damage',damage)
    cdo.set_editor_property('bShowAttackDebug',False)
    unreal.EditorAssetLibrary.save_loaded_asset(bp,False)
    report[path]={'damage':cdo.get_editor_property('Damage'),'charge':cdo.get_editor_property('bChargeEnabled')}
for name in ['BP_ShowcaseTraversal','BP_ShowcaseCombat']:
    bp=unreal.load_asset('/Game/Showcase/Blueprints/'+name)
    bp.modify()
    cdo=unreal.get_default_object(bp.generated_class())
    combo=cdo.get_component_by_class(unreal.FGShowcaseCombo);combo.modify()
    idle=combo.get_editor_property('DefaultState');idle.modify()
    idle.set_editor_property('RequiredInputs',[unreal.InputButtons.LIGHT_ATTACK,unreal.InputButtons.MEDIUM_ATTACK,unreal.InputButtons.HEAVY_ATTACK])
    idle.set_editor_property('RequiredStates',[unreal.ButtonState.PRESSED])
    unreal.EditorAssetLibrary.save_loaded_asset(bp,False)
(root/'Saved/ShowcaseComboConfiguration.json').write_text(json.dumps(report,indent=2))
unreal.log('SHOWCASE_COMBOS_CONFIGURED '+str(len(report)))
