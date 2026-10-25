import unreal
import pathlib, json, traceback

root = pathlib.Path(unreal.Paths.project_dir())
dest = pathlib.Path('E:/Unreal Projects/Gamdev708/Prototypes/Plugins/AdvancedParkourSystem/Resources/AnimationSources/Climbing')
dest.mkdir(parents=True, exist_ok=True)
base = '/AdvancedParkourSystem/Character/MannyClimbing'
report = {'success': False, 'animations': []}
rows = [('AA_ClimbUp_InPlace','AM_ClimbUp_Loop',None), ('AA_ClimbDown_InPlace','AM_ClimbDown_Loop',None), ('AA_ClimbIdle_Pose','AM_ClimbIdle_Loop',None), ('AA_ClimbEntry_RootMotion','AM_ClimbEntry','ClimbEntryPoint'), ('AA_ClimbToTop_RootMotion','AM_ClimbToTop','ClimbTopPoint'), ('AA_BracedHangToCrouch_RootMotion','AM_BracedHangToCrouch','ClimbTopPoint')]
try:
    opts = unreal.AnimPoseEvaluationOptions(evaluation_type=unreal.AnimDataEvalType.RAW, extract_root_motion=False, incorporate_root_motion_into_pose=True)
    for name, montage, warp in rows:
        a = unreal.load_asset(base+'/Animations/'+name)
        if not a: raise RuntimeError('Missing source: '+name)
        task = unreal.AssetExportTask()
        task.object = a
        task.filename = str(dest/(name+'.fbx'))
        task.automated = True
        task.prompt = False
        task.replace_identical = True
        task.exporter = unreal.AnimSequenceExporterFBX()
        options = unreal.FbxExportOption()
        options.set_editor_property('export_preview_mesh',False)
        task.options = options
        if not unreal.Exporter.run_asset_export_task(task): raise RuntimeError('Export failed: '+name)
        poses = []
        for alpha in [0, .25, .5, .75, 1]:
            p = unreal.AnimPoseExtensions.get_anim_pose_at_time(a, a.get_play_length()*alpha, opts)
            sample = {}
            for b in ['root','pelvis','hand_l','hand_r','foot_l','foot_r']:
                v = unreal.AnimPoseExtensions.get_bone_pose(p,b,unreal.AnimPoseSpaces.WORLD).translation
                sample[b] = [v.x,v.y,v.z]
            poses.append(sample)
        rate = a.get_editor_property('data_model_interface').get_frame_rate()
        report['animations'].append({'name':name,'montage':montage,'warp':warp,'duration':a.get_play_length(),'sample_rate':round(rate.numerator/rate.denominator),'enable_root_motion':a.get_editor_property('enable_root_motion'),'force_root_lock':a.get_editor_property('force_root_lock'),'poses':poses})
    report['success'] = True
except Exception:
    report['error'] = traceback.format_exc()
    unreal.log_error(report['error'])
(dest/'manifest.json').write_text(json.dumps(report,indent=2))
(root/'Saved/ClimbingMigrationExportReport.json').write_text(json.dumps(report,indent=2))
unreal.log('CLIMBING_EXPORT '+str(report['success']))
