import unreal
import pathlib
import json
import traceback

_climb_world=unreal.EditorLevelLibrary.get_pie_worlds(False)[0]
_climb_pawn=unreal.GameplayStatics.get_player_pawn(_climb_world,0)
_climb_move=_climb_pawn.get_component_by_class(unreal.APSCharacterMovementComponent)
_climb_anim=_climb_pawn.get_component_by_class(unreal.SkeletalMeshComponent).get_anim_instance()
_climb_start=unreal.GameplayStatics.get_time_seconds(_climb_world)
_climb_samples=[]
_climb_requested=False
_climb_shots=set()
def _climb_tick(dt):
    global _climb_requested
    try:
        t=unreal.GameplayStatics.get_time_seconds(_climb_world)-_climb_start
        if t>.25 and not _climb_requested:
            _climb_pawn.climb();_climb_requested=True
        if 4.0<t<6.0 or 7.0<t<19.0:
            _climb_pawn.add_movement_input(unreal.Vector(0,0,1),1.0,False)
        elif 6.0<t<7.0:
            _climb_pawn.add_movement_input(unreal.Vector(0,0,-1),1.0,False)
        active=_climb_anim.get_current_active_montage()
        montage=active.get_name() if active else None
        loc=_climb_pawn.get_actor_location()
        sample={'t':round(t,3),'xyz':[loc.x,loc.y,loc.z],'mode':str(_climb_move.get_editor_property('movement_mode')),'custom':_climb_move.get_editor_property('custom_movement_mode'),'montage':montage,'velocity_z':_climb_pawn.get_velocity().z}
        _climb_samples.append(sample)
        shot=None
        if 2.8<t<3.5 and 'idle' not in _climb_shots:shot='idle'
        if 5<t<5.7 and 'up' not in _climb_shots:shot='up'
        if montage=='AM_ClimbToTop' and 'ledge' not in _climb_shots:shot='ledge'
        if shot:
            _climb_shots.add(shot)
            unreal.AutomationLibrary.take_high_res_screenshot(1280,720,'Climbing_'+shot+'.png')
        if t>22:
            report={'samples':_climb_samples,'observed_montages':sorted({s['montage'] for s in _climb_samples if s['montage']}),'max_height_cm':max(s['xyz'][2] for s in _climb_samples),'ended_on_top':loc.z>850 and 'MOVE_WALKING' in sample['mode']}
            pathlib.Path(unreal.Paths.project_dir(),'Saved/ClimbingSmoke.json').write_text(json.dumps(report,indent=2))
            unreal.unregister_slate_post_tick_callback(_climb_handle)
            unreal.log('CLIMBING_SMOKE '+json.dumps({k:v for k,v in report.items() if k!='samples'}))
    except Exception:
        pathlib.Path(unreal.Paths.project_dir(),'Saved/ClimbingSmoke.json').write_text(json.dumps({'error':traceback.format_exc(),'samples':_climb_samples},indent=2))
        unreal.unregister_slate_post_tick_callback(_climb_handle)
        unreal.log_error(traceback.format_exc())
_climb_handle=unreal.register_slate_post_tick_callback(_climb_tick)
