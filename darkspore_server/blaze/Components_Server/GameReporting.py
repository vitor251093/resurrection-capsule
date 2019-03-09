import Utils.BlazeFuncs as BlazeFuncs
import Utils.Globals as Globals
#import MySQLdb

def ReciveComponent(self,func,data_e):
	func = func.upper()

	#print data_e
	#print "================"
	if func == '0064':
		print("[GMRPT] submitTrustedMidGameReport")
		packet = BlazeFuncs.BlazeDecoder(data_e)

		pids, content = packet.getStatsVar("PLYR")

		for i in range(len(pids)):
			pid = int(pids[i])

			name = None
			for Client in Globals.Clients:
				if Client.PersonaID == pid:
					name = Client.Name

			#stats = open('Users/'+name+'/userstats.txt', 'r')
			#pStats = []
			pStats = getUserstatsMySql(name)
		   	#lines = stats.readlines()
		   	#stats.close()

			'''
		   	lines = [word.strip() for word in lines]

		   	for line in lines:
		   		pStats.append(line.split("="))


			for line_2 in pStats:
				print("HELLOME"+str(line_2))
			'''

			for x in range(len(content[i])):
				for y in range(len(pStats)):
					if (content[i][x][0][:-2] == pStats[y][0]) or (content[i][x][0] == pStats[y][0]):
						stat = float(content[i][x][1])
						stat = stat+float(pStats[y][1])
						pStats[y][1] = str(stat)

			statsWrite = []
			#f = open("Users/"+name+"/userstats.txt", 'w')
			for y in range(len(pStats)):
				#f.write(pStats[y][0]+"="+pStats[y][1]+"\n")

				statsWrite.append(pStats[y][0]+"="+pStats[y][1])

				#print("SAVE"+str(pStats[y][0]+"="+pStats[y][1]+"\n"))
			writeUserstatsMySql(name, statsWrite)
			statsWrite = []
			#f.close()

		reply = BlazeFuncs.BlazePacket("001C","0064",packet.packetID,"1000")
		self.transport.getHandle().sendall(reply.build().decode('Hex'))

		reply = BlazeFuncs.BlazePacket("001C","0072","0000","2000")

		## Not working yet, weird ass packet -- This tells the server to request a drain on consumeables eg EXP Boosts
		#reply.writeArray("PLYR")
		#for i in range(len(pids)):
		#	reply.writeArray_Int(int(pids[i]))
		#reply.writeBuildArray("Int")

		reply.writeInt("FNL ", 0)
		reply.writeInt("GHID", 1000000)
		reply.writeInt("GRID", 1000000)
		pack1, pack2 = reply.build()
		self.transport.getHandle().sendall(pack1.decode('Hex'))
		self.transport.getHandle().sendall(pack2.decode('Hex'))
	elif func == '0065':
		print("[GMRPT] submitTrustedEndGameReport")
		packet = BlazeFuncs.BlazeDecoder(data_e)

		reply = BlazeFuncs.BlazePacket("001C","0065",packet.packetID,"1000")
		self.transport.getHandle().sendall(reply.build().decode('Hex'))

		reply = BlazeFuncs.BlazePacket("001C","0072","0000","2000")
		reply.writeInt("FNL ", 1)
		reply.writeInt("GHID", 1000000)
		reply.writeInt("GRID", 1000000)
		pack1, pack2 = reply.build()
		self.transport.getHandle().sendall(pack1.decode('Hex'))
		self.transport.getHandle().sendall(pack2.decode('Hex'))

	else:
		print("[GMRPT] ERROR! UNKNOWN FUNC "+func)

def getUserstatsMySql(user):
	print "TEMP: getUserstatsMySql"
	#Query example: SELECT usersettings FROM `users` WHERE username = 'StoCazzo'

	# db = MySQLdb.connect(Globals.dbHost, Globals.dbUser, Globals.dbPass, Globals.dbDatabase)
	#
	# cursor = db.cursor()
	#
	# sql = "SELECT userstats FROM `users` WHERE username = '"+str(user)+"'"

	pStats = []

	# try:
	#    cursor.execute(sql)
	#    results = cursor.fetchall()
	#    for row in results:
	# 	   userstats = row[0]
	# 	   temp = userstats.splitlines()
	#    for line in temp:
	# 		pStats.append(line.split("="))
	#
	# except:
	#    print "[MySQL] Can't load userstats!"


	return pStats

	# db.close()

def writeUserstatsMySql(user, stats):
	print "TEMP: writeUserstatsMySql"
	# toAdd = ""
	# for lineStats in stats:
	# 	toAdd = toAdd + lineStats + "\n"
	#
	# #Query Example: UPDATE `users` SET `usersettings` = 'helloguys' WHERE `users`.`username` = 'StoCazzo'
	#
	# db = MySQLdb.connect(Globals.dbHost, Globals.dbUser, Globals.dbPass, Globals.dbDatabase)
	#
	# cursor = db.cursor()
	#
	# sql = "UPDATE `users` SET `userstats` = '"+str(toAdd)+"' WHERE `users`.`username` = '"+str(user)+"'"
	#
	#
	# cursor.execute(sql)
	# db.commit()
	# '''
	# try:
	#
	# except:
	#    print "[MySQL] Can't write userstats!"
	#    db.rollback()
	# '''
	# db.close()
