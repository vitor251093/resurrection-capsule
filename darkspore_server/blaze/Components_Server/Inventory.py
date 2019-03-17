import blaze.Utils.BlazeFuncs as BlazeFuncs, blaze.Utils.Globals as Globals
import json
#import MySQLdb

def getItems(self, data_e):
	packet = BlazeFuncs.BlazeDecoder(data_e)
	reply = BlazeFuncs.BlazePacket("0803","0001",packet.packetID,"1000")

	userID = int(packet.getVar('UID '))

	name = None
	for Client in Globals.Clients:
		if Client.PersonaID == userID:
			name = Client.Name


	consumables = ""
	userItems = ""

	#bot join
	if name == None:
		print("[DEBUG] Loading bot!")
		consumeableFile = open('Data/stats/bots_consumables.txt', 'r')
		consumeables = json.loads(consumeableFile.readline())
		consumeableFile.close()
		itemsFile = open('Data/stats/bots_stats.txt', 'r')
		userItems = itemsFile.readlines()
		itemsFile.close()
	else:
	   	consumeables = json.loads(getItemsMySql(name))
	   	userItems = getItemsMySql_(name)




	#consumeableFile = open('Users/'+name+'/consumables.txt', 'r')
	#consumeableFile.close()



	itemsFile = open('Data/items.txt', 'r+')
   	items = itemsFile.readlines()
	itemsFile.close()

	#itemsFile = open('Users/'+name+'/items.txt', 'r')
	#itemsFile.close()



	reply.writeSStruct("INVT")

	reply.writeArray("BLST")
	for i in range(len(items)):
		if items[i] in userItems:
			reply.writeArray_Bool(1)
		else:
			reply.writeArray_Bool(0)
	reply.writeBuildArray("Bool")
	#reply.writeEUnion()

	reply.writeArray("CLST")
	for i in range(len(consumeables)):
		reply.writeArray_TInt("ACTT", userID)
		reply.writeArray_TString("CKEY", consumeables[i][0])
		reply.writeArray_TInt("DURA", consumeables[i][2])
		reply.writeArray_TInt("QANT", consumeables[i][1])
		reply.writeArray_ValEnd()

	reply.writeBuildArray("Struct")


	self.transport.getHandle().sendall(reply.build().decode('Hex'))


def getTemplate(self, data_e):
	packet = BlazeFuncs.BlazeDecoder(data_e)
	reply = BlazeFuncs.BlazePacket("0803","0006",packet.packetID,"1000")

	reply.writeArray("ILST")

	itemFile = open('Data/items.txt', 'r')
   	items = itemFile.readlines()
	itemFile.close()

	for i in range(len(items)):
		val = str(items[i])
		reply.writeArray_String(val.strip())

	reply.writeBuildArray("String")

	self.transport.getHandle().sendall(reply.build().decode('Hex'))

def ReciveComponent(self,func,data_e):
	func = func.upper()
	if func == '0001':
		print("[INV] getItems")
		getItems(self,data_e)
	elif func == '0006':
		print("[INV] getTemplate")
		getTemplate(self,data_e)
	else:
		print("[INV] ERROR! UNKNOWN FUNC "+func)

def getItemsMySql(user):
	print "TEMP: getItemsMySql"
	#Query example: SELECT usersettings FROM `users` WHERE username = 'StoCazzo'

	# db = MySQLdb.connect(Globals.dbHost, Globals.dbUser, Globals.dbPass, Globals.dbDatabase)
	#
	# cursor = db.cursor()
	#
	# sql = "SELECT consumables FROM `users` WHERE username = '"+str(user)+"'"
	#
	# try:
	#    cursor.execute(sql)
	#    results = cursor.fetchall()
	#    for row in results:
	# 	  consumables = row[0]
	# 	  return consumables
	# except:
	#    print "[MySQL] Can't load consumables!"
	#
	# db.close()

def getItemsMySql_(user):
	return "TEMP: getItemsMySql_"
	#Query example: SELECT usersettings FROM `users` WHERE username = 'StoCazzo'

	# db = MySQLdb.connect(Globals.dbHost, Globals.dbUser, Globals.dbPass, Globals.dbDatabase)
	#
	# cursor = db.cursor()
	#
	# sql = "SELECT items FROM `users` WHERE username = '"+str(user)+"'"
	#
	# try:
	#    cursor.execute(sql)
	#    results = cursor.fetchall()
	#    for row in results:
	# 	  items = row[0]
	# 	  return items
	# except:
	#    print "[MySQL] Can't load items!"
	#
	# db.close()
