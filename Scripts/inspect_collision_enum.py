import unreal
unreal.log('CHANNEL_NAMES '+str(dir(unreal.CollisionChannel)))
for label,fn in [('call',lambda:unreal.CollisionChannel(14)),('cast',lambda:unreal.CollisionChannel.cast(14))]:
    try:unreal.log('CHANNEL_RESULT '+label+' '+str(fn()))
    except Exception as e:unreal.log('CHANNEL_ERROR '+label+' '+str(e))
