import blaze.Utils.BlazeFuncs as BlazeFuncs, time, blaze.Utils.Globals as Globals
import os, shutil

def UpdateNetworkInfo(self,data_e):
	packet = BlazeFuncs.BlazeDecoder(data_e)

	
	maci = packet.getVarValue("MACI", 3)
	self.GAMEOBJ.INTIP = packet.getVarValue("IP  ", 2)
	self.GAMEOBJ.INTPORT = packet.getVarValue("PORT", 2)
	self.GAMEOBJ.EXTPORT = self.GAMEOBJ.INTPORT
	self.GAMEOBJ.MacAddr = maci

	reply = BlazeFuncs.BlazePacket("7802","0014",packet.packetID,"1000")
	self.transport.getHandle().sendall(reply.build().decode('Hex'))

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
	reply.writeEUnion() #END DATA
	##Struct array end

	reply.writeInt("USID", self.GAMEOBJ.PersonaID)
	pack1, pack2 = reply.build()
	self.transport.getHandle().sendall(pack1.decode('Hex'))
	self.transport.getHandle().sendall(pack2.decode('Hex'))

	self.GAMEOBJ.NetworkInt = self.transport

def ResumeSession(self,data_e):
	packet = BlazeFuncs.BlazeDecoder(data_e)
	sessionKey = packet.getVar("SKEY")
	
	for Client in Globals.Clients:
		if Client.SessionKey == sessionKey:
			Client.IsUp == True
			self.GAMEOBJ = Client
			
			reply = BlazeFuncs.BlazePacket("7802","0023",packet.packetID,"1000")
			self.transport.getHandle().sendall(reply.build().decode('Hex'))
			
			return 0
			
	self.transport.loseConnection()

def ReciveComponent(self,func,data_e):
	func = func.upper()
	if func == "0003":
		print("[USRSEH] FetchExtendedData")
	elif func == "0005":
		print("[USRSEH] UpdateExtendedAttribute")
	elif func == "0008":
		print("[USRSEH] UpdateHardwareFlags")
	elif func == "000C":
		print("[USRSEH] LookUpUser")
	elif func == "000D":
		print("[USRSEH] LookUpUsers")
	elif func == "000E":
		print("[USRSEH] LookUpUsersByPrefix")
	elif func == "0014":
		print("[USRSEH] UpdateNetworkInfo")
		UpdateNetworkInfo(self, data_e)
	elif func == "0017":
		print("[USRSEH] LookUpUserGeoIPData")
	elif func == "0018":
		print("[USRSEH] overrideUserGeoIPData")
	elif func == "0019":
		print("[USRSEH] UpdateUserSessionClientData")
	elif func == "001A":
		print("[USRSEH] SetUserInfoAttribute")
	elif func == "001B":
		print("[USRSEH] ResetUserGeoIPData")
	elif func == "0020":
		print("[USRSEH] LookUpUserSessionID")
	elif func == "0021":
		print("[USRSEH] FetchLastLocaleUsedAndAuthError")
	elif func == "0022":
		print("[USRSEH] FetchUserFirstLastAuthTime")
	elif func == "0023":
		#print("[USRSEH] ResumeSession")
		ResumeSession(self,data_e)
	else:
		print("[USRSEH] ERROR! UNKNOWN FUNC "+func)