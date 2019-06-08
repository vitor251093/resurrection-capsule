import blaze.Utils.BlazeFuncs as BlazeFuncs, math, time, os
#import MySQLdb

def getStatGroup(self, data_e):

	player_awards 			= open('Data/stats/player_awards.txt', 'r')
	player_awardsDogTags 	= open('Data/stats/player_awardsDogTags.txt', 'r')
	player_awardsXP 		= open('Data/stats/player_awardsXP.txt', 'r')
	player_core 			= open('Data/stats/player_core.txt', 'r')
	player_statcategory 	= open('Data/stats/player_statcategory.txt', 'r')
	player_unknown 			= open('Data/stats/player_unknown.txt', 'r')
	player_weapons1 		= open('Data/stats/player_weapons1.txt', 'r')
	player_weaponsxp 		= open('Data/stats/player_weaponsxp.txt', 'r')
	spplayer_singleplayer 	= open('Data/stats/spplayer_singleplayer.txt', 'r')

   	pAwards 	= []
   	pAwardsDT 	= []
	pAwardsXP	= []
	pCore		= []
	pStatCat	= []
	pUnknown	= []
	pWeapons1 	= []
   	pWeaponsXP 	= []
   	pSingle 	= []

   	Awards 		= player_awards.readlines()
   	AwardsDT 	= player_awardsDogTags.readlines()
	AwardsXP 	= player_awardsXP.readlines()
	Core		= player_core.readlines()
	StatCat		= player_statcategory.readlines()
	Unknown		= player_unknown.readlines()
	Weapons1 	= player_weapons1.readlines()
	WeaponsXP 	= player_weaponsxp.readlines()
	Single 		= spplayer_singleplayer.readlines()

	packet = BlazeFuncs.BlazeDecoder(data_e)
	reply = BlazeFuncs.BlazePacket("0007","0004",packet.packetID,"1010")
	reply.writeString("CNAM", "player_awards")
	reply.writeString("DESC", "player_WebPlayerStats")
	reply.writeString("NAME", "player_WebPlayerStats")
	reply.writeArray("STAT")

	for i in range(len(Awards)):
		reply.writeArray_TString("CATG", "player_awards")
		reply.writeArray_TString("DFLT", "0.00")
		reply.writeArray_TString("FRMT", "%.2f")
		reply.writeArray_TString("NAME", Awards[i].strip())
		reply.writeArray_TInt("TYPE", 1)
		reply.writeArray_ValEnd()

	for i in range(len(AwardsDT)):
		reply.writeArray_TString("CATG", "player_awardsDogTags")
		reply.writeArray_TString("DFLT", "0.00")
		reply.writeArray_TString("FRMT", "%.2f")
		reply.writeArray_TString("NAME", AwardsDT[i].strip())
		reply.writeArray_TInt("TYPE", 1)
		reply.writeArray_ValEnd()

	for i in range(len(AwardsXP)):
		reply.writeArray_TString("CATG", "player_awardsXP")
		reply.writeArray_TString("DFLT", "0.00")
		reply.writeArray_TString("FRMT", "%.2f")
		reply.writeArray_TString("NAME", AwardsXP[i].strip())
		reply.writeArray_TInt("TYPE", 1)
		reply.writeArray_ValEnd()

	for i in range(len(Core)):
		reply.writeArray_TString("CATG", "player_core")
		reply.writeArray_TString("DFLT", "0.00")
		reply.writeArray_TString("FRMT", "%.2f")
		reply.writeArray_TString("NAME", Core[i].strip())
		reply.writeArray_TInt("TYPE", 1)
		reply.writeArray_ValEnd()

	for i in range(len(StatCat)):
		reply.writeArray_TString("CATG", "player_statcategory")
		reply.writeArray_TString("DFLT", "0.00")
		reply.writeArray_TString("FRMT", "%.2f")
		reply.writeArray_TString("NAME", StatCat[i].strip())
		reply.writeArray_TInt("TYPE", 1)
		reply.writeArray_ValEnd()

	for i in range(len(Unknown)):
		reply.writeArray_TString("CATG", "player_unknown")
		reply.writeArray_TString("DFLT", "0.00")
		reply.writeArray_TString("FRMT", "%.2f")
		reply.writeArray_TString("NAME", Unknown[i].strip())
		reply.writeArray_TInt("TYPE", 1)
		reply.writeArray_ValEnd()

	for i in range(len(Weapons1)):
		reply.writeArray_TString("CATG", "player_weapons1")
		reply.writeArray_TString("DFLT", "0.00")
		reply.writeArray_TString("FRMT", "%.2f")
		reply.writeArray_TString("NAME", Weapons1[i].strip())
		reply.writeArray_TInt("TYPE", 1)
		reply.writeArray_ValEnd()

	for i in range(len(WeaponsXP)):
		reply.writeArray_TString("CATG", "player_weaponsxp")
		reply.writeArray_TString("DFLT", "0.00")
		reply.writeArray_TString("FRMT", "%.2f")
		reply.writeArray_TString("NAME", WeaponsXP[i].strip())
		reply.writeArray_TInt("TYPE", 1)
		reply.writeArray_ValEnd()

	for i in range(len(Single)):
		reply.writeArray_TString("CATG", "spplayer_singleplayer")
		reply.writeArray_TString("DFLT", "0.00")
		reply.writeArray_TString("FRMT", "%.2f")
		reply.writeArray_TString("NAME", Single[i].strip())
		reply.writeArray_TInt("TYPE", 1)
		reply.writeArray_ValEnd()


	reply.writeBuildArray("Struct")

	self.transport.write(reply.build().decode('Hex'))

def getStatsByGroupAsync(self, data_e):
	packet = BlazeFuncs.BlazeDecoder(data_e)
	user = packet.getVar('EID ')
	vid = packet.getVar('VID ')

	userID = user[0]

	stats = open('Users/'+self.GAMEOBJ.Name+'/userstats.txt', 'r')
	pStats = []
   	lines = stats.readlines()

   	lines = [word.strip() for word in lines]

   	for line in lines:
   		pStats.append(line.split("="))

	reply = BlazeFuncs.BlazePacket("0007","0010",packet.packetID,"1000")
	self.transport.getHandle().sendall(reply.build().decode('Hex'))

	reply = BlazeFuncs.BlazePacket("0007","0032","0000","2000")
	reply.writeString("GRNM", "player_WebPlayerStats")
	reply.writeString("KEY ", "No_Scope_Defined")
	reply.writeBool("LAST", True)

	reply.writeSStruct("STS ")
	reply.append("cf4874040301")

	reply.writeInt("EID ", userID)
	reply.writeArray("STAT")

	for i in range(len(lines)):
		val = str(pStats[i][1]).strip()
		reply.writeArray_String(val)

	reply.writeBuildArray("String")
	reply.writeEUnion()
	reply.writeEUnion()
	reply.writeInt("VID ", int(vid))

	pack1, pack2 = reply.build()
	self.transport.write(pack1.decode('Hex'))
	self.transport.write(pack2.decode('Hex'))

def ReciveComponent(self,func,data_e):
	func = func.upper()
	if func == '0004':
		print("[STATS CLIENT] getStatGroup")
		getStatGroup(self,data_e)
	if func == '0010':
		print("[STATS CLIENT] getStatsByGroupAsync")
		getStatsByGroupAsync(self, data_e)
	else:
		print("[STATS] ERROR! UNKNOWN FUNC "+func)
