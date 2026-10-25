import unreal,json,pathlib
bp=unreal.load_asset('/Game/Showcase/Blueprints/BP_ShowcaseTraversal')
c=unreal.get_default_object(bp.generated_class());m=c.get_component_by_class(unreal.CharacterMovementComponent)
r={'project':unreal.Paths.project_dir(),'nav':str(m.get_editor_property('nav_agent_props')),'crouched_half_height':m.get_editor_property('crouched_half_height'),'max_walk_speed_crouched':m.get_editor_property('max_walk_speed_crouched')}
unreal.EditorAssetLibrary.save_directory('/Game',True,True)
unreal.EditorAssetLibrary.save_directory('/AdvancedParkourSystem',True,True)
(pathlib.Path(unreal.Paths.project_dir())/'Saved/CrouchInspection.json').write_text(json.dumps(r,indent=2))
