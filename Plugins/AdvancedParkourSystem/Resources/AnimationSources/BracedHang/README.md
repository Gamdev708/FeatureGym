# Braced-hang animations

Retargeted from FeatureGym2's new Y-Bot imports to Manny, then exported through
FBX and imported with UE 5.4 using the existing Manny skeleton.

Content Browser: `/AdvancedParkourSystem/Character/MannyClimbing/Animations`.

- `AA_IdleToBracedHang_RootMotion`: standing-to-hang entry, 1.25 seconds.
- `AA_BracedHangShimmyLeft_InPlace`: lateral loop; movement comes from the character.
- `AA_BracedHangShimmyRight_InPlace`: lateral loop; movement comes from the character.
- `AA_BracedHangDrop_RootMotion`: drop transition with authored root travel.

New entry montage: `/AdvancedParkourSystem/Character/MannyClimbing/Montages/AM_IdleToBracedHang`.
Assigned to `ADSCharacter` > Character Movement > Climbing > Idle To Climb Montage.
The old entry sequence and montage are retained. Shimmy/drop playback is left
for the user to wire into the ABP and gameplay.

No motion-warp target is required by the new entry montage; it uses its authored
root movement. This transfer does not change the top-out animation or movement code.

Report: `Saved/BracedHangImportReport.json`.
The character asset before assignment is backed up under `Saved/BracedHangBackup`.
Imported bone positions were compared with the prepared UE 5.8 export at five
sample times; all four sequences passed the 1 cm tolerance.
