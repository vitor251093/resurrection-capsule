import blaze.Utils.BlazeFuncs as BlazeFuncs, blaze.Utils.Globals as Globals, blaze.Utils.DataClass as DataClass
import time, uuid, socket, struct
import shutil
import os

def FinalizeGameCreation(self,data_e):
	packet = BlazeFuncs.BlazeDecoder(data_e)
	reply = BlazeFuncs.BlazePacket("0004","000f",packet.packetID,"1000")
	self.transport.getHandle().sendall(reply.build().decode('Hex'))
	
def AdvanceGameState(self,data_e):
	packet = BlazeFuncs.BlazeDecoder(data_e)
	
	reply = BlazeFuncs.BlazePacket("0004","0003",packet.packetID,"1000")
	self.transport.getHandle().sendall(reply.build().decode('Hex'))

	State = packet.getVar("GSTA")
	
	self.COOPOBJ.GameState = State
	
	reply = BlazeFuncs.BlazePacket("0004","0064","0000","2000")
	reply.writeInt("GID ", self.COOPOBJ.GameID)
	reply.writeInt("GSTA", State)
	pack1, pack2 = reply.build()
	self.transport.getHandle().sendall(pack1.decode('Hex'))
	self.transport.getHandle().sendall(pack2.decode('Hex'))
	
def setGameAttributes(self,data_e):
	packet = BlazeFuncs.BlazeDecoder(data_e)
	ATTR_Name, ATTR_Cont = packet.getVar("ATTR")

	pack1, pack2 = reply.build()
	self.transport.getHandle().sendall(pack1.decode('Hex'))
	self.transport.getHandle().sendall(pack2.decode('Hex'))
	
def UpdateMeshConnection(self,data_e):
	packet = BlazeFuncs.BlazeDecoder(data_e)

	STAT = packet.getVar("STAT")
	GID = packet.getVar("GID ")
	
	reply = BlazeFuncs.BlazePacket("0004","001d",packet.packetID,"1000")
	self.transport.getHandle().sendall(reply.build().decode('Hex'))
	
	#STAT = 2
	if STAT == 2:
		reply = BlazeFuncs.BlazePacket("0004","0074","0000","2000")
		reply.writeInt("GID ", GID)
		reply.writeInt("PID ", self.GAMEOBJ.PersonaID)
		reply.writeInt("STAT", 4)
		pack1, pack2 = reply.build()
		self.transport.getHandle().sendall(pack1.decode('Hex'))
		self.transport.getHandle().sendall(pack2.decode('Hex'))
	
		reply = BlazeFuncs.BlazePacket("0004","001e","0000","2000")
		reply.writeInt("GID ", GID)
		reply.writeInt("PID ", self.GAMEOBJ.PersonaID)
		pack1, pack2 = reply.build()
		self.transport.getHandle().sendall(pack1.decode('Hex'))
		self.transport.getHandle().sendall(pack2.decode('Hex'))
	else:
		reply = BlazeFuncs.BlazePacket("0004","0028","0000","2000")
		reply.writeInt("GID ", GID)
		reply.writeInt("PID ", self.GAMEOBJ.PersonaID)
		reply.writeInt("REAS", 1)
		pack1, pack2 = reply.build()
		self.transport.getHandle().sendall(pack1.decode('Hex'))
		self.transport.getHandle().sendall(pack2.decode('Hex'))

def JoinGame(self,data_e):
	packet = BlazeFuncs.BlazeDecoder(data_e)
	GID = packet.getVar("GID ")
	
	wtf = int(packet.getVarValue("ID  ", 2))

	serverOBJ = None
	ntop = 1
	gtype = "frostbite_multiplayer"
	pres = 1
	pingsite = "ams"
	
	for Server in Globals.Servers:
		if Server.GameID == GID and Server.IsUp == True:
			serverOBJ = Server
			
	if serverOBJ == None:
		self.transport.loseConnection()
	else:
		self.GAMEOBJ.CurServer = serverOBJ
	
		slot = serverOBJ.playerJoin(self.GAMEOBJ.PersonaID)+1
		
		reply = BlazeFuncs.BlazePacket("7802","0002","0000","2000")
		reply.writeSStruct("DATA")
		reply.append("864932067f") #ADDR
		reply.writeString("BPS ", "")
		reply.writeString("CTY ", "")
		reply.append("8f68720700") #CVAR
		reply.writeInt("HWFG", 0)
		reply.writeSStruct("QDAT")
		reply.writeInt("DBPS", 0)
		reply.writeInt("NATT", 0)
		reply.writeInt("UBPS", 0)
		reply.writeEUnion() #END USER
		reply.writeInt("UATT", 0)
		reply.writeEUnion() #END USER
		reply.writeSStruct("USER")
		reply.writeInt("AID ", self.GAMEOBJ.UserID)
		reply.writeInt("ALOC", 1701729619)
		reply.writeInt("ID  ", self.GAMEOBJ.PersonaID)
		reply.writeString("NAME", self.GAMEOBJ.Name)
		reply.writeInt("ORIG", 0)
		reply.writeInt("PIDI", self.GAMEOBJ.PersonaID)
		reply.writeEUnion() #END USER
		pack1, pack2 = reply.build()
		serverOBJ.NetworkInt.getHandle().sendall(pack1.decode('Hex'))
		serverOBJ.NetworkInt.getHandle().sendall(pack2.decode('Hex'))
		
		reply = BlazeFuncs.BlazePacket("7802","0005","0000","2000")
		reply.writeInt("FLGS", 3)
		reply.writeInt("ID  ", self.GAMEOBJ.PersonaID)
		pack1, pack2 = reply.build()
		serverOBJ.NetworkInt.getHandle().sendall(pack1.decode('Hex'))
		serverOBJ.NetworkInt.getHandle().sendall(pack2.decode('Hex'))
		
		
		reply = BlazeFuncs.BlazePacket("0004","0015","0000","2000")
		reply.writeInt("GID ", GID)
			
		reply.writeSStruct("PDAT")
		reply.writeInt("CONG", self.GAMEOBJ.UserID)
		reply.writeInt("CSID", slot)
		reply.writeInt("EXID", 0)
		reply.writeInt("GID ", GID)
		reply.writeInt("LOC ", 1701729619)
		reply.writeString("NAME", self.GAMEOBJ.Name)
		reply.writeMap("PATT")
		reply.writeMapData("premium", "false")
		reply.writeBuildMap()
		reply.writeInt("PID ", self.GAMEOBJ.PersonaID)
			
		#### PNET (Union: 2)
		reply.append("c2e9740602")
			
		reply.writeSStruct("VALU")
		reply.writeSStruct("EXIP")
		reply.writeInt("IP  ", self.GAMEOBJ.EXTIP)
		reply.writeInt("PORT", self.GAMEOBJ.EXTPORT)
		reply.writeEUnion() #END EXIP
			
		reply.writeSStruct("INIP")
		reply.writeInt("IP  ", self.GAMEOBJ.INTIP)
		reply.writeInt("PORT", self.GAMEOBJ.INTPORT)
		reply.writeEUnion() #END INIP
			
		reply.writeEUnion() #END VALU
		#role soldier
		reply.writeString("ROLE", "soldier")
		reply.writeInt("SID ", slot)
		reply.writeInt("SLOT", 0)
		reply.writeInt("STAT", 0)
		reply.writeInt("TIDX", 0)
		reply.writeInt("TIME", 0)
		reply.writeInt("UID ", self.GAMEOBJ.UserID)
		pack1, pack2 = reply.build()
		serverOBJ.NetworkInt.getHandle().sendall(pack1.decode('Hex'))
		serverOBJ.NetworkInt.getHandle().sendall(pack2.decode('Hex'))

		reply = BlazeFuncs.BlazePacket("0004","0009",packet.packetID,"1000")
		reply.writeInt("GID ", GID)
		reply.writeInt("JGS ", 0)
		self.transport.getHandle().sendall(reply.build().decode('Hex'))
		
		reply = BlazeFuncs.BlazePacket("0004","0047",packet.packetID,"1000")
		reply.writeInt("GID ", GID)
		reply.writeInt("PHID", serverOBJ.PersonaID)
		reply.writeInt("PHST", 0)
		self.transport.getHandle().sendall(reply.build().decode('Hex'))
		#################################################################
		reply = BlazeFuncs.BlazePacket("7802","0002","0000","2000")
		reply.writeSStruct("DATA")
		reply.append("864932067f") #ADDR
		reply.writeString("BPS ", "")
		reply.writeString("CTY ", "")
		reply.append("8f68720700") #CVAR
		reply.writeInt("HWFG", 0)
		reply.writeSStruct("QDAT")
		reply.writeInt("DBPS", 0)
		reply.writeInt("NATT", 0)
		reply.writeInt("UBPS", 0)
		reply.writeEUnion() #END QDAT
		reply.writeInt("UATT", 0)
		reply.writeEUnion() #END ADDR
		reply.writeSStruct("USER")
		reply.writeInt("AID ", serverOBJ.PersonaID)
		reply.writeInt("ALOC", 1701729619)
		reply.writeInt("ID  ", serverOBJ.PersonaID)
		reply.writeString("NAME", serverOBJ.Name)
		reply.writeInt("ORIG", self.GAMEOBJ.PersonaID)
		reply.writeInt("PIDI", self.GAMEOBJ.PersonaID)
		reply.writeEUnion() #END USER
		pack1, pack2 = reply.build()
		self.transport.getHandle().sendall(pack1.decode('Hex'))
		self.transport.getHandle().sendall(pack2.decode('Hex'))
		#################################################################
		reply = BlazeFuncs.BlazePacket("7802","0005","0000","2000")
		reply.writeInt("FLGS", 3)
		reply.writeInt("ID  ", serverOBJ.PersonaID)
		pack1, pack2 = reply.build()
		self.transport.getHandle().sendall(pack1.decode('Hex'))
		self.transport.getHandle().sendall(pack2.decode('Hex'))
		###############################################################

		reply = BlazeFuncs.BlazePacket("0004","0016","0000","2000")

		reply.writeSStruct("GAME")
		#reply.append("864b6e04000191a4e5bc02")
		reply.writeMap("ATTR")
		for i in range(len(serverOBJ.AttrNames)):
			reply.writeMapData(serverOBJ.AttrNames[i], serverOBJ.AttrData[i])
		reply.writeBuildMap()
		#reply.append("8e1c000400022000")
		reply.writeIntArray("CAP ")
		reply.writeIntArray_Int(34)
		reply.writeIntArray_Int(0)
		reply.writeIntArray_Int(4)
		reply.writeIntArray_Int(0)
		reply.writeBuildIntArray()
		
		reply.writeInt("GID ", GID)
		reply.writeInt("GMRG", serverOBJ.GameRegis)
		reply.writeString("GNAM", serverOBJ.GameName)
		reply.writeInt("GPVH", 6667)
		reply.writeInt("GSET", serverOBJ.GameSet)
		reply.writeInt("GSID", 1337)
		reply.writeInt("GSTA", serverOBJ.GameState)
		reply.writeString("GTYP", gtype)
			
		####
		reply.writeSArray("HNET")
		##Struct in array
		reply.append("030102")

		reply.writeSStruct("EXIP")
		reply.writeInt("IP  ", serverOBJ.EXTIP)
		reply.writeInt("PORT", serverOBJ.EXTPORT)
		reply.writeEUnion() #END EXIP
			
		reply.writeSStruct("INIP")
		reply.writeInt("IP  ", serverOBJ.INTIP)
		reply.writeInt("PORT", serverOBJ.EXTPORT)
		reply.writeEUnion() #END INIP
			
		reply.writeEUnion() #END HNET
		##Struct array end
			
		reply.writeInt("HSES", 13666)
		reply.writeBool("IGNO", False)
		#reply.append("b63870008001")
		reply.writeInt("MCAP", 128)
		reply.writeInt("MNCP", 1)
		
		reply.writeBool("NRES", True)
		reply.writeInt("NTOP", ntop)
			
		reply.writeString("PGID", serverOBJ.PGID)

		reply.writeSStruct("PHST")
		reply.writeInt("CSID", slot)
		reply.writeInt("HPID", self.GAMEOBJ.PersonaID)
		reply.writeInt("HSLT", slot)
		reply.writeEUnion() #END INIP
			
		reply.writeInt("PRES", pres)
		reply.writeString("PSAS", pingsite)
		reply.writeInt("QCAP", 14)
		reply.writeInt("SEED", 11181)
		
		reply.writeSStruct("THST")
		reply.writeInt("CSID", 0)
		reply.writeInt("HPID", serverOBJ.PersonaID)
		reply.writeInt("HSLT", 0)
		reply.writeEUnion() #END INIP
		reply.writeString("UUID", serverOBJ.UUID)
		reply.writeInt("VOIP", 0)
		reply.writeString("VSTR","4900")
		reply.append("e2eba30200")
		reply.append("e339730200") 
		reply.writeEUnion() # END GAME
		
		reply.writeInt("LFPJ", 0)
			
		####
		reply.writeSArray("PROS")
		##Struct in array

		reply.append("0301")

		reply.append("8acbe20200")
		reply.writeInt("CSID", slot)
		reply.writeInt("EXID", 0)
		reply.writeInt("GID ", GID)
		reply.writeInt("JFPS", 0)
		reply.writeInt("LOC ", 1701729619)
		reply.writeString("NAME", self.GAMEOBJ.Name)

		reply.writeMap("PATT")
		reply.writeMapData("premium", "false")
		reply.writeBuildMap()
			
		reply.writeInt("PID ", self.GAMEOBJ.PersonaID)
			
		##PNET
		reply.append("c2e9740602")
			
		reply.writeSStruct("VALU")
		reply.writeSStruct("EXIP")
		reply.writeInt("IP  ", self.GAMEOBJ.EXTIP)
		reply.writeInt("PORT", self.GAMEOBJ.EXTPORT)
		reply.writeEUnion() #END EXIP
				
		reply.writeSStruct("INIP")
		reply.writeInt("IP  ", self.GAMEOBJ.INTIP)
		reply.writeInt("PORT", self.GAMEOBJ.INTPORT)
		reply.writeEUnion() #END INIP
		reply.writeEUnion() #END VALU
		
		reply.writeString("ROLE", "soldier")
		reply.writeInt("SID ", slot)
		reply.writeInt("SLOT", 0)
		reply.writeInt("STAT", 2)
		reply.writeInt("TIDX", 0)
		reply.writeInt("TIME", 0)
		reply.writeInt("UID ", self.GAMEOBJ.UserID)
			
		reply.append("ca58730600")
		reply.writeSStruct("VALU")
		reply.writeInt("DCTX", 3)
		reply.writeEUnion() #END VALU
			
		pack1, pack2 = reply.build()
		self.transport.getHandle().sendall(pack1.decode('Hex'))
		self.transport.getHandle().sendall(pack2.decode('Hex'))
		###############################################################
		
		reply = BlazeFuncs.BlazePacket("0004","0019","0000","2000")
		reply.writeInt("GID ", GID)
		reply.writeSStruct("PDAT")
		reply.writeInt("CONG", self.GAMEOBJ.UserID)
		reply.writeInt("CSID", slot)
		reply.writeInt("EXID", 0)
		reply.writeInt("GID ", GID)
		reply.writeInt("LOC ", 1701729619)
		reply.writeString("NAME", self.GAMEOBJ.Name)
			
		reply.writeMap("PATT")
		reply.writeMapData("premium", "false")
		reply.writeBuildMap()
				
		reply.writeInt("PID ", self.GAMEOBJ.PersonaID)
		####
		reply.append("c2e9740602")
			
		reply.writeSStruct("VALU")
		reply.writeSStruct("EXIP")
		reply.writeInt("IP  ", self.GAMEOBJ.EXTIP)
		reply.writeInt("PORT", self.GAMEOBJ.EXTPORT)
		reply.writeEUnion() #END EXIP
						
		reply.writeSStruct("INIP")
		reply.writeInt("IP  ", self.GAMEOBJ.INTIP)
		reply.writeInt("PORT", self.GAMEOBJ.INTPORT)
		reply.writeEUnion() #END INIP
						
		reply.writeEUnion() #END VALU
		##Struct array end

		reply.writeString("ROLE", "soldier")
		reply.writeInt("SID ", slot)
		reply.writeInt("SLOT", 0)
		reply.writeInt("STAT", 2)
		reply.writeInt("TIDX", 0)
		reply.writeInt("TIME", 0)
		reply.writeInt("UID ", self.GAMEOBJ.UserID)
		reply.writeEUnion() #END PDAT
		pack1, pack2 = reply.build()
		serverOBJ.NetworkInt.getHandle().sendall(pack1.decode('Hex'))
		serverOBJ.NetworkInt.getHandle().sendall(pack2.decode('Hex'))
		
		reply = BlazeFuncs.BlazePacket("7802","0001","0000","2000")
			
		reply.writeSStruct("DATA")
		reply.append("8649320602")
				
		reply.writeSStruct("VALU")
		reply.writeSStruct("EXIP")
		reply.writeInt("IP  ", self.GAMEOBJ.EXTIP)
		reply.writeInt("PORT", self.GAMEOBJ.EXTPORT)
		reply.writeEUnion() #END EXIP
				
		reply.writeSStruct("INIP")
		reply.writeInt("IP  ", self.GAMEOBJ.INTIP)
		reply.writeInt("PORT", self.GAMEOBJ.INTPORT)
		reply.writeEUnion() #END INIP
					
		reply.writeEUnion() #END VALU
		#reply.writeEUnion() #END DATA
		##Struct array end
		#reply.append("8b0cc00104616d73008f4e400101008f6872070092d870050000028180382b8280388d06a379a70000c33b2d040006bfbff8ff01bfbff8ff01bfbff8ff01bfbff8ff01bfbff8ff01bfbff8ff01c6487403922c330000ba1d340004d62c33000000d61d3400808080808a58d6ccf40409010201" + reply.makeInt(self.GAMEOBJ.PersonaID) +"00")
		reply.writeInt("USID", self.GAMEOBJ.PersonaID)
		pack1, pack2 = reply.build()
		serverOBJ.NetworkInt.getHandle().sendall(pack1.decode('Hex'))
		serverOBJ.NetworkInt.getHandle().sendall(pack2.decode('Hex'))
		

def ReciveComponent(self,func,data_e):
	func = func.upper()
	if func == "0001":
		print("[GMGR CLIENT] CreateGame")
	elif func == "0002":
		print("[GMGR CLIENT] DestroyGame")
	elif func == "0003":
		print("[GMGR CLIENT] AdvanceGameState")
		AdvanceGameState(self,data_e)
	elif func == "0004":
		print("[GMGR CLIENT] SetGameSettings")
	elif func == "0005":
		print("[GMGR CLIENT] SetPlayerCapacity")
	elif func == "0006":
		print("[GMGR CLIENT] SetPresenceMode")
	elif func == "0007":
		print("[GMGR CLIENT] SetGameAttributes")
		setGameAttributes(self,data_e)
	elif func == "0008":
		print("[GMGR CLIENT] SetPlayerAttributes")
	elif func == "0009":
		print("[GMGR CLIENT] JoinGame")
		JoinGame(self,data_e)
	elif func == "000B":
		print("[GMGR CLIENT] RemovePlayer")
	elif func == "000D":
		print("[GMGR CLIENT] StartMatchMaking")
	elif func == "000E":
		print("[GMGR CLIENT] CancelMatchMaking")
	elif func == "000F":
		print("[GMGR CLIENT] FinalizeGameCreation")
		FinalizeGameCreation(self,data_e)
	elif func == "0011":
		print("[GMGR CLIENT] ListGames")
	elif func == "0012":
		print("[GMGR CLIENT] SetPlayerCustomData")
	elif func == "0013":
		print("[GMGR CLIENT] ReplayGame")
	elif func == "0014":
		print("[GMGR CLIENT] returnDedicatedServerToPool")
	elif func == "0015":
		print("[GMGR CLIENT] JoinGameByGroup")
	elif func == "0016":
		print("[GMGR CLIENT] leaveGameByGroup")
		'''
		print("[ORIGIN-PATCH] Deleting user folder...")
		cwd = os.getcwd()
		shutil.rmtree(cwd+"\\Users\\"+self.GAMEOBJ.Name)
		print("[ORIGIN-PATCH] User folder deleted!")
		'''
	elif func == "0017":
		print("[GMGR CLIENT] migrateGame")
	elif func == "0018":
		print("[GMGR CLIENT] updateGameHostMigrationStatus")
	elif func == "0019":
		print("[GMGR CLIENT] resetDedicatedServer")
	elif func == "001A":
		print("[GMGR CLIENT] UpdateGameSession")
	elif func == "001B":
		print("[GMGR CLIENT] BanPlayer")
	elif func == "001D":
		print("[GMGR CLIENT] UpdateMeshConnection")
		UpdateMeshConnection(self,data_e)
	elif func == "001F":
		print("[GMGR CLIENT] RemovePlayerFromBannedList")
	elif func == "0020":
		print("[GMGR CLIENT] ClearBannedList")
	elif func == "0021":
		print("[GMGR CLIENT] getBannedList")
	elif func == "0026":
		print("[GMGR CLIENT] addQueuedPlayerToGame")
	elif func == "0027":
		print("[GMGR CLIENT] updateGameName")
	elif func == "0028":
		print("[GMGR CLIENT] ejectHost")
	elif func == "0064":
		print("[GMGR CLIENT] getGameListSnapshot")
	elif func == "0065":
		print("[GMGR CLIENT] getGameListSubscription")
	elif func == "0066":
		print("[GMGR CLIENT] DestoryGameList")
	elif func == "0067":
		print("[GMGR CLIENT] getFullGameData")
	elif func == "0068":
		print("[GMGR CLIENT] GetMatchmakingConfig")
	elif func == "0069":
		print("[GMGR CLIENT] GetGameDataFromID")
	elif func == "006A":
		print("[GMGR CLIENT] AddAdminPlayer")
	elif func == "006B":
		print("[GMGR CLIENT] RemoveAdminPlayer")
	elif func == "006C":
		print("[GMGR CLIENT] SetPlayerTeam")
	elif func == "006D":
		print("[GMGR CLIENT] ChangeGameTeamID")
	elif func == "006E":
		print("[GMGR CLIENT] MigrateAdminPlayer")
	elif func == "006F":
		print("[GMGR CLIENT] GetUserSetGameListSubscription")
	elif func == "0070":
		print("[GMGR CLIENT] SwapPlayersTeam")
	elif func == "0096":
		print("[GMGR CLIENT] RegisterDynamicDedicatedServerCreator")
	elif func == "0097":
		print("[GMGR CLIENT] UN-RegisterDynamicDedicatedServerCreator")
	else:
		print("[GMGR CLIENT] ERROR! UNKNOWN FUNC "+func)