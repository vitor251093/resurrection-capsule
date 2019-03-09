import Utils.BlazeFuncs as BlazeFuncs
import Utils.Globals as Globals
import json
#import MySQLdb

def grantPacks(self, data_e):
	packet = BlazeFuncs.BlazeDecoder(data_e)
	packList = packet.getVar("PKLS")
	userID = packet.getVar("UID ")

	reply = BlazeFuncs.BlazePacket("0802","0002",packet.packetID,"1000")
	reply.writeArray("PIDL")
	for i in range(len(packList)):
		reply.writeArray_TInt("ERR ", 0)
		reply.writeArray_TString("PKEY", packList[i])
		reply.writeArray_ValEnd()
	reply.writeBuildArray("Struct")
	self.transport.getHandle().sendall(reply.build().decode('Hex'))

	battlepackFile = open('Data/battlepacks.txt', 'w')
	battlepackFile.write(reply.build())
	battlepackFile.close()

	name = None
	for Client in Globals.Clients:
		if Client.UserID == userID:
			name = Client.Name

	if name == None:
		return

	#itemFile = open('Users/'+name+'/items.txt', 'r+')
	#items = itemFile.readlines()
	items = readItemsMySql(name)
	toWriteItems = items+"\n"

	for x in range(len(packList)):
		if not (packList[x] in items):
			#itemFile.write(packList[x]+"\n")
			toWriteItems = toWriteItems+packList[x]+"\n"

			battlepackItem = [packList[x], []]

			#battlepackFile = open('Users/'+name+'/battlepacks.txt', 'r')
			packStr = getPacksMySql(name)
			#battlepackFile.close()


			writeDta = []
			if len(packStr) <= 2:
				writeDta = [battlepackItem]
			else:
				battlepacks = json.loads(packStr)
				battlepacks.append(battlepackItem)
				writeDta = battlepacks

			#battlepackFile = open('Users/'+name+'/battlepacks.txt', 'w')
			#battlepackFile.write(json.dumps(writeDta))
			#battlepackFile.close()
			writeBattlepacksMySql(name, json.dumps(writeDta))


	#itemFile.close()
	writeItemsMySql(name, toWriteItems)
	toWriteItems = ""

def ReciveComponent(self,func,data_e):
	func = func.upper()
	if func == '0002':
		#print("[PACKS] grant Packs")
		grantPacks(self,data_e)
	else:
		print("[INV] ERROR! UNKNOWN FUNC "+func)


def getPacksMySql(user):
	print "TEMP: getPacksMySql"
	#Query example: SELECT usersettings FROM `users` WHERE username = 'StoCazzo'

	# db = MySQLdb.connect(Globals.dbHost, Globals.dbUser, Globals.dbPass, Globals.dbDatabase)
	#
	# cursor = db.cursor()
	#
	# sql = "SELECT battlepacks FROM `users` WHERE username = '"+str(user)+"'"
	#
	# try:
	#    cursor.execute(sql)
	#    results = cursor.fetchall()
	#    for row in results:
	# 	  packs = row[0]
	# 	  return packs
	# except:
	#    print "[MySQL] Can't load battlepacks!"
	#
	# db.close()

def writeBattlepacksMySql(user, data):
	print "TEMP: writeBattlepacksMySql"
	#Query Example: UPDATE `users` SET `usersettings` = 'helloguys' WHERE `users`.`username` = 'StoCazzo'

	# db = MySQLdb.connect(Globals.dbHost, Globals.dbUser, Globals.dbPass, Globals.dbDatabase)
	#
	# cursor = db.cursor()
	#
	# sql = "UPDATE `users` SET `battlepacks` = '"+str(data)+"' WHERE `users`.`username` = '"+str(user)+"'"
	#
	# try:
	#    cursor.execute(sql)
	#    db.commit()
	# except:
	#    print "[MySQL] Can't write battlepacks!"
	#    db.rollback()
	#
	# db.close()

def readItemsMySql(user):
	print "TEMP: readItemsMySql"
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


def writeItemsMySql(user, data):
	print "TEMP: writeItemsMySql"
	#Query Example: UPDATE `users` SET `usersettings` = 'helloguys' WHERE `users`.`username` = 'StoCazzo'

	# db = MySQLdb.connect(Globals.dbHost, Globals.dbUser, Globals.dbPass, Globals.dbDatabase)
	#
	# cursor = db.cursor()
	#
	# sql = "UPDATE `users` SET `items` = '"+str(data)+"' WHERE `users`.`username` = '"+str(user)+"'"
	#
	# try:
	#    cursor.execute(sql)
	#    db.commit()
	# except:
	#    print "[MySQL] Can't write items!"
	#    db.rollback()
	#
	# db.close()
