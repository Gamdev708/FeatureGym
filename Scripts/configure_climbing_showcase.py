import unreal
import pathlib
import json
import traceback

ROOT=pathlib.Path(unreal.Paths.project_dir())
BASE='/AdvancedParkourSystem/Character/MannyClimbing'
r={'success':False}
try:
    path='/Game/Showcase/Blueprints/BP_ShowcaseTraversal'
    bp=unreal.load_asset(path)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    bp.modify()
    cdo=unreal.get_default_object(bp.generated_class());cdo.modify()
    movement=cdo.get_component_by_class(unreal.APSCharacterMovementComponent);movement.modify()
    for key,name in [('ClimbUpLoop','AM_ClimbUp_Loop'),('ClimbDownLoop','AM_ClimbDown_Loop'),('ClimbIdleLoop','AM_ClimbIdle_Loop')]:
        cdo.set_editor_property(key,unreal.load_asset(BASE+'/Montages/'+name))
    source=unreal.get_default_object(unreal.load_asset('/AdvancedParkourSystem/ADSCharacter').generated_class()).get_editor_property('character_movement')
    for key in ['IdleToClimbMontage','ClimbToTopMontageSettings','ClimbDownLedgeMontageSettings','HopUpMontage','HopDownMontage']:
        movement.set_editor_property(key,source.get_editor_property(key))
    unreal.EditorAssetLibrary.save_loaded_asset(bp,False)
    r['pawn']=path
    r['settings']={k:str(movement.get_editor_property(k)) for k in ['IdleToClimbMontage','ClimbToTopMontageSettings','ClimbDownLedgeMontageSettings']}
    r['loops']={k:str(cdo.get_editor_property(k)) for k in ['ClimbUpLoop','ClimbDownLoop','ClimbIdleLoop']}
    # Existing AnimGraph has a full-body DefaultSlot for traversal montages.
    abp=unreal.load_asset('/AdvancedParkourSystem/ABP_Manny_ADS')
    acdo=unreal.get_default_object(abp.generated_class())
    r['root_motion_mode']=str(acdo.get_editor_property('root_motion_mode'))
    if acdo.get_editor_property('root_motion_mode')!=unreal.RootMotionMode.ROOT_MOTION_FROM_MONTAGES_ONLY:
        abp.modify();acdo.modify()
        acdo.set_editor_property('root_motion_mode',unreal.RootMotionMode.ROOT_MOTION_FROM_MONTAGES_ONLY)
        unreal.EditorAssetLibrary.save_loaded_asset(abp,False)
    level=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    level.load_level('/Game/Showcase/Maps/L_Showcase_SystemsInMotion')
    for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        if a.get_actor_label()=='Climb instructions':
            a.text_render.set_text('CLIMB / LEDGE EXIT\nE at wall / WASD climb / Q detach\nMixamo animations retargeted to Manny')
    level.save_current_level()
    r['blueprint_status']=str(bp.get_editor_property('status'))
    if 'BS_ERROR' in r['blueprint_status']:raise RuntimeError('Traversal Blueprint compilation failed')
    r['success']=True
except Exception:
    r['error']=traceback.format_exc();unreal.log_error(r['error'])
(ROOT/'Saved/ClimbingConfiguration.json').write_text(json.dumps(r,indent=2))
unreal.log('CLIMBING_CONFIGURED '+str(r['success']))
