FEATURE GYM 2 - SYSTEMS IN MOTION
================================

Open FeatureGym2.uproject with Unreal Engine 5.8.3.
Map: /Game/Showcase/Maps/L_Showcase_SystemsInMotion
Press Play. Click the viewport to capture the mouse.

CONTROLS
WASD: move; mouse: look; Shift: sprint; Space: jump / wall jump.
1: traversal character; 2: combat character (switch while grounded).
E: vault / enter climb; Q: detach; C: slide / crouch; F: hop.
R: return to the current station checkpoint; Home: restart the route.
Combat: left mouse = light, middle mouse = medium, right mouse = heavy.
The second light-combo action supports charge: press LMB twice, hold the
second press, then release. Medium and heavy are separate configured attacks.
Shift+F1 releases the mouse in Play-in-Editor; Escape stops play.

LEVEL
A 195 x 90 metre prototype campus with a clear central avenue and six stations:
00 Arrival - controls and orientation.
01 Momentum - measured vault obstacles and slide-clearance tunnels.
02 Wall Lab - interface-bearing runnable walls, corners and wall-jump channel.
03 Vertical - climb-physics wall, stairs and drop/landing platform.
04 Combo Lab - practice targets for buffering, branches and charge timing.
05 Final Arena - cover traversal leading into a multi-target combat space.
The avenue remains open so reviewers can skip to any station. Side bays contain
the obstacles. Falling below the campus returns to the station checkpoint.

IMPLEMENTATION AND AUTHORSHIP
AdvancedParkourSystem and ComboSystemPlugin were copied from the user's
Prototypes project, excluding old Binaries and Intermediate directories.
The Prototypes source project was not modified.
Docs/ShowcaseMigration.json records additional content copied for dependencies.
Original project configuration/build files are in Saved/ShowcaseBackup.

The level, generation script, player integration, HUD, checkpoint/reset logic,
mode switching and practice-target feedback were added with Codex assistance.
Copied sample attack Blueprints were given nonzero demonstration damage.
The copied Ninja animation Blueprint uses a FeatureGym2 animation adapter,
and the unused migrated test character was reparented and its obsolete graph
removed. These integration assets differ from the untouched Prototypes source.
These additions are in Scripts/build_showcase.py and Source/FeatureGym2/Showcase.
The plugin implements movement physics, combo state selection, input buffering,
animation-notify routing and configured charge damage. Target hit timing and
proximity feedback are demonstration integration, not a plugin hitbox system.
The combat feedback applies one proximity hit per observed attack action,
using the plugin's damage value; it does not claim animation-accurate hitboxes.

CLIMBING ANIMATIONS
The five supplied Mixamo clips have been retargeted from Y-Bot to Manny.
Entry, up/down loops, a sampled stationary pose and ledge exit are integrated.
Up/down copies are in place; entry/exit copies retain prepared root motion
and have Motion Warping windows. See Docs/ClimbingAnimations_README.txt.
No separate sideways climbing or ledge-hop clips were supplied. Sideways
travel currently holds the sampled climbing pose. The optional braced-hang
exit is prepared but is not selected by the default character.
The showcase uses Manny for traversal and the plugin ninja rig for combat.
Mode switching respawns the appropriate character to keep cached animation
instances valid.

RECORDING SUGGESTION (UNDER FIVE MINUTES)
0:00-0:20  Title, controls, and a campus overview.
0:20-1:15  Vault depths and slide tunnels.
1:15-2:05  Wall run, jump away, corner and wall-jump channel.
2:05-2:35  Animated climbing and ledge exit, followed by the landing station.
2:35-3:50  Combat state display, early input, branches and hold/release.
3:50-4:40  Final arena sequence and checkpoint/reset demonstration.
Use the two level CameraActors as editor review viewpoints.

BEFORE SUBMITTING AN APPLICATION
Create and test a Windows executable, capture a video, and write a personal
account of the hardest technical problem and your individual contribution.
List third-party assets/libraries and Codex-assisted work accurately. The
existing SuperGrid, Epic template/mannequin and imported character/animation
assets are not original programming work. Verify their applicable licenses.
CyberConnect2's linked posting asks for source, executable, readme and a video
of at most 5 minutes / 100 MB. Kojima Productions asks for an executable,
capture and readme explaining execution, challenges, focus areas and references.
Check the live application pages for the requirements at submission time.

GENERATION / EVIDENCE
Scripts/build_showcase.py generates or resumes only the new showcase assets.
Saved/ShowcaseBuildReport.json records generation success or errors.
Saved/Logs/ShowcaseGeneration.log contains the commandlet log.

Validation: UE 5.8.3 Development Editor C++ build succeeded. The eight existing
ComboSystem.Charge automation tests passed. The level and showcase player
Blueprints were created and Play-in-Editor mode switching was exercised.

Combat PIE smoke test passed: traversal-to-combat switch, active attack state,
and practice-target health changing from 100 to 80 HP were observed.
Evidence: Saved/ShowcaseCombatSmoke.json and Saved/Logs/ShowcaseCompile.log.
The entire traversal course has not been manually play-tested end to end.

Crouch fix: Ctrl toggles crouch while stationary or walking; at 600 cm/s
or faster (sprinting), Ctrl slides. Standing up uses capsule clearance checks.
Slide exit restores standing collision when clear or stays crouched under cover.
