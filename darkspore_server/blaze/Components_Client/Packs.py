#import Utils.BlazeFuncs as BlazeFuncs
import json
from random import randint
import time
#import MySQLdb
import blaze.Utils.BlazeFuncs as BlazeFuncs, blaze.Utils.Globals as Globals


def getPacks(self, data_e):
	packet = BlazeFuncs.BlazeDecoder(data_e)
	reply = BlazeFuncs.BlazePacket("0802","0001",packet.packetID,"1000")

	while (self.GAMEOBJ.Name == None):
		print "WAITING FOR NAME"
		continue

	#battlepackFile = open('Users/'+self.GAMEOBJ.Name+'/battlepacks.txt', 'r')
   	battlepacks = json.loads(getPacksMySql(self.GAMEOBJ.Name))
	#battlepackFile.close()

	#SCAT 0 = ALL
	#SCAT 1
	#SCAT 2
	#SCAT 3 = AWARDED

	if len(battlepacks) > 0:
		#No multi-array/list support yet!
		reply.append("c2ccf4040304")
		reply.writeArray("ITLI")
		for i in range(len(battlepacks)):
			if len(battlepacks[i][1]) == 0:
				reply.writeArray_String("")
			else:
				continue
			reply.writeBuildArray("String")
			reply.writeInt("PID ", (i+1))
			reply.writeString("PKEY", battlepacks[i][0])
			reply.writeInt("SCAT", 3)
			reply.writeInt("UID ", self.GAMEOBJ.UserID)
			reply.append("00")

	self.transport.getHandle().sendall(reply.build().decode('Hex'))

def openPack(self, data_e):
	packet = BlazeFuncs.BlazeDecoder(data_e)

	packID = (int(packet.getVar("PID "))-1)

	#battlepackFile = open('Users/'+self.GAMEOBJ.Name+'/battlepacks.txt', 'r')
	battlepacks = json.loads(getPacksMySql(self.GAMEOBJ.Name))
	#battlepackFile.close()

	itemFile = open('Data/items.txt', 'r')
   	itemDB = itemFile.readlines()
	itemFile.close()

	#itemFile = open('Users/'+self.GAMEOBJ.Name+'/items.txt', 'a+')
	#items = itemFile.readlines()
	items = readItemsMySql(self.GAMEOBJ.Name)

	itemToPutMysql = items

	#2223 (2224-1) = End of actual item list, the rest is battlepacks
	packItems = []
	for i in range(randint(2,5)):
		itemID = itemDB[randint(0,2223)].strip()
		packItems.append(itemID)
		if not (itemID in items):
			#itemFile.write(itemID+"\n")
			itemToPutMysql = itemToPutMysql+itemID+"\n"

	writeItemsMySql(self.GAMEOBJ.Name, itemToPutMysql)
	#itemFile.close()

	battlepacks[packID][1] = packItems;

	#battlepackFile = open('Users/'+self.GAMEOBJ.Name+'/battlepacks.txt', 'w')
	#battlepackFile.write(json.dumps(battlepacks))
	#battlepackFile.close()
	writeBattlepacksMySql(self.GAMEOBJ.Name, json.dumps(battlepacks))

	reply = BlazeFuncs.BlazePacket("0802","0004",packet.packetID,"1000")
	reply.writeArray("ITLI")
	for x in range(len(packItems)):
		reply.writeArray_String(packItems[x])
	reply.writeBuildArray("String")

	reply.writeInt("PID ", (packID+1))
	reply.writeString("PKEY", battlepacks[packID][0])
	reply.writeInt("SCAT", 3)
	reply.writeInt("TGEN", int(time.time()))
	reply.writeInt("TVAL", int(time.time()))
	reply.writeInt("UID ", self.GAMEOBJ.UserID)

	self.transport.getHandle().sendall(reply.build().decode('Hex'))

	reply = BlazeFuncs.BlazePacket("0802","000a","0000","2000")
	reply.writeArray("ITLI")
	for x in range(len(packItems)):
		reply.writeArray_String(packItems[x])
	reply.writeBuildArray("String")

	reply.writeInt("PID ", (packID+1))
	reply.writeString("PKEY", battlepacks[packID][0])
	reply.writeInt("SCAT", 3)
	reply.writeInt("TGEN", int(time.time()))
	reply.writeInt("TVAL", int(time.time()))
	reply.writeInt("UID ", self.GAMEOBJ.UserID)
	pack1, pack2 = reply.build()
	self.transport.getHandle().sendall(pack1.decode('Hex'))
	self.transport.getHandle().sendall(pack2.decode('Hex'))

	if self.GAMEOBJ.CurServer != None:
		self.GAMEOBJ.CurServer.NetworkInt.getHandle().sendall(pack1.decode('Hex'))
		self.GAMEOBJ.CurServer.NetworkInt.getHandle().sendall(pack2.decode('Hex'))

def ReciveComponent(self,func,data_e):
	func = (str(func).upper()).strip()
	if func == '0001':
		print("[PACKS CLIENT] Get Packs")
		getPacks(self,data_e)
	elif func == '0004':
		print("[PACKS CLIENT] Open Pack")
		openPack(self,data_e)
	else:
		print("[INV CLIENT] ERROR! UNKNOWN FUNC "+func)

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

def readItemsMySql(user):
	return "TEMP: readItemsMySql"
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
