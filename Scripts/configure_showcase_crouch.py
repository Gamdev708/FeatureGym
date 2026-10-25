import unreal,json,pathlib
bp=unreal.load_asset('/Game/Showcase/Blueprints/BP_ShowcaseTraversal')
bp.modify();c=unreal.get_default_object(bp.generated_class());c.modify();m=c.get_component_by_class(unreal.CharacterMovementComponent);m.modify()
nav=m.get_editor_property('nav_agent_props');nav.set_editor_property('can_crouch',True);m.set_editor_property('nav_agent_props',nav)
m.set_editor_property('crouched_half_height',45.0);m.set_editor_property('max_walk_speed_crouched',300.0)
assert unreal.EditorAssetLibrary.save_loaded_asset(bp,False)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
