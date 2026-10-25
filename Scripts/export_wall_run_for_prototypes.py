import unreal,pathlib,json,traceback
root=pathlib.Path(unreal.Paths.project_dir());folder=root/'Saved/WallRunMigrationFBX';folder.mkdir(parents=True,exist_ok=True);report={'success':False,'exports':[]}
try:
 for name in ['AA_WallRun_Left_PalmContact','AA_WallRun_Right_PalmContact','AA_WallRun_Up_ArmsBack_Prototype']:
  a=unreal.load_asset('/Game/Showcase/AnimationExperiments/WallRun/'+name);task=unreal.AssetExportTask();task.object=a;task.filename=str(folder/(name+'.fbx'));task.automated=True;task.prompt=False;task.replace_identical=True;task.exporter=unreal.AnimSequenceExporterFBX();options=unreal.FbxExportOption();options.set_editor_property('export_preview_mesh',False);task.options=options
  if not unreal.Exporter.run_asset_export_task(task):raise RuntimeError('FBX export failed: '+name)
  report['exports'].append({'name':name,'file':task.filename,'duration':a.get_play_length()})
 report['success']=True
except Exception:report['error']=traceback.format_exc();unreal.log_error(report['error'])
(root/'Saved/WallRunMigrationExportReport.json').write_text(json.dumps(report,indent=2))
