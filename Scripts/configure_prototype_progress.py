import unreal
import pathlib,json,traceback,re,shutil

root=pathlib.Path(unreal.Paths.project_dir()).resolve()
backup=pathlib.Path(json.loads((root/'Saved/PrototypeSyncBackup/Latest.json').read_text(encoding='utf-8-sig'))['backup'])
snapshot=json.loads((root/'Saved/PrototypesProgressSnapshot.json').read_text())
r={'success':False,'characters':{},'compiled_blueprints':[],'collision_actors':[]}
eal=unreal.EditorAssetLibrary
def save_backup(relative):
    source=root/relative;dest=backup/relative
    if source.exists() and not dest.exists():dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,dest)
def asset_value(obj,name):
    v=obj.get_editor_property(name)
    return v.get_path_name() if isinstance(v,unreal.Object) else str(v)
try:
    for package in snapshot['packages']:
        asset=unreal.load_asset(package)
        if not asset:raise RuntimeError('Migrated package could not load: '+package)
        if isinstance(asset,unreal.Blueprint):
            unreal.BlueprintEditorLibrary.compile_blueprint(asset)
            if not eal.save_loaded_asset(asset,False):raise RuntimeError('Blueprint save failed: '+package)
            r['compiled_blueprints'].append(package)
    source_bp=unreal.load_asset('/AdvancedParkourSystem/ADSCharacter')
    source=unreal.get_default_object(source_bp.generated_class())
    source_move=source.get_editor_property('character_movement')
    header_text=(root/'Plugins/AdvancedParkourSystem/Source/AdvancedParkourSystem/Public/APSCharacterMovementComponent.h').read_text(encoding='utf-8-sig')
    header=header_text[header_text.index('class ADVANCEDPARKOURSYSTEM_API UAPSCharacterMovementComponent'):].splitlines()
    movement_properties=[]
    for i,line in enumerate(header[:-1]):
        if line.lstrip().startswith('UPROPERTY(EditDefaultsOnly'):
            declaration=header[i+1].strip().split('=')[0].rstrip(';').strip()
            match=re.search(r'(\w+)$',declaration)
            if match:movement_properties.append(match.group(1))
    default_context=source.get_editor_property('default_mapping_context')
    combat_path='/Game/Showcase/Input/IMC_GymCombat'
    combat_context=unreal.load_asset(combat_path) if eal.does_asset_exist(combat_path) else unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset('IMC_GymCombat','/Game/Showcase/Input',default_context)
    blocked={source.get_editor_property(p) for p in ['vault_action','wall_run_action','slide_action']}
    # UE 5.8 migrates the former Mappings array into DefaultKeyMappings.
    mapping_data=default_context.get_editor_property('default_key_mappings').copy()
    mapping_data.set_editor_property('mappings',[m for m in mapping_data.get_editor_property('mappings') if m.get_editor_property('action') not in blocked])
    combat_context.set_editor_property('default_key_mappings',mapping_data)
    if not eal.save_loaded_asset(combat_context,False):raise RuntimeError('Combat context save failed')
    for path in ['/Game/Showcase/Blueprints/BP_ShowcaseTraversal','/Game/Showcase/Blueprints/BP_ShowcaseCombat']:
        save_backup('Content/'+path[len('/Game/'):]+'.uasset')
        bp=unreal.load_asset(path)
        bp.modify();cdo=unreal.get_default_object(bp.generated_class());cdo.modify()
        for prop in snapshot['inputs']:
            cdo.set_editor_property(prop,source.get_editor_property(prop))
        movement=cdo.get_editor_property('character_movement');movement.modify()
        for prop in movement_properties:
            movement.set_editor_property(prop,source_move.get_editor_property(prop))
        if path.endswith('BP_ShowcaseTraversal'):
            cdo.get_editor_property('mesh').set_editor_property('anim_class',unreal.load_asset('/AdvancedParkourSystem/ABP_Manny_ADS').generated_class())
        else:
            cdo.set_editor_property('default_mapping_context',combat_context)
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        if not eal.save_loaded_asset(bp,False):raise RuntimeError('Character save failed: '+path)
        cdo=unreal.get_default_object(bp.generated_class())
        r['characters'][path]={'inputs':{p:asset_value(cdo,p) for p in snapshot['inputs']},'animation_class':asset_value(cdo.get_editor_property('mesh'),'anim_class'),'movement_properties_copied':movement_properties}
    map_path='/Game/Showcase/Maps/L_Showcase_SystemsInMotion'
    save_backup('Content/Showcase/Maps/L_Showcase_SystemsInMotion.umap')
    level=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not level.load_level(map_path):raise RuntimeError('Cannot load Showcase map')
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    for actor in actors:
        label=actor.get_actor_label()
        should_tag=(label in ['Climb face','Climb top platform','Campus foundation','Clear demonstration avenue','Landing mat'] or label.startswith('Vault hurdle ') or label.startswith('Arena vault cover') or label.endswith('north practice deck'))
        if should_tag:
            for component in actor.get_components_by_class(unreal.StaticMeshComponent):
                component.modify();component.set_collision_object_type(unreal.CollisionChannel.ECC_CLIMB)
            r['collision_actors'].append(label)
        for text in actor.get_components_by_class(unreal.TextRenderComponent):
            previous=str(text.get_editor_property('text'))
            if 'Ctrl' in previous:text.modify();text.set_text(previous.replace('Ctrl','C'))
    if not level.save_current_level():raise RuntimeError('Cannot save Showcase map')
    r['backup']=str(backup)
    r['success']=True
except Exception:
    r['error']=traceback.format_exc();unreal.log_error(r['error'])
(root/'Saved/GymProgressSyncReport.json').write_text(json.dumps(r,indent=2))
unreal.log('GYM_PROGRESS_SYNC '+str(r['success']))
