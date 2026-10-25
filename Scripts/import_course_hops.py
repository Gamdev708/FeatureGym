import unreal
import pathlib,runpy,json,traceback

root=pathlib.Path(unreal.Paths.project_dir()).resolve()
report={'success':False,'animations':[]}
try:
    for name in ['AA_ClimbHopDown','AA_ClimbHopUp']:
        result=runpy.run_path(str(root/'Scripts/import_course_climb_down_ledge.py'),init_globals={'ANIMATION_NAME':name})
        row=result['report']
        report['animations'].append(row)
        if not row['success']:raise RuntimeError('Import failed: '+name)
    report['success']=True
except Exception:report['error']=traceback.format_exc();unreal.log_error(report['error'])
(root/'Saved/CourseHopImport.json').write_text(json.dumps(report,indent=2))
