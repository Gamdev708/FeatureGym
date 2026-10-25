import unreal,json,pathlib,traceback
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
p=unreal.GameplayStatics.get_player_character(world,0);m=p.get_component_by_class(unreal.APSCharacterMovementComponent);cap=p.get_component_by_class(unreal.CapsuleComponent)
rows={};elapsed=0.;stage=0;roof=None
if m.is_crouching(): getattr(m,next(n for n in dir(m) if n.startswith('toggle_crouch')))()

def sample(name):
    rows[name]={'crouched':m.is_crouching(),'half_height':cap.get_unscaled_capsule_half_height(),'mode':str(m.movement_mode),'custom':m.custom_movement_mode}

def tick(dt):
    global elapsed,stage,roof
    elapsed+=dt
    try:
        if stage==0 and elapsed>.5:
            getattr(m,next(n for n in dir(m) if n.startswith("toggle_crouch")))();stage=1
        elif stage==1 and elapsed>1:
            sample('idle_crouch');getattr(m,next(n for n in dir(m) if n.startswith("toggle_crouch")))();stage=2
        elif stage==2 and elapsed>1.5:
            sample('standing');m.velocity=unreal.Vector(200,0,0);getattr(m,next(n for n in dir(m) if n.startswith("toggle_crouch")))();stage=3
        elif stage==3 and elapsed>2:
            sample('walking_crouch');m.stop_movement_immediately()
            loc=p.get_actor_location();loc.z+=90
            tf=unreal.Transform(location=loc)
            roof=next(a for a in unreal.GameplayStatics.get_all_actors_of_class(world,unreal.StaticMeshActor) if a.get_actor_location().x > 4000)
            mesh=roof.static_mesh_component;mesh.set_mobility(unreal.ComponentMobility.MOVABLE);mesh.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'));mesh.set_collision_profile_name('BlockAll');roof.set_actor_scale3d(unreal.Vector(3,3,.3));roof.set_actor_location(loc,False,False)
            getattr(m,next(n for n in dir(m) if n.startswith("toggle_crouch")))();stage=4
        elif stage==4 and elapsed>2.5:
            sample('blocked_stand');roof.destroy_actor();stage=5
        elif stage==5 and elapsed>3:
            sample('clearance_restored');m.velocity=unreal.Vector(800,0,0);getattr(m,next(n for n in dir(m) if n.startswith("toggle_crouch")))();stage=6
        elif stage==6 and elapsed>3.15:
            sample('sprint_slide');stage=7
        elif stage==7 and elapsed>5.5:
            sample('slide_finished')
            rows['success']=all([rows['idle_crouch']['crouched'],rows['walking_crouch']['crouched'],not rows['standing']['crouched'],rows['blocked_stand']['crouched'],not rows['clearance_restored']['crouched'],rows['sprint_slide']['custom']!=0,abs(rows['slide_finished']['half_height']-96)<1])
            (pathlib.Path(unreal.Paths.project_dir())/'Saved/CrouchSmoke.json').write_text(json.dumps(rows,indent=2));unreal.unregister_slate_post_tick_callback(handle)
    except Exception:
        rows['error']=traceback.format_exc();(pathlib.Path(unreal.Paths.project_dir())/'Saved/CrouchSmoke.json').write_text(json.dumps(rows,indent=2));unreal.unregister_slate_post_tick_callback(handle)
handle=unreal.register_slate_post_tick_callback(tick)




