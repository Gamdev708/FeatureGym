import unreal
w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
p=unreal.GameplayStatics.get_player_character(w,0)
m=p.get_component_by_class(unreal.APSCharacterMovementComponent)
if not m.is_crouching(): getattr(m,next(n for n in dir(m) if n.startswith('toggle_crouch')))()
unreal.AutomationLibrary.take_high_res_screenshot(1280,720,'Crouch_verified.png')
