import unreal
import pathlib
import json

_fg_world=unreal.EditorLevelLibrary.get_pie_worlds(False)[0]
_fg_pawn=unreal.GameplayStatics.get_player_pawn(_fg_world,0)
_fg_combo=_fg_pawn.get_component_by_class(unreal.FGShowcaseCombo)
_fg_start=unreal.GameplayStatics.get_time_seconds(_fg_world)
_fg_samples=[]
_fg_pressed=False
_fg_released=False
def _fg_sample(dt):
    global _fg_pressed,_fg_released
    t=unreal.GameplayStatics.get_time_seconds(_fg_world)-_fg_start
    if t>.1 and not _fg_pressed:
        _fg_combo.attack_pressed(unreal.InputButtons.LIGHT_ATTACK);_fg_pressed=True
    if t>.5 and not _fg_released:
        _fg_combo.attack_released(unreal.InputButtons.LIGHT_ATTACK);_fg_released=True
    targets=[{'target':a.get_actor_label(),'readout':str(a.get_editor_property('Readout').get_editor_property('text'))} for a in unreal.GameplayStatics.get_all_actors_of_class(_fg_world,unreal.FGShowcaseTarget)]
    _fg_samples.append({'seconds':round(t,2),'attack':_fg_combo.is_in_attack_state(),'targets':targets})
    if t>2.5:
        hit=any('100 HP' != a['readout'] for s in _fg_samples for a in s['targets'])
        report={'pawn':_fg_pawn.get_class().get_name(),'combat_mode':_fg_pawn.get_editor_property('bCombatMode'),'target_damage_observed':hit,'attack_state_observed':any(s['attack'] for s in _fg_samples),'samples':_fg_samples}
        pathlib.Path(unreal.Paths.project_dir(),'Saved','ShowcaseCombatSmoke.json').write_text(json.dumps(report,indent=2))
        unreal.unregister_slate_post_tick_callback(_fg_handle)
        unreal.log('SHOWCASE_COMBAT_SMOKE '+str(hit))
_fg_handle=unreal.register_slate_post_tick_callback(_fg_sample)
