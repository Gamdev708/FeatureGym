CLIMBING ANIMATION INTEGRATION
==============================
Original imports: /AdvancedParkourSystem/Character/MixamoImport/Animations
Manny assets: /AdvancedParkourSystem/Character/MannyClimbing

All five supplied Mixamo animations were retargeted from Y-Bot to Manny.
The original imports are unchanged. RT_ assets are the raw retargeted copies.
AA_ assets contain prepared root tracks and AM_ assets are usable montages.

Climbing Up Wall and Climbing Down Wall: in-place loops, including removal
of vertical travel that remained in the imported files. Movement comes from
the parkour movement component. Playback rate follows vertical speed.
Idle: a still pose extracted from the upward climb, not an additional clip.
Sprint To Wall Climb: root-motion entry, aligned with ClimbEntryPoint.
Climbing To Top: default root-motion ledge exit, aligned with ClimbTopPoint.
Braced Hang To Crouch: retargeted and prepared as an optional alternate exit;
it is not selected automatically because its ending pose requires a crouch.
No dedicated hop-up or hop-down clips were supplied or fabricated.

Character: /Game/Showcase/Blueprints/BP_ShowcaseTraversal
Controls: E starts climbing, W/S climb vertically, A/D move sideways,
Q detaches. Continue upward to trigger the ledge exit. Press 1 for traversal.
Sideways travel currently uses the extracted idle pose because no sideways
climbing clips were supplied.

The FeatureGym2 copy of APSCharacterMovementComponent was adjusted to allow
vertical root motion during entry, avoid double montage-completion events,
and avoid wall snapping during a root-motion ledge exit. The Prototypes
project was not modified. Backups are in Saved/ClimbingBackup.

Retargeting assets include source/target IK rigs and a reusable IK Retargeter.
Scripts: retarget_climbing.py, prepare_climbing_animation.py,
configure_climbing_showcase.py and smoke_climbing.py.
Reports: Saved/ClimbingRetargetReport.json, ClimbingPreparationReport.json,
ClimbingConfiguration.json and ClimbingSmoke.json.

Validation: Development Editor build passed. The PIE wall test exercised
entry, idle, upward/downward loops and ledge exit, finishing in walking mode
on the platform. Prepared loop root/pelvis endpoint travel is zero.
See Saved/ClimbingAssetValidation.json and Saved/ClimbingSmoke.json.
