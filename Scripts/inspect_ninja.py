import unreal,json,pathlib
from editor_toolset.toolsets.blueprint import BlueprintTools
bp=unreal.load_asset('/ComboSystemPlugin/Character/ABP_Ninja')
r={}
for graph in BlueprintTools.list_graphs(bp):
 try:r[graph.get_name()]=BlueprintTools.read_graph_dsl(graph)
 except Exception as e:r[graph.get_name()]=str(e)
r['variables']=str(BlueprintTools.list_variables(bp))
pathlib.Path(unreal.Paths.project_dir(),'Saved','NinjaGraphs.json').write_text(json.dumps(r,indent=2))
