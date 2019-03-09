import Utils.BlazeFuncs as BlazeFuncs, time, Utils.Globals as Globals

def getFriendData(self,data_e):
	packet = BlazeFuncs.BlazeDecoder(data_e)
	
	reply = BlazeFuncs.BlazePacket("0019","0006",packet.packetID,"1000")
	'''
	reply.append("B2D870040304")
	
	reply.writeSStruct("INFO")
	reply.writeInt("FLGS", 0)
	reply.writeSStruct("LID ")
	reply.writeString("LNM ", "friendList")
	reply.writeInt("TYPE", 1)
	reply.writeEUnion() #END LID
	reply.writeInt("LMS ", 64)
	reply.writeString("PNAM", "")
	reply.writeInt("PRID ", 0)
	reply.writeEUnion() #END INFO
	reply.writeInt("OFRC ", 0)
	reply.writeInt("TOCT ", 0)
	reply.writeEUnion() #END INFO
	
	reply.writeSStruct("INFO")
	reply.writeInt("FLGS", 2)
	reply.writeSStruct("LID ")
	reply.writeString("LNM ", "recentPlayerList")
	reply.writeInt("TYPE", 2)
	reply.writeEUnion() #END LID
	reply.writeInt("LMS ", 64)
	reply.writeString("PNAM", "")
	reply.writeInt("PRID ", 0)
	reply.writeEUnion() #END INFO
	reply.writeInt("OFRC ", 0)
	reply.writeInt("TOCT ", 0)
	reply.writeEUnion() #END INFO
	
	reply.writeSStruct("INFO")
	reply.writeInt("FLGS", 2)
	reply.writeSStruct("LID ")
	reply.writeString("LNM ", "persistentMuteList")
	reply.writeInt("TYPE", 3)
	reply.writeEUnion() #END LID
	reply.writeInt("LMS ", 64)
	reply.writeString("PNAM", "")
	reply.writeInt("PRID ", 0)
	reply.writeEUnion() #END INFO
	reply.writeInt("OFRC ", 0)
	reply.writeInt("TOCT ", 0)
	reply.writeEUnion() #END INFO
	
	reply.writeSStruct("INFO")
	reply.writeInt("FLGS", 0)
	reply.writeSStruct("LID ")
	reply.writeString("LNM ", "communicationBlockList")
	reply.writeInt("TYPE", 4)
	reply.writeEUnion() #END LID
	reply.writeInt("LMS ", 64)
	reply.writeString("PNAM", "")
	reply.writeInt("PRID ", 0)
	reply.writeEUnion() #END INFO
	reply.writeInt("OFRC ", 0)
	reply.writeInt("TOCT ", 0)
	reply.writeEUnion() #END INFO
	'''
	
	self.transport.getHandle().sendall(reply.build().decode('Hex'))
	
def Zero7(self, data_e):
	reply = BlazeFuncs.BlazePacket("0019","0007",packet.packetID,"1000")
	self.transport.getHandle().sendall(reply.build().decode('Hex'))

def ReciveComponent(self,func,data_e):
	func = func.upper()
	if func == "0006":
		print("[FRNDS] getFriendData")
		getFriendData(self, data_e)
	elif func == "0007":
		print("[FRNDS] 0007")
		Zero7(self, data_e)
	else:
		print("[FRNDS] ERROR! UNKNOWN FUNC "+func)