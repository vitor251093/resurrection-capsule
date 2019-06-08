from datetime import datetime
from threading import Timer

from blaze.Utils import Globals

x=datetime.today()
y=x.replace(day=x.day, hour=0, minute=10, second=0, microsecond=0)
delta_t=y-x
secs=delta_t.seconds+1

def cleanGlobalShit():
	print "CLEAN GLOBAL SHIT"
	clientList = []
	serverList = []
	#global Globals.Servers
	#global Globals.Clients
	
	for Server in Globals.Servers:
		if Server.IsUp:
			serverList.append(Server)
			
	Globals.Servers = serverList
	
	for Client in Globals.Clients:
		if Client.IsUp:
			clientList.append(Client)
			
	Globals.Clients = clientList

#t = Timer(secs, cleanGlobalShit)
#t.start()