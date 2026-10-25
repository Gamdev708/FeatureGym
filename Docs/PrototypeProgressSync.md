# Prototypes progress transferred to FeatureGym2

Transfer date: 2026-10-24. Source: `E:/Unreal Projects/Gamdev708/Prototypes` (UE 5.4). Destination: FeatureGym2 (UE 5.8.3).

The Fab / Game Animation Sample investigation is paused. This transfer uses the current Prototypes assets and code.

## Shared character implementation

The plugin's `APSCharacter`, `APSCharacterMovementComponent`, and `ADSAnimInstance` header/source pairs match Prototypes byte for byte. This includes the separate climbing input context and the current per-montage motion warping settings.

`AFGShowcaseCharacter::SetupPlayerInputComponent` now calls its parent implementation. The old per-frame WASD/mouse input and C++ climbing-loop montage playback were removed. Traversal uses `ABP_Manny_ADS`, which drives climbing through `ADSAnimInstance`. The Gym retains its checkpoints, HUD, mode switching, combat, sprint, and hop demonstration controls.

The traversal and combat Blueprint movement components receive the current ADSCharacter movement defaults and montage assignments. Combat retains its combat animation Blueprint and uses a filtered ground input context to prevent traversal bindings from conflicting with attacks.

## Assets and collision

57 progress packages were migrated, along with missing available dependencies. These include the ADS character/animation Blueprints, climbing blend space, wall run animations, current climbing sequences and montages, corrected idle-to-braced-hang root motion, course ledge/hop animations, landing/vault/slide assets, and Enhanced Input assets. Existing Gym mannequin skeletons and meshes were retained.

The `Climb` object channel and `ClimbableWall` profile match Prototypes. The Showcase climbing surfaces and relevant floor/vault practice surfaces use the Climb object channel so the shared object traces can find them.

## Current controls

- WASD / arrows / gamepad: movement; while climbing, vertical and lateral climbing input.
- Mouse: look. Space: jump / wall jump. Shift: Gym sprint.
- E: vault / climb entry. Q: detach while climbing. C: slide / crouch.
- Right mouse: traversal wall run; in combat mode, heavy attack.
- F: Gym hop. 1 / 2: traversal / combat mode.
- R: checkpoint. Home: restart route.

## Validation and recovery

The UE 5.8 editor target built successfully. Imported ADS Blueprints and both Showcase character Blueprints compiled and saved. `Scripts/validate_prototype_progress.py` reloads saved assets and checks source hashes, input assignments, movement settings, montage fields, combat filtering, and practice-surface collision. Its result is in `Saved/GymProgressValidation.json`; transfer details are in `Saved/GymProgressSyncReport.json` and `Saved/PrototypeAssetSync.json`.

Original overwritten files were backed up under `Saved/PrototypeSyncBackup/20261024_140211`. The transfer preserves the current Prototypes gameplay behavior, including ongoing animation/ledge-transition issues; it does not introduce a new fix for those issues. Interactive gameplay testing is still required.
