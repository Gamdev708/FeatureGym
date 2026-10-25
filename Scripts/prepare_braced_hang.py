import unreal
import pathlib,json,traceback,shutil,datetime

root=pathlib.Path(unreal.Paths.project_dir()).resolve()
base='/AdvancedParkourSystem/Character/MannyClimbing'
dest=pathlib.Path('E:/Unreal Projects/Gamdev708/Prototypes/Plugins/AdvancedParkourSystem/Resources/AnimationSources/BracedHang')
dest.mkdir(parents=True,exist_ok=True)
rows=[('Idle_To_Braced_Hang','AA_IdleToBracedHang_RootMotion',False),('Braced_Hang_Shimmy_Left','AA_BracedHangShimmyLeft_InPlace',True),('Braced_Hang_Shimmy_Right','AA_BracedHangShimmyRight_InPlace',True),('Braced_Hang_Drop','AA_BracedHangDrop_RootMotion',False)]
r={'success':False,'animations':[],'assignments':[]}
eal=unreal.EditorAssetLibrary
tools=unreal.AssetToolsHelpers.get_asset_tools()
options=unreal.AnimPoseEvaluationOptions(evaluation_type=unreal.AnimDataEvalType.RAW,extract_root_motion=False,incorporate_root_motion_into_pose=True)
def pose(a,t):return unreal.AnimPoseExtensions.get_anim_pose_at_time(a,t,options)
def bone(p,b):return unreal.AnimPoseExtensions.get_bone_pose(p,b,unreal.AnimPoseSpaces.WORLD)
def vec(v):return [v.x,v.y,v.z]
try:
    for _,name,_ in rows:
        if eal.does_asset_exist(base+'/Animations/'+name):raise RuntimeError('Refusing overwrite '+name)
    idle=unreal.load_asset(base+'/Animations/AA_ClimbIdle_Pose')
    base_hip=bone(pose(idle,0),'pelvis').translation
    outputs={}
    for source,name,inplace in rows:
        raw=unreal.load_asset(base+'/Animations/RT_'+source)
        model=raw.get_editor_property('data_model_interface')
        count=model.get_number_of_keys()
        length=raw.get_play_length()
        poses=[pose(raw,length*i/(count-1)) for i in range(count)]
        first=bone(poses[0],'pelvis').translation
        drift=bone(poses[-1],'pelvis').translation-first
        names=[str(n) for n in model.get_bone_track_names()]
        children=[b for b in ['pelvis','ik_foot_root','ik_hand_root'] if b in names]
        keys={b:([],[],[]) for b in ['root']+children}
        for index,p in enumerate(poses):
            offset=(first-base_hip)+drift*(index/(count-1)) if inplace else unreal.Vector()
            location=unreal.Vector() if inplace else bone(p,'pelvis').translation-first
            newroot=unreal.Transform()
            newroot.set_editor_property('translation',location)
            keys['root'][0].append(location);keys['root'][1].append(unreal.Quat());keys['root'][2].append(unreal.Vector(1,1,1))
            for b in children:
                transform=bone(p,b)
                transform.set_editor_property('translation',transform.translation-offset)
                local=transform.make_relative(newroot)
                keys[b][0].append(local.translation);keys[b][1].append(local.rotation);keys[b][2].append(local.scale3d)
        a=tools.duplicate_asset(name,base+'/Animations',raw)
        controller=unreal.AnimSequencerController()
        controller.set_model(a.get_editor_property('data_model_interface'))
        controller.open_bracket('Prepare braced-hang root track',False)
        try:
            for b,(positions,rotations,scales) in keys.items():
                if not controller.set_bone_track_keys(b,positions,rotations,scales,False):raise RuntimeError('Track update failed '+b)
        finally:controller.close_bracket(False)
        a.set_editor_property('enable_root_motion',not inplace)
        a.set_editor_property('force_root_lock',inplace)
        a.set_editor_property('root_motion_root_lock',unreal.RootMotionRootLock.ANIM_FIRST_FRAME)
        if not eal.save_loaded_asset(a,False):raise RuntimeError('Animation save failed '+name)
        samples=[]
        for alpha in [0,.25,.5,.75,1]:
            p=pose(a,length*alpha)
            samples.append({b:vec(bone(p,b).translation) for b in ['root','pelvis','hand_l','hand_r','foot_l','foot_r']})
        root_travel=(bone(pose(a,length),'root').translation-bone(pose(a,0),'root').translation).length()
        if (not inplace and root_travel<10) or (inplace and root_travel>.01):raise RuntimeError('Unexpected root travel '+name)
        rate=model.get_frame_rate()
        row={'name':name,'source':raw.get_path_name(),'duration':length,'sample_rate':round(rate.numerator/rate.denominator),'enable_root_motion':not inplace,'force_root_lock':inplace,'root_travel_cm':root_travel,'poses':samples}
        task=unreal.AssetExportTask()
        task.object=a;task.filename=str(dest/(name+'.fbx'));task.automated=True;task.prompt=False;task.replace_identical=True
        task.exporter=unreal.AnimSequenceExporterFBX()
        opts=unreal.FbxExportOption();opts.set_editor_property('export_preview_mesh',False);task.options=opts
        if not unreal.Exporter.run_asset_export_task(task):raise RuntimeError('Export failed '+name)
        outputs[name]=a
        r['animations'].append(row)
    montage_path=base+'/Montages/AM_IdleToBracedHang'
    if eal.does_asset_exist(montage_path):raise RuntimeError('Entry montage already exists')
    entry=outputs['AA_IdleToBracedHang_RootMotion']
    factory=unreal.AnimMontageFactory()
    factory.set_editor_property('target_skeleton',entry.get_editor_property('skeleton'))
    factory.set_editor_property('source_animation',entry)
    montage=tools.create_asset('AM_IdleToBracedHang',base+'/Montages',unreal.AnimMontage,factory)
    if not montage or not eal.save_loaded_asset(montage,False):raise RuntimeError('Montage creation failed')
    r['success']=True
except Exception:
    r['error']=traceback.format_exc();unreal.log_error(r['error'])
(root/'Saved/BracedHangPreparationReport.json').write_text(json.dumps(r,indent=2))
(dest/'manifest.json').write_text(json.dumps(r,indent=2))
unreal.log('BRACED_HANG_PREPARATION '+str(r['success']))
