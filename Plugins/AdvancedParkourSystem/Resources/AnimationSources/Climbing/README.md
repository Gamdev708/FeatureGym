# Climb animations from FeatureGym2

Imported for UE 5.4 through FBX from the Gym's prepared Manny sequences.
Content Browser: `/AdvancedParkourSystem/Character/MannyClimbing/Animations`.
Uses the existing `/Game/Characters/Mannequins/Meshes/SK_Mannequin` skeleton.

- `AA_ClimbUp_InPlace`: upward loop; movement supplied by the climb mechanic.
- `AA_ClimbDown_InPlace`: downward loop; movement supplied by the climb mechanic.
- `AA_ClimbIdle_Pose`: stationary pose extracted from the up loop.
- `AA_ClimbEntry_RootMotion`: entry; root motion enabled.
- `AA_ClimbToTop_RootMotion`: ledge exit; root motion enabled.
- `AA_BracedHangToCrouch_RootMotion`: optional exit ending crouched; root motion enabled.

Montages are left for the user to create. The Gym uses motion-warp targets
`ClimbEntryPoint` for entry and `ClimbTopPoint` for the two exits.
Gameplay configuration and movement code have not been migrated.

`manifest.json` records source settings and sample bone positions.
Import script: `Scripts/ImportClimbingAnimations.py` (refuses existing assets).
Validation report: `Saved/ClimbingMigrationImportReport.json`.
