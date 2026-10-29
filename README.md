# FeatureGym2 — Systems in Motion

Unreal Engine 5.8.3 portfolio showcase for AdvancedParkourSystem and ComboSystemPlugin.

## Getting started

Install Git LFS before cloning, then run `git lfs pull` in the checkout.
Generate project files from `FeatureGym2.uproject`, build the Development Editor
target, and open the project. The default map is
`/Game/Showcase/Maps/L_Showcase_SystemsInMotion`.

Levels (`.umap`) and Unreal assets (`.uasset`) are tracked with Git LFS.
Generated binaries, caches, IDE files, and the Saved directory are excluded.

## Controls and implementation

See [Showcase notes](Docs/Showcase_README.txt) for controls, authorship,
validation, and known limitations, and [Climbing integration](Docs/ClimbingAnimations_README.txt)
for animation retargeting details.

The original plugins were authored in the [Prototypes project](https://github.com/Gamdev708/Prototypes). Showcase integration,
level generation, and the documented climbing changes were added with Codex assistance.
