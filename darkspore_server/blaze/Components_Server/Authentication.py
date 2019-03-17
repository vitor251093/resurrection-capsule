import blaze.Utils.DataClass as DataClass, blaze.Utils.BlazeFuncs as BlazeFuncs, blaze.Utils.Globals as Globals
import time, socket, struct, random, string, json
import blaze.Utils.BlazeFuncs as BlazeFuncs, time, os, shutil
#import MySQLdb
import hashlib

def Login(self,data_e):
	packet = BlazeFuncs.BlazeDecoder(data_e)

	#mail and password from server, default mail = bf4.server.pc@ea.com

	mail = packet.getVar("MAIL")
	password = packet.getVar("PASS")
	password_md5=hashlib.md5(password).hexdigest()

	logged = True

	print('[MySQL] Server Trying to login with Mail: ' + mail + ' and Password: ' + password)

	# db = MySQLdb.connect(Globals.dbHost, Globals.dbUser, Globals.dbPass, Globals.dbDatabase)
	#
	# cursor = db.cursor()
	# cursor.execute ("SELECT * FROM `servers` WHERE email = '" + mail + "' && password = '" + password_md5 + "'")
	#
	# if not cursor.rowcount:
	# 	print('[MySQL] Login Error! Username and password not found!')
	# else:
	# 	logged = True
	# 	print('[MySQL] Logging in, username and password found...')



	# cursor.close()
	# db.close()


	#cwd = os.getcwd()
	#path = cwd+"\\Servers\\"+mail

	if logged == True:
		Login2(self, data_e, mail, packet)
		print('[MySQL] Logged in!')
	else: #shutdown socket
		#print("[MySQL] Error! Server Account not found...")
		self.transport.loseConnection()
		self.GAMEOBJ.IsUp = False

def Login2(self, data_e, mail, packet):
	packedIP = socket.inet_aton(self.transport.getPeer().host)

	self.GAMEOBJ.EXTIP = struct.unpack("!L", packedIP)[0]

	self.GAMEOBJ.SessionKey = ''.join(random.choice(string.ascii_uppercase + string.digits) for _ in range(42))

	reply = BlazeFuncs.BlazePacket("0001","003C",packet.packetID,"1000")
	reply.writeBool("AGUP",False)
	reply.writeBool("ANON",False)
	reply.writeBool("NTOS",False)
	reply.writeString("PCTK","PlayerTicket_1337")

	reply.writeSStruct("SESS")
	reply.writeInt("BUID", 321422442)
	reply.writeBool("FRST", False)
	reply.writeString("KEY ","SessionKey_1337")
	reply.writeInt("LLOG", 1403663841)
	reply.writeString("MAIL",mail) #write var mail to server

	reply.writeSStruct("PDTL")
	reply.writeString("DSNM",self.GAMEOBJ.Name)
	reply.writeInt("LAST", 1403663841)
	reply.writeInt("PID ", self.GAMEOBJ.PersonaID)
	reply.writeInt("PLAT", 4)
	#1 XBL2
	#2 PS3
	#3 WII
	#4 PC
	reply.writeInt("STAS", 2)
	reply.writeInt("XREF", 0)

	reply.append("00") #END PDTL
	reply.writeInt("UID ", self.GAMEOBJ.PersonaID)
	reply.append("00") #EMD SESS

	reply.writeBool("SPAM", False)
	reply.writeBool("UNDR", False)

	####

	self.transport.getHandle().sendall(reply.build().decode('Hex'))


	reply = BlazeFuncs.BlazePacket("7802","0008","0000","2000")
	reply.writeInt("ALOC", 1403663841)
	reply.writeInt("BUID", 321422442)
	reply.writeString("DSNM",self.GAMEOBJ.Name)
	reply.writeBool("FRST", False)
	reply.writeString("KEY ","SessionKey_1337")
	reply.writeInt("LAST", 1403663841)
	reply.writeInt("LLOG", 1403663841)
	reply.writeString("MAIL",mail) #mail to server
	reply.writeInt("PID ", self.GAMEOBJ.PersonaID)
	reply.writeInt("PLAT", 4)
	reply.writeInt("UID ", self.GAMEOBJ.PersonaID)
	reply.writeInt("USTP", 0)
	reply.writeInt("XREF", 0)
	pack1, pack2 = reply.build()
	self.transport.getHandle().sendall(pack1.decode('Hex'))
	self.transport.getHandle().sendall(pack2.decode('Hex'))

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
	self.transport.getHandle().sendall(pack1.decode('Hex'))
	self.transport.getHandle().sendall(pack2.decode('Hex'))

	reply = BlazeFuncs.BlazePacket("7802","0005","0000","2000")
	reply.writeInt("FLGS", 3)
	reply.writeInt("ID  ", self.GAMEOBJ.PersonaID)
	pack1, pack2 = reply.build()
	self.transport.getHandle().sendall(pack1.decode('Hex'))
	self.transport.getHandle().sendall(pack2.decode('Hex'))



def LoginPersona(self,data_e):
	packet = BlazeFuncs.BlazeDecoder(data_e)

	personaName = packet.getVar("PNAM")
	self.GAMEOBJ.Name = personaName
	print("Trying to login to persona (" + personaName + ")")

	reply = BlazeFuncs.BlazePacket("0001","006e",packet.packetID,"1000")
	reply.writeInt("BUID", self.GAMEOBJ.PersonaID)
	reply.writeBool("FRST", False)
	reply.writeString("KEY ", self.GAMEOBJ.SessionKey)
	reply.writeInt("LLOG", 1403663841)
	reply.writeString("MAIL", "bf4.server.pc@ea.com")
	reply.writeSStruct("PDTL")
	reply.writeString("DSNM", self.GAMEOBJ.Name)
	reply.writeInt("LAST", 1403663841)
	reply.writeInt("PID ", self.GAMEOBJ.PersonaID)
	reply.writeInt("STAS", 2)
	#STAS 0 = Unknown
	#STAS 1 = Pending
	#STAS 2 = Active
	#STAS 3 = Deactivated
	#STAS 4 = Disabled
	#STAS 5 = Deleted
	#STAS 6 = Banned
	reply.writeInt("XREF", 0)
	reply.writeInt("XTYP", 0)

	##todo writeestruct
	reply.writeEUnion()

	reply.writeInt("UID ",self.GAMEOBJ.UserID)
	self.transport.getHandle().sendall(reply.build().decode('Hex'))

	reply = BlazeFuncs.BlazePacket("7802","0005","0000","2000")
	reply.writeInt("FLGS", 3)
	reply.writeInt("ID  ", self.GAMEOBJ.PersonaID)
	pack1, pack2 = reply.build()
	self.transport.getHandle().sendall(pack1.decode('Hex'))
	self.transport.getHandle().sendall(pack2.decode('Hex'))

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
	reply.writeEUnion() #END USER
	pack1, pack2 = reply.build()
	self.transport.getHandle().sendall(pack1.decode('Hex'))
	self.transport.getHandle().sendall(pack2.decode('Hex'))

def listUserEntitlements2(self, data_e):
	packet = BlazeFuncs.BlazeDecoder(data_e)
	etag = packet.getVar("ETAG")

	reply = BlazeFuncs.BlazePacket("0001","001D",packet.packetID,"1000")
	if etag == "":
		file = open('Data/global_entitlements.json', 'r')

		jsonStr = file.read()
		jsonObj = json.loads(jsonStr)

		gnls = packet.getVar("GNLS")
		reply.writeArray("NLST")

		for i in range(len(jsonObj)):
			reply.writeArray_TString("DEVI", "")
			reply.writeArray_TString("GDAY", "")
			reply.writeArray_TString("GNAM", jsonObj[i]["GroupName"])
			reply.writeArray_TInt("ID  ", 1)
			reply.writeArray_TInt("ISCO", 0)
			reply.writeArray_TInt("PID ", 0)
			reply.writeArray_TString("PJID", "")
			reply.writeArray_TInt("PRCA", 2)
			reply.writeArray_TString("PRID", jsonObj[i]["PRID"])
			reply.writeArray_TInt("STAT", 1)
			reply.writeArray_TInt("STRC", 0)
			reply.writeArray_TString("TAG ", jsonObj[i]["TAG"])
			reply.writeArray_TString("TDAY", "")
			reply.writeArray_TInt("TYPE", jsonObj[i]["Type"])
			reply.writeArray_TInt("UCNT", 0)
			reply.writeArray_TInt("VER ", 0)
			reply.writeArray_ValEnd()
		reply.writeBuildArray("Struct")

		file.close()
	self.transport.getHandle().sendall(reply.build().decode('Hex'))


def ReciveComponent(self,func,data_e):
	func = func.upper()
	if func == "0010":
		print("[AUTH] CreateAccount")
	elif func == "0014":
		print("[AUTH] UpdateAccount")
	elif func == "001E":
		print("[AUTH] GetAccount")
	elif func == "0028":
		print("[AUTH] Login")
	elif func == "0029":
		print("[AUTH] AcceptTOS")
	elif func == "00F1":
		print("[AUTH] AcceptLegalDocs")
	elif func == "002A":
		print("[AUTH] GetTOSInfo")
	elif func == "00F2":
		print("[AUTH] GetLegalDocsInfo")
	elif func == "002C":
		print("[AUTH] ConsumeCode")
	elif func == "002D":
		print("[AUTH] PasswordForgot")
	elif func == "002E":
		print("[AUTH] GetTermsAndConditionsContent")
	elif func == "00F6":
		print("[AUTH] GetTermsOfServiceContent")
	elif func == "002F":
		print("[AUTH] GetPrivacyPolicyContent")
	elif func == "0032":
		print("[AUTH] SilentLogin")
		SilentLogin(self,data_e)
	elif func == "0033":
		print("[AUTH] CheckAgeReq")
	elif func == "0098":
		print("[AUTH] OriginLogin")
	elif func == "003C":
		print("[AUTH] ExpressLogin")
		Login(self,data_e)
	elif func == "0046":
		print("[AUTH] LogOut")
	elif func == "0050":
		print("[AUTH] CreatePerson")
	elif func == "005A":
		print("[AUTH] GetPersona")
	elif func == "0064":
		print("[AUTH] ListPersonas")
	elif func == "006E":
		print("[AUTH] LoginPersona")
		LoginPersona(self,data_e)
	elif func == "0078":
		print("[AUTH] LogoutPersona")
	elif func == "008C":
		print("[AUTH] DeletePersona")
	elif func == "008D":
		print("[AUTH] DisablePersona")
	elif func == "008F":
		print("[AUTH] ListDeviceAccounts")
	elif func == "0096":
		print("[AUTH] XboxCreateAccount")
	elif func == "00A0":
		print("[AUTH] XboxAssociateAccount")
	elif func == "00AA":
		print("[AUTH] XboxLogin")
	elif func == "00B4":
		print("[AUTH] PS3CreateAccount")
	elif func == "00BE":
		print("[AUTH] PS3AssociateAccount")
	elif func == "00C8":
		print("[AUTH] PS3Login")
	elif func == "00D2":
		print("[AUTH] ValidateSessionKey")
	elif func == "001C":
		print("[AUTH] UpdateParentalEmail")
	elif func == "001D":
		print("[AUTH] ListUserEntitlements2")
		listUserEntitlements2(self, data_e)
	elif func == "0030":
		print("[AUTH] ListPersonaEntitlements2")
	elif func == "0027":
		print("[AUTH] GrantEntitlement2")
	elif func == "001F":
		print("[AUTH] GrantEntitlement")
	elif func == "0020":
		print("[AUTH] ListEntitlements")
	elif func == "0021":
		print("[AUTH] HasEntitlement")
	elif func == "0022":
		print("[AUTH] GetUseCount")
	elif func == "0023":
		print("[AUTH] DecrementUseCount")
	elif func == "0024":
		print("[AUTH] GetAuthToken")
	elif func == "0025":
		print("[AUTH] GetHandoffToken")
	elif func == "0026":
		print("[AUTH] GetPasswordRules")
	elif func == "002B":
		print("[AUTH] ModifyEntitlement2")
	elif func == "0034":
		print("[AUTH] GetOptIn")
	elif func == "0035":
		print("[AUTH] EnableOptIn")
	elif func == "0036":
		print("[AUTH] DisableOptIn")
	elif func == "00E6":
		print("[AUTH] CreateWalUserSession")
	else:
		print("[AUTH] ERROR! UNKNOWN FUNC "+func)
