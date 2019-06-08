import blaze.Utils.BlazeFuncs as BlazeFuncs, time, os, shutil
import blaze.Utils.DataClass as DataClass, blaze.Utils.Globals as Globals, socket, struct, random, string, json

def SilentLogin(self,data_e):

	if self.GAMEOBJ == None:
		self.GAMEOBJ = DataClass.BF3Client()
		self.GAMEOBJ.NetworkInt = self.transport
		Globals.Clients.append(self.GAMEOBJ)

	packedIP = socket.inet_aton(self.transport.getPeer().host)
	self.GAMEOBJ.EXTIP = struct.unpack("!L", packedIP)[0]
	self.GAMEOBJ.IsUp = True
	
	packet = BlazeFuncs.BlazeDecoder(data_e)
	loginKey = packet.getVar("AUTH")
	PID = packet.getVar("PID ")
	
	id = 0
	for c in loginKey:
		id += ord(c)
			
	#path2 = cwd+"\\Stats\\"+str(id)+"\\"
	
	#if not os.path.exists(path2):
	#	os.makedirs(path2)
	
	self.GAMEOBJ.UserID = id
	self.GAMEOBJ.EMail = "notavaliable@gmail.com"

	self.GAMEOBJ.PersonaID = id
	self.GAMEOBJ.Name = loginKey

	self.GAMEOBJ.SessionKey = ''.join(random.choice(string.ascii_uppercase + string.digits) for _ in range(42))
	self.GAMEOBJ.LoginKey = loginKey

	#if not os.path.isfile(path2+"userstats.txt"):
	#	shutil.copy2('Stats/default/userstats.txt', path2)
	
	#if not os.path.isfile(path2+"usersettings.txt"):
	#	shutil.copy2('Stats/default/usersettings.txt', path2)
	
	reply = BlazeFuncs.BlazePacket("0001","0032",packet.packetID,"1000")
	reply.writeInt("AGUP", 0)
	reply.writeString("LDHT", "")
	reply.writeBool("NTOS", False)
	reply.writeString("PCTK", "LoginKey_1337")
	reply.writeString("PRIV", "")
		
	#SESS STRUCT START
	reply.writeSStruct("SESS")
	reply.writeInt("BUID", self.GAMEOBJ.PersonaID)
	reply.writeBool("FRST", False)
	reply.writeString("KEY ", self.GAMEOBJ.SessionKey)
	reply.writeInt("LLOG", 0)
	reply.writeString("MAIL", self.GAMEOBJ.Name)
		
	#PDTL STRUCT START
	reply.writeSStruct("PDTL")
	reply.writeString("DSNM", self.GAMEOBJ.Name)
	reply.writeInt("LAST", 0)
	reply.writeInt("PID ",self.GAMEOBJ.PersonaID)
	reply.writeInt("STAS", 0)
	reply.writeInt("XREF", 0)
	reply.writeInt("XTYP", 0)
	reply.writeEUnion()
	#PDTL STRUCT END
		
	reply.writeInt("UID ",self.GAMEOBJ.UserID)
	#------------------
	reply.writeEUnion()
	#SESS STRUCT END---

	reply.writeBool("SPAM", False)
	reply.writeString("THST", "")
	reply.writeString("TSUI", "")
	reply.writeString("TURI", "")
	self.transport.getHandle().sendall(reply.build().decode('Hex'))
	
	reply = BlazeFuncs.BlazePacket("7802","0002","0000","2000")
	reply.writeSStruct("USER")
	reply.writeInt("AID ", self.GAMEOBJ.UserID)
	reply.writeInt("ALOC", 1701729619)
	reply.writeInt("ID  ", self.GAMEOBJ.PersonaID)
	reply.writeString("NAME", self.GAMEOBJ.Name)
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
	
def UNKNOWN(self, data_e):
	print ""

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
		Login(self,data_e)
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
	elif func == "01F4":
		print("[AUTH] UNKNOWN")
		UNKNOWN(self,data_e)
	else:
		print("[AUTH] ERROR! UNKNOWN FUNC "+func)