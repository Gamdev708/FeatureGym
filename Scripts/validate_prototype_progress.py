"""Reload saved assets and verify the Prototypes-to-Gym transfer without saving."""
import unreal
import pathlib
import json
import hashlib
import traceback

root = pathlib.Path(unreal.Paths.project_dir()).resolve()
prototype = pathlib.Path('E:/Unreal Projects/Gamdev708/Prototypes')
snapshot = json.loads((root / 'Saved/PrototypesProgressSnapshot.json').read_text())
sync = json.loads((root / 'Saved/GymProgressSyncReport.json').read_text())
report = {'success': False, 'shared_source_matches': [], 'characters': {}}

def same_value(a, b):
    if isinstance(a, unreal.Object):
        return b is not None and a.get_path_name() == b.get_path_name()
    if isinstance(a, unreal.Array):
        return len(a) == len(b) and all(same_value(x, y) for x, y in zip(a, b))
    fields = {
        'TraversalMontageSettings': ['montage', 'use_motion_warping', 'warp_target_name'],
        'VaultMontageData': ['montage', 'min_vault_distance', 'max_vault_distance', 'min_obstacle_height', 'max_obstacle_height'],
        'LandingMontageData': ['montage', 'min_fall_height', 'max_fall_height'],
    }.get(type(a).__name__)
    if fields:
        return all(same_value(a.get_editor_property(p), b.get_editor_property(p)) for p in fields)
    return str(a) == str(b)

try:
    assert sync['success'], sync.get('error')
    for folder in ['Public', 'Private']:
        for name in ['APSCharacter', 'APSCharacterMovementComponent', 'ADSAnimInstance']:
            relative = pathlib.Path('Plugins/AdvancedParkourSystem/Source/AdvancedParkourSystem') / folder / (name + ('.h' if folder == 'Public' else '.cpp'))
            assert hashlib.sha256((root / relative).read_bytes()).digest() == hashlib.sha256((prototype / relative).read_bytes()).digest(), str(relative)
            report['shared_source_matches'].append(str(relative))
    report['packages_loaded'] = 0
    for package in snapshot['packages']:
        assert unreal.load_asset(package), package
        report['packages_loaded'] += 1
    base = unreal.get_default_object(unreal.load_asset('/AdvancedParkourSystem/ADSCharacter').generated_class())
    base_move = base.get_editor_property('character_movement')
    for prop, expected in snapshot['inputs'].items():
        assert base.get_editor_property(prop).get_path_name() == expected, prop
    for prop, expected in snapshot['input_mappings'].items():
        context = base.get_editor_property(prop)
        actual = [{'action': m.get_editor_property('action').get_path_name(), 'key': str(m.get_editor_property('key').get_editor_property('key_name'))} for m in context.get_editor_property('default_key_mappings').get_editor_property('mappings')]
        assert actual == expected, prop + ': key mappings differ'
    for path, settings in sync['characters'].items():
        bp = unreal.load_asset(path)
        cdo = unreal.get_default_object(bp.generated_class())
        movement = cdo.get_editor_property('character_movement')
        for prop in snapshot['inputs']:
            if path.endswith('Combat') and prop == 'default_mapping_context':
                continue
            assert same_value(cdo.get_editor_property(prop), base.get_editor_property(prop)), path + ':' + prop
        for prop in settings['movement_properties_copied']:
            assert same_value(movement.get_editor_property(prop), base_move.get_editor_property(prop)), path + ':' + prop
        assert cdo.get_editor_property('mesh').get_editor_property('anim_class').get_path_name() == settings['animation_class'], path + ':anim_class'
        report['characters'][path] = {'matching_movement_properties': len(settings['movement_properties_copied']), 'animation_class': settings['animation_class']}
    combat = unreal.load_asset('/Game/Showcase/Input/IMC_GymCombat')
    actions = {m.get_editor_property('action') for m in combat.get_editor_property('default_key_mappings').get_editor_property('mappings')}
    for prop in ['move_action', 'look_action', 'jump_action']:
        assert base.get_editor_property(prop) in actions, 'Combat missing ' + prop
    for prop in ['vault_action', 'slide_action', 'wall_run_action']:
        assert base.get_editor_property(prop) not in actions, 'Combat conflict ' + prop
    level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    assert level.load_level('/Game/Showcase/Maps/L_Showcase_SystemsInMotion')
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    found = []
    for actor in actors:
        if actor.get_actor_label() in sync['collision_actors']:
            for component in actor.get_components_by_class(unreal.StaticMeshComponent):
                assert component.get_collision_object_type() == unreal.CollisionChannel.ECC_CLIMB, actor.get_actor_label()
            found.append(actor.get_actor_label())
    assert set(found) == set(sync['collision_actors'])
    report['climb_collision_actors_verified'] = found
    report['success'] = True
except Exception:
    report['error'] = traceback.format_exc()
    unreal.log_error(report['error'])
(root / 'Saved/GymProgressValidation.json').write_text(json.dumps(report, indent=2))
unreal.log('GYM_PROGRESS_VALIDATION ' + str(report['success']))
