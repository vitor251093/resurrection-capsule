# -*- coding: utf-8 -*-
import blaze.Utils.BlazeFuncs as BlazeFuncs, blaze.Utils.Globals as Globals
import time, os
#import MySQLdb

def Ping(self,data_e):
	packet = BlazeFuncs.BlazeDecoder(data_e)
	reply = BlazeFuncs.BlazePacket("0009","0002",packet.packetID,"1000")
	reply.writeInt("STIM",int(time.time()))
	self.transport.getHandle().sendall(reply.build().decode('Hex'))

def PreAuth(self,data_e):
	packet = BlazeFuncs.BlazeDecoder(data_e)
	#preAuth_Response = "86ebee0000873ca30107333030323934008e99330400130119041b1c0607090a82e0070b80e00381e00382e00383e0031485e00386e003901f8ee9ee0101008efba6038efba60501010510636f6e6e49646c6554696d656f75740004393073001664656661756c745265717565737454696d656f75740004383073000b70696e67506572696f6400043230730016766f69704865616473657455706461746552617465000531303030001a786c7370436f6e6e656374696f6e49646c6554696d656f757400043330300000a6ecf40111626174746c656669656c642d332d706300b69bb20000ba1cf0010a63656d5f65615f696400c29b24010100c2c8740103706300c34867010100c6fcf3038b7c3303c33840011e676f7370726f642d716f7330312e727370632d6c68722e65612e636f6d00c33c00009e9102cee840011072732d70726f642d6c68722d6266330000b2ec00000ab34c330501030704616d7300c33840011d676f7370726f642d716f7330312e6d33642d616d732e65612e636f6d00c33c00009e9102cee840011065612d70726f642d616d732d62663300000464667700c33840011d676f7370726f642d716f7330312e6d33642d6466772e65612e636f6d00c33c00009e9102cee84001116d69732d70726f642d6466772d62663300000469616400c33840011d676f7370726f642d716f7330312e6d33642d6961642e65612e636f6d00c33c00009e9102cee840011065612d70726f642d6961642d6266330000046c617800c33840011d676f7370726f642d716f7330312e6d33642d6c61782e65612e636f6d00c33c00009e9102cee840011065612d70726f642d6c61782d6266330000046c687200c33840011e676f7370726f642d716f7330312e727370632d6c68722e65612e636f6d00c33c00009e9102cee840011072732d70726f642d6c68722d6266330000046e727400c33840011d676f7370726f642d716f7330312e6d33642d6e72742e65612e636f6d00c33c00009e9102cee84001116933642d70726f642d6e72742d62663300000473796400c33840011d676f7370726f642d716f7330312e6d33642d7379642e65612e636f6d00c33c00009e9102cee840011065612d70726f642d7379642d6266330000cf6a640085a088d40800cb3ca3010733303032393400cf6972011e426c617a6520332e31352e30382e302028434c2320393434323635290a00"
	#preAuth_Response = "86ebee0000873ca30107333030323934008e99330400120119041b1c0607090a0b80e00381e00382e00383e0031485e00386e003901f8ee9ee0101008efba6038efba60501010510636f6e6e49646c6554696d656f75740004393073001664656661756c745265717565737454696d656f75740004383073000b70696e67506572696f6400043230730016766f69704865616473657455706461746552617465000531303030001a786c7370436f6e6e656374696f6e49646c6554696d656f757400043330300000a6ecf40111626174746c656669656c642d332d706300b69bb20000ba1cf0010a63656d5f65615f696400c29b24010100c2c8740103706300c34867010100c6fcf3038b7c3303c33840010931302e302e302e3100c33c00009e9102cee84001047a6c6f0000b2ec00000ab34c330501030104616d7300c33840010931302e302e302e3100c33c00009e9102cee84001047a6c6f0000cf6a640085a088d40800cb3ca3010733303032393400cf6972011d5a4c4f7a6520332e31352e30382e302028434c23203833343535362900"
	preAuth_Response = "873ca30107333032313233008e993304001688e0030189e003198ae0031b041c0607090a82e007230f80e00382e00383e00384e00386e003901f87e0038efba6038efba6050101111e6173736f63696174696f6e4c697374536b6970496e697469616c5365740002310014626c617a65536572766572436c69656e7449640017474f532d426c617a655365727665722d4246342d50430012627974657661756c74486f73746e616d65001e627974657661756c742e67616d6573657276696365732e65612e636f6d000e627974657661756c74506f7274000634323231300010627974657661756c74536563757265000574727565001863617073537472696e6756616c69646174696f6e557269001c636c69656e742d737472696e67732e78626f786c6976652e636f6d0010636f6e6e49646c6554696d656f75740004393073001664656661756c745265717565737454696d656f7574000436307300136964656e74697479446973706c61795572690011636f6e736f6c65322f77656c636f6d6500146964656e7469747952656469726563745572690019687474703a2f2f3132372e302e302e312f73756363657373000f6e75636c657573436f6e6e656374001868747470733a2f2f6163636f756e74732e65612e636f6d000d6e75636c65757350726f7879001768747470733a2f2f676174657761792e65612e636f6d000b70696e67506572696f640004333073001a757365724d616e616765724d617843616368656455736572730002300016766f69704865616473657455706461746552617465000531303030000c78626c546f6b656e55726e00106163636f756e74732e65612e636f6d001a786c7370436f6e6e656374696f6e49646c6554696d656f757400043330300000973ca3010733303231323300a6ecf40111626174746c656669656c642d342d706300b69bb20000ba1cf0010a63656d5f65615f696400c29b24010100c2c8740103706300c6fcf3038b7c3303c33840012a716f732d70726f642d62696f2d6475622d636f6d6d6f6e2d636f6d6d6f6e2e676f732e65612e636f6d00c33c00009e9102cee840011162696f2d70726f642d616d732d6266340000b2ec00000ab34c330501030604616d7300c33840012a716f732d70726f642d62696f2d6475622d636f6d6d6f6e2d636f6d6d6f6e2e676f732e65612e636f6d00c33c00009e9102cee840011162696f2d70726f642d616d732d62663400000467727500c33840012a716f732d70726f642d6d33642d62727a2d636f6d6d6f6e2d636f6d6d6f6e2e676f732e65612e636f6d00c33c00009e9102cee840010100000469616400c33840012a716f732d70726f642d62696f2d6961642d636f6d6d6f6e2d636f6d6d6f6e2e676f732e65612e636f6d00c33c00009e9102cee840011162696f2d70726f642d6961642d6266340000046c617800c33840012a716f732d70726f642d62696f2d736a632d636f6d6d6f6e2d636f6d6d6f6e2e676f732e65612e636f6d00c33c00009e9102cee840011162696f2d70726f642d6c61782d6266340000046e727400c33840012a716f732d70726f642d6d33642d6e72742d636f6d6d6f6e2d636f6d6d6f6e2e676f732e65612e636f6d00c33c00009e9102cee84001116933642d70726f642d6e72742d62663400000473796400c33840012a716f732d70726f642d62696f2d7379642d636f6d6d6f6e2d636f6d6d6f6e2e676f732e65612e636f6d00c33c00009e9102cee840011162696f2d70726f642d7379642d6266340000cf6a640085a088d408d29b650080ade20400cb3ca3010733303231323300cf69720120426c617a652031332e332e312e382e302028434c232031313438323639290a00"
	reply = BlazeFuncs.BlazePacket("0009","0007",packet.packetID,"1000")
	'''
	reply.writeBool("ANON", False)
	reply.writeString("ASRC", "300294")

	reply.append("8e99330400130119041b1c0607090a82e0070b80e00381e00382e00383e0031485e00386e003901f")

	reply.writeSStruct("CONF") #CONF STRUCT 1

	reply.writeMap("CONF")
	reply.writeMapData("associationListSkipInitialSet", "1")
	reply.writeMapData("blazeServerClientId", "GOS-BlazeServer-BF4-PC")
	reply.writeMapData("bytevaultHostname", "bytevault.gameservices.ea.com")
	reply.writeMapData("bytevaultPort", "42210")
	reply.writeMapData("bytevaultSecure", "false")
	reply.writeMapData("capsStringValidationUri", "client-strings.xboxlive.com")
	reply.writeMapData("connIdleTimeout", "90s")
	reply.writeMapData("defaultRequestTimeout", "60s")
	reply.writeMapData("identityDisplayUri", "console2/welcome")
	reply.writeMapData("identityRedirectUri", "http://127.0.0.1/success")
	reply.writeMapData("nucleusConnect", "http://127.0.0.1")
	reply.writeMapData("nucleusProxy", "http://127.0.0.1/")
	reply.writeMapData("pingPeriod", "20s")
	reply.writeMapData("userManagerMaxCachedUsers", "0")
	reply.writeMapData("voipHeadsetUpdateRate", "1000")
	reply.writeMapData("xblTokenUrn", "http://127.0.0.1")
	reply.writeMapData("xlspConnectionIdleTimeout", "300")
	reply.writeBuildMap()
	reply.writeEUnion() #END MAP

	reply.writeString("INST", "battlefield-4-pc")
	reply.writeBool("MINR", False)
	reply.writeString("NASP", "cem_ea_id")
	reply.writeString("PLAT", "pc")

	reply.writeSStruct("QOSS") #QOSS STRUCT
	reply.writeSStruct("BWPS") #BWPS STRUCT
	reply.writeString("PSA ", Globals.serverIP)
	reply.writeInt("PSP ", 17502)
	reply.writeString("SNA ", "rs-prod-lhr-bf4")
	reply.writeEUnion() #END BWPS

	reply.writeInt("LNP ", 10)
	reply.append("b34c330501030704616d7300c33840011d676f7370726f642d716f7330312e6d33642d616d732e65612e636f6d00c33c00009e9102cee840011065612d70726f642d616d732d62663300000464667700c33840011d676f7370726f642d716f7330312e6d33642d6466772e65612e636f6d00c33c00009e9102cee84001116d69732d70726f642d6466772d62663300000469616400c33840011d676f7370726f642d716f7330312e6d33642d6961642e65612e636f6d00c33c00009e9102cee840011065612d70726f642d6961642d6266330000046c617800c33840011d676f7370726f642d716f7330312e6d33642d6c61782e65612e636f6d00c33c00009e9102cee840011065612d70726f642d6c61782d6266330000046c687200c33840011e676f7370726f642d716f7330312e727370632d6c68722e65612e636f6d00c33c00009e9102cee840011072732d70726f642d6c68722d6266330000046e727400c33840011d676f7370726f642d716f7330312e6d33642d6e72742e65612e636f6d00c33c00009e9102cee84001116933642d70726f642d6e72742d62663300000473796400c33840011d676f7370726f642d716f7330312e6d33642d7379642e65612e636f6d00c33c00009e9102cee840011065612d70726f642d7379642d6266330000");
	reply.writeInt("SVID", 1337)
	reply.writeEUnion() #END QOSS

	reply.writeString("RSRC", "302123")
	reply.writeString("SVER", "Blaze 13.15.08.0 (CL# 9442625)")
	'''
	reply.append(preAuth_Response)
	self.transport.getHandle().sendall(reply.build().decode('Hex'))


def PostAuth(self,data_e):
	packet = BlazeFuncs.BlazeDecoder(data_e)
	reply = BlazeFuncs.BlazePacket("0009","0008",packet.packetID,"1000")
	reply.append("c33cc003864cb301010086d8780000bed8780000c2aa64010100c2fcb40000cb0cb40000d29a64000000d25b2503864cb3011b676f7374656c656d657472792e626c617a65332e65612e636f6d0086ebee0000929ce10101009648f400009a9b3401102d47414d452f434f4d4d2f4558504400b2f8c00093d5f2d60cb69bb20000bafbeb010100c2fcb400849c01ce4b390098ea01ce5cf3010b46577065574a3652727000ceb97901b1015eb9caf7d19cb3ddefcb93afaaff818cbbd8e18b9af6ed9bb6b1e8b0a986c6ceb1e2f4d0a9a6a78eb1baea84d3b3ec8d96a4e0c08183868c98b0e0c089e6c6989ab7c2c9e182eed897e2c2d1a3c7ad99b3e9cab1a3d685cd96f0c6b189c3a68d98b8eed091c3a68d96e5dcd59aa5818000cf08f4008b01cf4a6d010844656661756c7400cf6bad011374656c656d657472792d332d636f6d6d6f6e0000d298eb03864cb3010d31302e31302e37382e31353000c2fcb400a78c01ceb9790180013137343830323835302c31302e31302e37382e3135303a383939392c626174746c656669656c642d342d70632c31302c35302c35302c35302c35302c302c300000d72bf003d2dbf00001")
	reply.writeInt("UID ", self.GAMEOBJ.PersonaID)
	reply.writeEUnion() #END UROP

	self.transport.getHandle().sendall(reply.build().decode('Hex'))

def UserSettingsLoadAll(self,data_e):
	packet = BlazeFuncs.BlazeDecoder(data_e)
	reply = BlazeFuncs.BlazePacket("0009","000c",packet.packetID,"1000")
	'''
	cwd = os.getcwd()
	path = cwd+"\\Users\\"+self.GAMEOBJ.Name+"\\"
	f = open(path+'usersettings.txt', 'r')
	data = f.readline()
	'''

	data = ReadUserSettingsMySql(self.GAMEOBJ.Name)

	reply.writeMap("SMAP")
	reply.writeMapData("cust", str(data))
	reply.writeBuildMap()
	self.transport.getHandle().sendall(reply.build().decode('Hex'))

	#f.close()

def GetTelementryServer(self,data_e):
	packet = BlazeFuncs.BlazeDecoder(data_e)
	reply = BlazeFuncs.BlazePacket("0009","0005",packet.packetID,"1000")
	'''
	reply.writeString("ADRS", Globals.serverIP)
	reply.writeBool("ANON", False)
	reply.writeString("DISA", "")
	reply.writeInt("LOC ", 1701729619)
	reply.writeString("NOOK", "US,CA,MX")
	reply.writeInt("PORT", 9988)
	reply.writeInt("SDLY", 15000)
	reply.writeString("SESS", "2fj3StGgjcB")
	reply.writeString("SKEY", "daywwdh")
	reply.writeInt("SPCT", 30)
	reply.writeString("STIM", "Default")
	'''
	reply.append("864cb3011b676f7374656c656d657472792e626c617a65332e65612e636f6d0086ebee0000929ce10101009648f400009a9b3401102d47414d452f434f4d4d2f4558504400b2f8c00093d5f2d60cb69bb20000bafbeb010100c2fcb400849c01ce4b390098ea01ce5cf3010b46577065574a3652727000ceb979019e015ea8e28687a0ca81a2b087a7eb8bf3e4b3beb38a9af6ed9bb6b1e8b0e1c2848c98b0e0c08183868c98e1ec88a3f3a69899ace08dfba2ac98baf4d895b396ad99b6e4dad0e982ee9896b1e8d48183e78d9ab2e8d4e1d2ccdbaad394808000cf08f4008b01cf4a6d010844656661756c7400cf6bad011374656c656d657472792d332d636f6d6d6f6e00")
	self.transport.getHandle().sendall(reply.build().decode('Hex'))

def FetchClientConfig(self,data_e):
	packet = BlazeFuncs.BlazeDecoder(data_e)
	cfid = packet.getVar("CFID")
	reply = BlazeFuncs.BlazePacket("0009","0001",packet.packetID,"1000")
	if cfid == "GOSAchievements":
		print "GOSAchievements"
		reply.writeMap("CONF")
		reply.writeMapData("Achievements", "ACH32_00,ACH33_00,ACH34_00,ACH35_00,ACH36_00,ACH37_00,ACH38_00,ACH39_00,ACH40_00,XPACH01_00,XPACH02_00,XPACH03_00,XPACH04_00,XPACH05_00,XP2ACH01_00,XP2ACH04_00,XP2ACH03_00,XP2ACH05_00,XP2ACH02_00,XP3ACH01_00,XP3ACH05_00,XP3ACH03_00,XP3ACH04_00,XP3ACH02_00,XP4ACH01_00,XP4ACH02_00,XP4ACH03_00,XP4ACH04_00,XP4ACH05_00,XP5ACH01_00,XP5ACH02_00,XP5ACH03_00,XP5ach04_00,XP5ach05_00")
		reply.writeMapData("WinCodes", "r01_00,r05_00,r04_00,r03_00,r02_00,r10_00,r08_00,r07_00,r06_00,r09_00,r11_00,r12_00,r13_00,r14_00,r15_00,r16_00,r17_00,r18_00,r19_00,r20_00,r21_00,r22_00,r23_00,r24_00,r25_00,r26_00,r27_00,r28_00,r29_00,r30_00,r31_00,r32_00,r33_00,r35_00,r36_00,r37_00,r34_00,r38_00,r39_00,r40_00,r41_00,r42_00,r43_00,r44_00,r45_00,xp2rgm_00,xp2rntdmcq_00,xp2rtdmc_00,xp3rts_00,xp3rdom_00,xp3rnts_00,xp3rngm_00,xp4rndom_00,xp4rscav_00,xp4rnscv_00,xp4ramb1_00,xp4ramb2_00,xp5r502_00,xp5r501_00,xp5ras_00,xp5asw_00")
		reply.writeBuildMap()

	self.transport.getHandle().sendall(reply.build().decode('Hex'))


def UserSettingsSave(self, data_e):
	packet = BlazeFuncs.BlazeDecoder(data_e)
	data = packet.getVar("DATA")


	WriteUserSettingsMySql(self.GAMEOBJ.Name, data)

	'''
	cwd = os.getcwd()
	path = cwd+"\\Users\\"+self.GAMEOBJ.Name+"\\"

	f = open(path+'usersettings.txt', 'w')
	f.write(data)
	f.close()
	'''

	reply = BlazeFuncs.BlazePacket("0009","000b",packet.packetID,"1000")
	self.transport.getHandle().sendall(reply.build().decode('Hex'))

def SetUserMode(self, data_e):
	packet = BlazeFuncs.BlazeDecoder(data_e)
	mode = packet.getVar("MODE")
	reply = BlazeFuncs.BlazePacket("0009","001c",packet.packetID,"1000")
	self.transport.getHandle().sendall(reply.build().decode('Hex'))

def ReciveComponent(self,func,data_e):
	func = func.upper()
	if func == "0001":
		print("[UTIL] FetchClientConfig")
		FetchClientConfig(self,data_e)
	elif func == "0002":
		Ping(self,data_e)
	elif func == "0003":
		print("[UTIL] SendClientData")
	elif func == "0004":
		print("[UTIL] LocalizeStrings")
	elif func == "0005":
		print("[UTIL] GetTelementryServer")
		GetTelementryServer(self,data_e)
	elif func == "0006":
		print("[UTIL] GetTickerServer")
	elif func == "0007":
		print("[UTIL] PreAuth")
		PreAuth(self,data_e)
	elif func == "0008":
		print("[UTIL] PostAuth")
		PostAuth(self,data_e)
	elif func == "000A":
		print("[UTIL] UserSettingsLoad")
	elif func == "000B":
		print("[UTIL] UserSettingsSave")
		UserSettingsSave(self, data_e)
	elif func == "000C":
		print("[UTIL] UserSettingsLoadAll")
		UserSettingsLoadAll(self,data_e)
	elif func == "000E":
		print("[UTIL] DeleteUserSettings")
	elif func == "0014":
		print("[UTIL] FilterForProfanity")
	elif func == "0015":
		print("[UTIL] FetchQOSConfig")
	elif func == "0016":
		print("[UTIL] SetClientMetrics")
	elif func == "0017":
		print("[UTIL] SetConnectionState")
	elif func == "0018":
		print("[UTIL] GetPassConfig")
	elif func == "0019":
		print("[UTIL] GetUserOptions")
	elif func == "001A":
		print("[UTIL] SetUserOptions")
	elif func == "001B":
		print("[UTIL] SuspendUserPing")
	elif func == "001C":
		print("[UTIL] SetUserMode")
		SetUserMode(self,data_e)
	else:
		print("[UTIL] ERROR! UNKNOWN FUNC "+func)

def ReadUserSettingsMySql(user):
	print "TEMP: ReadUserSettingsMySql"
	#Query example: SELECT usersettings FROM `users` WHERE username = 'StoCazzo'

	# db = MySQLdb.connect(Globals.dbHost, Globals.dbUser, Globals.dbPass, Globals.dbDatabase)
	#
	# cursor = db.cursor()
	#
	# sql = "SELECT usersettings FROM `users` WHERE username = '"+str(user)+"'"
	#
	# try:
	#    cursor.execute(sql)
	#    results = cursor.fetchall()
	#    for row in results:
	# 	  settings = row[0]
	# 	  return settings
	# except:
	#    print "[MySQL] Can't load usersettings!"
	#    db.close()
	#
	# db.close()

def WriteUserSettingsMySql(user, data):
	print "TEMP: WriteUserSettingsMySql"
	#Query Example: UPDATE `users` SET `usersettings` = 'helloguys' WHERE `users`.`username` = 'StoCazzo'

	# db = MySQLdb.connect(Globals.dbHost, Globals.dbUser, Globals.dbPass, Globals.dbDatabase)
	#
	# cursor = db.cursor()
	#
	# sql = "UPDATE `users` SET `usersettings` = '"+str(data)+"' WHERE `users`.`username` = '"+str(user)+"'"
	#
	# try:
	#    cursor.execute(sql)
	#    db.commit()
	# except:
	#    print "[MySQL] Can't write usersettings!"
	#    db.rollback()
	#
	# db.close()
