FEATURE GYM 2 - SYSTEMS IN MOTION
Windows showcase build

RUN
1. Extract the entire supplied folder before running.
2. Double-click Start_Showcase.cmd, or Windows\FeatureGym2.exe.
3. Click inside the game window if mouse input is not captured.
4. Alt+F4 closes the game.

Unreal Editor and Visual Studio are not required. Keep the Windows folder
and all of its contents together; the executable depends on the packaged data.
This is a Windows 64-bit build. Use a DirectX 12-capable graphics driver.
If Windows reports missing runtime components, run the included installer:
Windows\Engine\Extras\Redist\en-us\vc_redist.x64.exe

CONTROLS
W/A/S/D             Move
Mouse               Look
Shift               Sprint
Space               Jump / wall jump
1                   Parkour character
2                   Combat character (switch while grounded)
E                   Vault / attach to climbable wall
W/S while climbing  Climb up/down
A/D while climbing  Move sideways
Q                   Detach from climbing
C                   Slide / crouch, depending on movement state
Left mouse          Light attack
Middle mouse        Medium attack
Right mouse         Heavy attack
R                   Return to the current station checkpoint
Home                Restart the route and reset practice targets
Alt+Enter           Toggle fullscreen
Alt+F4              Quit

COMBO DEMONSTRATION
Press left mouse twice, holding the second press, then release to exercise
the configured charged light follow-up. The HUD displays attack state,
damage and target hits. Practice targets reset automatically.

ROUTE
00 Arrival: orientation and controls
01 Momentum: vaults and slide-clearance tunnels
02 Wall Lab: wall running and wall jumping
03 Vertical: climbing and landing
04 Combo Lab: attack buffering, branches and charging
05 Final Arena: combined traversal and combat
The central avenue allows reviewers to skip stations.

PROJECT NOTES
This prototype showcases AdvancedParkourSystem and ComboSystemPlugin by
Pasindu Siriwardana. Showcase integration, level generation and the documented
debug controls were created with Codex assistance. Character, animation,
Unreal template and SuperGrid assets are third-party assets.

This is a prototype portfolio showcase. Traversal and combat animations,
collision settings, and gameplay are under active development. Plugin debug
messages and diagnostic trace drawings are disabled in this release.

