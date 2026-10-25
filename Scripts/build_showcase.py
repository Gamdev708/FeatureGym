import unreal
import json
import pathlib
import traceback

ROOT = pathlib.Path(unreal.Paths.project_dir())
REPORT = {'warnings': [], 'blueprints': [], 'actors': 0}
ASSETS = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary
ACTORS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LEVEL = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
BASE = '/Game/Showcase'
MAP = BASE + '/Maps/L_Showcase_SystemsInMotion'

def load(path):
    obj = unreal.load_asset(path)
    if not obj:
        raise RuntimeError('Missing required asset: ' + path)
    return obj

def setting(obj, name, value):
    obj.set_editor_property(name, value)

def spawn(cls, label, loc, rot=(0,0,0), folder='Showcase'):
    a = next((a for a in ACTORS.get_all_level_actors() if a.get_actor_label()==label), None)
    if a is None:
        a = ACTORS.spawn_actor_from_class(cls, unreal.Vector(*loc), unreal.Rotator(pitch=rot[0],yaw=rot[1],roll=rot[2]))
    if not a:
        raise RuntimeError('Could not spawn ' + label)
    a.set_actor_label(label)
    a.set_folder_path(folder)
    REPORT['actors'] += 1
    return a

def make_pawn(name, combat):
    path = BASE + '/Blueprints/' + name
    if EAL.does_asset_exist(path):
        bp = load(path)
    else:
        fac = unreal.BlueprintFactory()
        fac.set_editor_property('parent_class', unreal.FGShowcaseCharacter)
        bp = ASSETS.create_asset(name, BASE+'/Blueprints', unreal.Blueprint, fac)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    cdo = unreal.get_default_object(bp.generated_class())
    setting(cdo, 'bCombatMode', combat)
    mesh = cdo.get_component_by_class(unreal.SkeletalMeshComponent)
    if combat:
        mesh.set_skeletal_mesh_asset(load('/ComboSystemPlugin/Character/Ch24_nonPBR'))
        anim_path='/Game/Showcase/Animations/ABP_ShowcaseCombat.ABP_ShowcaseCombat_C' if EAL.does_asset_exist('/Game/Showcase/Animations/ABP_ShowcaseCombat') else '/ComboSystemPlugin/Character/ABP_Ninja.ABP_Ninja_C'
        mesh.set_anim_instance_class(unreal.load_class(None, anim_path))
    else:
        mesh.set_skeletal_mesh_asset(load('/Game/Characters/Mannequins/Meshes/SKM_Manny'))
        mesh.set_anim_instance_class(unreal.load_class(None, '/AdvancedParkourSystem/ABP_Manny_ADS.ABP_Manny_ADS_C'))
    mesh.set_relative_location(unreal.Vector(0,0,-90), False, False)
    mesh.set_relative_rotation(unreal.Rotator(pitch=0,yaw=-90,roll=0), False, False)
    combo = cdo.get_component_by_class(unreal.FGShowcaseCombo)
    idle = unreal.AttackIdleState(outer=combo)
    for key, asset in [('LightAttackPress','BP_LightPressStart'),('MediumAttackPress','BP_MediumPressStart'),('HeavyAttackPress','BP_HeavyPressStart')]:
        setting(idle, key, unreal.load_class(None,'/ComboSystemPlugin/Combos/TestCombo1/'+asset+'.'+asset+'_C'))
    setting(idle,'RequiredInputs',[unreal.InputButtons.LIGHT_ATTACK,unreal.InputButtons.MEDIUM_ATTACK,unreal.InputButtons.HEAVY_ATTACK])
    setting(idle,'RequiredStates',[unreal.ButtonState.PRESSED])
    setting(combo, 'DefaultState', idle)
    movement = cdo.get_component_by_class(unreal.APSCharacterMovementComponent)
    nav=movement.get_editor_property('nav_agent_props')
    nav.set_editor_property('can_crouch',True)
    setting(movement,'nav_agent_props',nav)
    setting(movement,'crouched_half_height',45.0)
    setting(movement,'max_walk_speed_crouched',300.0)
    setting(movement,'ClimbableSurfaceTraceTypes',[unreal.ObjectTypeQuery.ECC_WORLD_STATIC])
    setting(movement,'VaultableSurfaceTraceTypes',[unreal.ObjectTypeQuery.ECC_WORLD_STATIC])
    setting(movement,'SlideSpeed',1.6)
    setting(movement,'SlideDuration',1.5)
    setting(movement,'TraceOffsetWhenSliding',55.0)
    setting(movement,'TraceSpawnSocketName','pelvis')
    setting(movement,'CapsuleDefaultHeight',96.0)
    setting(movement,'CapsuleDefaultRadius',42.0)
    setting(movement,'MeshDefaultLocationZ',-90.0)
    setting(movement,'MeshRelativeLocationZWhenSliding',-30.0)
    setting(movement,'bAutoWallRun',not combat)
    if not combat:
        for prop, asset in [('SlideMontage','AM_Sliding'),('SlideEndMontage','AM_SlideEnd'),('MidJumpMontage','AM_MidJump')]:
            setting(movement,prop,load('/Game/Blueprints/RetargetdAnimations/'+asset))
        source_class=unreal.load_class(None,'/AdvancedParkourSystem/ADSCharacter.ADSCharacter_C')
        original=unreal.get_default_object(source_class).get_component_by_class(unreal.APSCharacterMovementComponent)
        setting(movement,'VaultMontageData',original.get_editor_property('VaultMontageData'))
        setting(movement,'FallingMontageData',original.get_editor_property('FallingMontageData'))
        climb='/AdvancedParkourSystem/Character/MannyClimbing/Montages/'
        for prop in ['IdleToClimbMontage','ClimbToTopMontageSettings','ClimbDownLedgeMontageSettings','HopUpMontage','HopDownMontage']:
            setting(movement,prop,original.get_editor_property(prop))
        for prop, asset in [('ClimbUpLoop','AM_ClimbUp_Loop'),('ClimbDownLoop','AM_ClimbDown_Loop'),('ClimbIdleLoop','AM_ClimbIdle_Loop')]:
            setting(cdo,prop,load(climb+asset))
    source_pawn=unreal.get_default_object(unreal.load_class(None,'/AdvancedParkourSystem/ADSCharacter.ADSCharacter_C'))
    for prop in ['DefaultMappingContext','MoveAction','LookAction','JumpAction','VaultAction','SlideAction','WallRunAction','ClimbMappingContext','ClimbMoveAction','ClimbDetachAction']:
        setting(cdo,prop,source_pawn.get_editor_property(prop))
    if combat and EAL.does_asset_exist('/Game/Showcase/Input/IMC_GymCombat'):
        setting(cdo,'DefaultMappingContext',load('/Game/Showcase/Input/IMC_GymCombat'))
    bp.modify()
    cdo.modify()
    movement.modify()
    if not EAL.save_loaded_asset(bp, False): raise RuntimeError('Pawn save failed: '+path)
    REPORT['blueprints'].append(path)
    return bp

def run():
    # Re-running resumes only this generated map; existing user maps are untouched.
    for folder in ['Maps','Blueprints','Materials','Animations']:
        EAL.make_directory(BASE+'/'+folder)
    traversal=make_pawn('BP_ShowcaseTraversal',False)
    make_pawn('BP_ShowcaseCombat',True)
    if EAL.does_asset_exist(MAP):
        if not LEVEL.load_level(MAP): raise RuntimeError('Could not resume generated map')
    elif not LEVEL.new_level(MAP): raise RuntimeError('New level creation failed')
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    setting(world.get_world_settings(),'default_game_mode',unreal.FGShowcaseGameMode)
    # Explicit override avoids relying on the native class constructor loading a
    # Blueprint that did not exist on the first editor startup.
    gf=unreal.BlueprintFactory();setting(gf,'parent_class',unreal.FGShowcaseGameMode)
    gm=unreal.load_asset(BASE+'/Blueprints/BP_ShowcaseGameMode') or ASSETS.create_asset('BP_ShowcaseGameMode',BASE+'/Blueprints',unreal.Blueprint,gf)
    setting(unreal.get_default_object(gm.generated_class()),'default_pawn_class',traversal.generated_class())
    unreal.BlueprintEditorLibrary.compile_blueprint(gm)
    EAL.save_loaded_asset(gm)
    setting(world.get_world_settings(),'default_game_mode',gm.generated_class())
    grid=load('/Game/SuperGrid/StarterPack/Materials/Palette/Grids/M_SuperGrid_Default')
    dark=load('/Game/SuperGrid/StarterPack/Materials/Palette/Basic/M_SuperGrid_MetalDark')
    white=load('/Game/SuperGrid/StarterPack/Materials/Palette/Grids/M_SuperGrid_White')
    cyan=load('/Game/SuperGrid/StarterPack/Materials/Palette/Holographic/M_SuperGrid_Holo_Mono')
    orange=load('/Game/SuperGrid/StarterPack/Materials/Palette/Advanced/M_SuperGrid_PanelRed')
    cube=load('/Engine/BasicShapes/Cube')
    wedge=load('/Game/LevelPrototyping/Meshes/SM_Ramp') if EAL.does_asset_exist('/Game/LevelPrototyping/Meshes/SM_Ramp') else None
    def box(label, loc, size, mat=grid, folder='Showcase/Architecture',rot=(0,0,0),wall=False):
        a=spawn(unreal.APSWallRunnableActor if wall else unreal.StaticMeshActor,label,loc,rot,folder)
        if wall:
            # The plugin's wall actor is intentionally meshless. Add geometry via
            # an authored child StaticMeshActor; collision traces see the child,
            # so wall-running surfaces below use the plugin's BP_Wall instead.
            raise RuntimeError('Use wall_blueprint for runnable walls')
        c=a.static_mesh_component;c.set_static_mesh(cube);c.set_material(0,mat)
        c.set_collision_profile_name('BlockAll')
        a.set_actor_scale3d(unreal.Vector(*(v/100.0 for v in size)))
        return a
    def sign(label,text,loc,size=48,color=(190,225,240),yaw=180,folder='Showcase/Wayfinding'):
        a=spawn(unreal.TextRenderActor,label,loc,(0,yaw,0),folder)
        c=a.text_render;c.set_text(text);c.set_world_size(size)
        c.set_text_render_color(unreal.Color(*color,255));c.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
        return a
    # A 195m x 90m campus, with a clear avenue, free-practice bays and recovery floor.
    box('Campus foundation',(1250,0,-75),(19500,9000,150),dark)
    box('Clear demonstration avenue',(1250,0,-8),(19000,900,16),white)
    box('North boundary',(1250,4500,70),(19500,100,140),dark)
    box('South boundary',(1250,-4500,70),(19500,100,140),dark)
    box('West boundary',(-8500,0,70),(100,9000,140),dark)
    box('East boundary',(11000,0,70),(100,9000,140),dark)
    stations=[(-6900,'00','ARRIVAL','SYSTEMS IN MOTION'),(-3900,'01','MOMENTUM','VAULT / SLIDE'),(-600,'02','WALL LAB','RUN / JUMP / CORNER'),(2600,'03','VERTICAL','CLIMB / LANDING'),(5900,'04','COMBO LAB','BUFFER / BRANCH / CHARGE'),(9200,'05','FINAL ARENA','TRAVERSE TO COMBAT')]
    for x,num,title,sub in stations:
        folder='Showcase/'+num+' '+title
        accent=orange if int(num)>=4 else cyan
        box(num+' north practice deck',(x,2450,0),(2900,3100,24),grid,folder)
        box(num+' south practice deck',(x,-2450,0),(2900,3100,24),grid,folder)
        box(num+' gateway left',(x-1300,-520,280),(90,90,560),dark,folder)
        box(num+' gateway right',(x-1300,520,280),(90,90,560),dark,folder)
        box(num+' gateway lintel',(x-1300,0,580),(100,1130,90),dark,folder)
        box(num+' gate light',(x-1354,0,565),(8,1030,12),accent,folder)
        sign(num+' avenue title',num+' / '+title,(x-1360,0,515),60,yaw=180)
        sign(num+' station summary',sub,(x-1365,0,435),24,yaw=180)
        # Ground markers are visible from the entry and elevated review cameras.
        sign(num+' campus ID',num,(x,0,3),170,yaw=180).set_actor_rotation(unreal.Rotator(pitch=90,yaw=180,roll=0),False)
    sign('Welcome title','FEATURE GYM',(-7600,2200,420),110)
    sign('Welcome subtitle','TWO PLUGINS / ONE PLAYABLE CAMPUS',(-7590,2200,280),34)
    sign('Welcome guide','Follow the numbered avenue.\n1 Parkour / 2 Combat\nR checkpoint / Home restart',(-7580,2200,190),30)
    # Vault lanes: clear run-ups and measured obstacle depths for motion warping.
    for i,(x,h,depth) in enumerate([(-4700,85,80),(-4100,110,140),(-3300,120,220)]):
        box('Vault hurdle '+str(i+1),(x,2100,h/2),(depth,1100,h),white,'Showcase/01 MOMENTUM/Vault')
        sign('Vault dimensions '+str(i+1),str(h)+' cm HIGH / '+str(depth)+' cm DEEP',(x-140,2100,200),25)
    sign('Vault instructions','VAULT LANES\nFace a low obstacle, then E',(-5100,2100,380),48)
    # Low-clearance tunnels: a standing capsule is 192cm high, sliding is 60cm.
    for i,x in enumerate([-4600,-3500]):
        box('Slide canopy '+str(i),(x,-2200,145),(650,1100,100),dark,'Showcase/01 MOMENTUM/Slide')
        for y in [-2820,-1580]:box('Slide canopy support',(x,y,100),(650,100,200),white,'Showcase/01 MOMENTUM/Slide')
        box('Slide entry stripe',(x-370,-2200,15),(35,1100,6),cyan)
    sign('Slide instructions','SLIDE TUNNELS\nShift + W, then C\n95 cm clearance',(-5100,-2200,350),44)
    # Runnable walls use the actual plugin actor class and its interface.
    # A small Blueprint adds a static mesh to the native interface-bearing actor.
    wf=unreal.BlueprintFactory();setting(wf,'parent_class',unreal.APSWallRunnableActor)
    wallbp=unreal.load_asset(BASE+'/Blueprints/BP_ShowcaseWall') or ASSETS.create_asset('BP_ShowcaseWall',BASE+'/Blueprints',unreal.Blueprint,wf)
    sub=unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    handles=sub.k2_gather_subobject_data_for_blueprint(wallbp)
    params=unreal.AddNewSubobjectParams(parent_handle=handles[0],new_class=unreal.StaticMeshComponent,blueprint_context=wallbp)
    handle,reason=sub.add_new_subobject(params=params)
    component=unreal.SubobjectDataBlueprintFunctionLibrary.get_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle))
    component.set_static_mesh(cube);component.set_material(0,white);component.set_collision_profile_name('BlockAll')
    unreal.BlueprintEditorLibrary.compile_blueprint(wallbp);EAL.save_loaded_asset(wallbp)
    def wall(label,loc,dimensions,rot=(0,0,0)):
        a=spawn(wallbp.generated_class(),label,loc,rot,'Showcase/02 WALL LAB/Runnable surfaces')
        a.set_actor_scale3d(unreal.Vector(*(v/100 for v in dimensions)))
        return a
    wall('Left wall run',(-600,1650,250),(2500,90,500))
    wall('Right wall run',(-600,2850,250),(2500,90,500))
    wall('Corner wall A',(-1200,-2900,250),(1400,90,500))
    wall('Corner wall B',(-500,-2400,250),(90,1000,500))
    wall('Wall jump left',(250,-1800,350),(1100,90,700))
    wall('Wall jump right',(250,-1400,350),(1100,90,700))
    sign('Wallrun instructions','WALL RUN / JUMP\nJump beside the white walls\nSpace to push away',(-1820,2300,650),45)
    sign('Corner instructions','CORNER TURN / WALL JUMP\nKeep directional input into the wall',(-1800,-2400,850),40)
    # Climbing volumes and broad drop platforms with a stair return.
    box('Climb face',(2500,2400,400),(300,1500,800),white,'Showcase/03 VERTICAL/Climb')
    box('Climb top platform',(2850,2400,790),(1000,1500,60),grid,'Showcase/03 VERTICAL/Climb')
    sign('Climb instructions','CLIMBING\nE at wall / WASD climb / Q detach\nContinue upward for ledge exit',(1850,2400,1060),36)
    box('Landing high platform',(3100,-2400,500),(900,1400,100),white,'Showcase/03 VERTICAL/Landing')
    for i in range(10):
        box('Return stair '+str(i),(1600+i*120,-3200,25+i*25),(120,700,50+i*50),grid,'Showcase/03 VERTICAL/Landing')
    sign('Landing instructions','DROP / AIR CONTROL\nUse the stairs. Jump to the landing mat.',(1400,-2300,750),39)
    box('Landing mat',(3500,-1300,10),(1100,800,20),orange)
    # Combat bays have 360-degree room, plus a target line for readable captures.
    for i,(x,y,title) in enumerate([(5350,2000,'LIGHT / BUFFER'),(6550,2000,'HEAVY / CHARGE'),(5900,-2200,'BRANCH / DELAY')]):
        box('Combat station backboard',(x+450,y,220),(70,1200,440),dark,'Showcase/04 COMBO LAB')
        sign('Combat exercise '+str(i),title,(x+405,y,390),42)
        spawn(unreal.FGShowcaseTarget,'Practice target '+str(i+1),(x,y,105),folder='Showcase/04 COMBO LAB/Targets')
    sign('Combo lab instructions','PRESS 2 / COMBO SYSTEM\nLMB light / MMB medium / RMB heavy\nLMB twice, hold second press, release / Watch damage',(4550,2100,650),36)
    sign('Buffer instructions','INPUT LAB\nTry early presses and delayed follow-ups\nTarget feedback is application-side',(4600,-2200,500),34)
    # Finale: a readable open arena with cover and elevated entrances.
    box('Arena rear wall',(10600,0,260),(140,6800,520),dark,'Showcase/05 FINAL ARENA')
    for y in [-3000,3000]:
        box('Arena side cover',(9200,y,160),(1800,160,320),white,'Showcase/05 FINAL ARENA')
        box('Arena vault cover',(8450,y/2,55),(100,700,110),white,'Showcase/05 FINAL ARENA')
    for i,(x,y) in enumerate([(9350,-1000),(9550,0),(9350,1000),(10100,0)]):
        spawn(unreal.FGShowcaseTarget,'Final arena target '+str(i+1),(x,y,105),folder='Showcase/05 FINAL ARENA/Targets')
    sign('Finale instructions','FINAL ARENA\nTraverse the cover / Press 2 / Demonstrate a combo\nHome restarts the demonstration',(8000,2400,650),40)
    # Lighting is confined to this new map.
    sun=spawn(unreal.DirectionalLight,'Campus sun',(0,0,3000),(-38,-35,0),'Showcase/Lighting')
    sun.light_component.set_editor_property('intensity',80000.0)
    sun.light_component.set_editor_property('atmosphere_sun_light',True)
    sun.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    spawn(unreal.SkyAtmosphere,'Sky atmosphere',(0,0,0),folder='Showcase/Lighting')
    sky=spawn(unreal.SkyLight,'Sky fill',(0,0,1500),folder='Showcase/Lighting')
    sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    setting(sky.light_component,'real_time_capture',True)
    setting(sky.light_component,'intensity',1.0)
    pp=spawn(unreal.PostProcessVolume,'Campus exposure',(0,0,0),folder='Showcase/Lighting')
    setting(pp,'unbound',True)
    settings=pp.get_editor_property('settings')
    setting(settings,'override_auto_exposure_min_brightness',True)
    setting(settings,'override_auto_exposure_max_brightness',True)
    setting(settings,'auto_exposure_min_brightness',9.0)
    setting(settings,'auto_exposure_max_brightness',11.5)
    setting(settings,'override_auto_exposure_bias',True)
    setting(settings,'auto_exposure_bias',0.0)
    setting(pp,'settings',settings)
    spawn(unreal.PlayerStart,'Showcase start',(-7200,0,115),folder='Showcase/Start')
    camera=spawn(unreal.CameraActor,'CAM_CampusOverview',(-11500,-13000,12500),(-34,48,0),'Showcase/Review cameras')
    spawn(unreal.CameraActor,'CAM_ComboLab',(4000,-4200,2600),(-24,48,0),'Showcase/Review cameras')
    unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(unreal.Vector(-10000,-11000,11000),unreal.Rotator(pitch=-32,yaw=48,roll=0))
    if not LEVEL.save_current_level(): raise RuntimeError('Map save failed')
    EAL.save_directory(BASE, only_if_is_dirty=True, recursive=True)
    REPORT['map']=MAP
    REPORT['warnings'].append('Sideways climbing uses a sampled idle pose; no dedicated sideways or hop clips were supplied.')
    REPORT['warnings'].append('Target hit timing and HUD are FeatureGym2 integration, not features of the combo plugin.')
    REPORT['success']=True

try:
    run()
except Exception:
    REPORT['success']=False
    REPORT['error']=traceback.format_exc()
    unreal.log_error(REPORT['error'])
finally:
    (ROOT/'Saved'/'ShowcaseBuildReport.json').write_text(json.dumps(REPORT,indent=2))
    unreal.log('SHOWCASE_REPORT '+json.dumps(REPORT))
