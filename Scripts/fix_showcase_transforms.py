import unreal
import pathlib
import json

actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
eal=unreal.EditorAssetLibrary
marker=pathlib.Path(unreal.Paths.project_dir(),'Saved','ShowcaseTransformsFixed.json')
if not marker.exists():
    for actor in actors.get_all_level_actors():
        if str(actor.get_folder_path()).startswith('Showcase'):
            old=actor.get_actor_rotation()
            actor.set_actor_rotation(unreal.Rotator(pitch=old.roll,yaw=old.pitch,roll=old.yaw),False)
    marker.write_text(json.dumps({'corrected':True}))
for path in ['/Game/Showcase/Blueprints/BP_ShowcaseTraversal','/Game/Showcase/Blueprints/BP_ShowcaseCombat']:
    bp=unreal.load_asset(path)
    bp.modify()
    mesh=unreal.get_default_object(bp.generated_class()).get_component_by_class(unreal.SkeletalMeshComponent)
    mesh.modify()
    if path.endswith('BP_ShowcaseCombat'):
        mesh.set_anim_instance_class(unreal.load_class(None,'/Game/Showcase/Animations/ABP_ShowcaseCombat.ABP_ShowcaseCombat_C'))
    mesh.set_relative_rotation(unreal.Rotator(pitch=0,yaw=-90,roll=0),False,False)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    eal.save_loaded_asset(bp,False)
for actor in actors.get_all_level_actors():
    if actor.get_actor_label()=='Campus exposure':
        settings=actor.get_editor_property('settings')
        settings.set_editor_property('auto_exposure_min_brightness',9.0)
        settings.set_editor_property('auto_exposure_max_brightness',11.5)
        actor.set_editor_property('settings',settings)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(unreal.Vector(-10500,-12500,10500),unreal.Rotator(pitch=-30,yaw=48,roll=0))
unreal.log('SHOWCASE_TRANSFORMS_FIXED')
