import unreal
import pathlib,json,traceback

root=pathlib.Path(unreal.Paths.project_dir()).resolve()
dest=pathlib.Path('E:/Unreal Projects/Gamdev708/Prototypes/Plugins/AdvancedParkourSystem/Resources/AnimationSources/BracedHang')
r=json.loads((root/'Saved/BracedHangPreparationReport.json').read_text())
try:
    assert len(r['animations'])==4
    for row in r['animations']:
        a=unreal.load_asset('/AdvancedParkourSystem/Character/MannyClimbing/Animations/'+row['name'])
        assert a and a.get_editor_property('enable_root_motion')==row['enable_root_motion']
        assert (dest/(row['name']+'.fbx')).stat().st_size>1000
    assert unreal.load_asset('/AdvancedParkourSystem/Character/MannyClimbing/Montages/AM_IdleToBracedHang')
    r.pop('error',None)
    r['success']=True
    r['assignments']=[]
    r['note']='Prepared animations and entry montage in Gym; character assignment is performed in Prototypes.'
except Exception:
    r['success']=False;r['error']=traceback.format_exc();unreal.log_error(r['error'])
(root/'Saved/BracedHangPreparationReport.json').write_text(json.dumps(r,indent=2))
(dest/'manifest.json').write_text(json.dumps(r,indent=2))
unreal.log('BRACED_HANG_EXPORT_VALIDATION '+str(r['success']))
