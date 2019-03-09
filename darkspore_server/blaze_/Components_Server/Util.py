# -*- coding: utf-8 -*-
import Utils.BlazeFuncs as BlazeFuncs, Utils.Globals as Globals
import time

def Ping(self,data_e):
	packet = BlazeFuncs.BlazeDecoder(data_e)
	reply = BlazeFuncs.BlazePacket("0009","0002",packet.packetID,"1000")
	reply.writeInt("STIM",int(time.time()))
	self.transport.getHandle().sendall(reply.build().decode('Hex'))

def PreAuth(self,data_e):
	packet = BlazeFuncs.BlazeDecoder(data_e)
	preAuth_Response = "86ebee0000873ca30107333030323934008e99330400130119041b1c0607090a82e0070b80e00381e00382e00383e0031485e00386e003901f8ee9ee0101008efba6038efba60501010510636f6e6e49646c6554696d656f75740004393073001664656661756c745265717565737454696d656f75740004383073000b70696e67506572696f6400043230730016766f69704865616473657455706461746552617465000531303030001a786c7370436f6e6e656374696f6e49646c6554696d656f757400043330300000a6ecf40111626174746c656669656c642d332d706300b69bb20000ba1cf0010a63656d5f65615f696400c29b24010100c2c8740103706300c34867010100c6fcf3038b7c3303c33840011e676f7370726f642d716f7330312e727370632d6c68722e65612e636f6d00c33c00009e9102cee840011072732d70726f642d6c68722d6266330000b2ec00000ab34c330501030704616d7300c33840011d676f7370726f642d716f7330312e6d33642d616d732e65612e636f6d00c33c00009e9102cee840011065612d70726f642d616d732d62663300000464667700c33840011d676f7370726f642d716f7330312e6d33642d6466772e65612e636f6d00c33c00009e9102cee84001116d69732d70726f642d6466772d62663300000469616400c33840011d676f7370726f642d716f7330312e6d33642d6961642e65612e636f6d00c33c00009e9102cee840011065612d70726f642d6961642d6266330000046c617800c33840011d676f7370726f642d716f7330312e6d33642d6c61782e65612e636f6d00c33c00009e9102cee840011065612d70726f642d6c61782d6266330000046c687200c33840011e676f7370726f642d716f7330312e727370632d6c68722e65612e636f6d00c33c00009e9102cee840011072732d70726f642d6c68722d6266330000046e727400c33840011d676f7370726f642d716f7330312e6d33642d6e72742e65612e636f6d00c33c00009e9102cee84001116933642d70726f642d6e72742d62663300000473796400c33840011d676f7370726f642d716f7330312e6d33642d7379642e65612e636f6d00c33c00009e9102cee840011065612d70726f642d7379642d6266330000cf6a640085a088d40800cb3ca3010733303032393400cf6972011e426c617a6520332e31352e30382e302028434c2320393434323635290a00"
	#preAuth_Response = "86ebee0000873ca30107333030323934008e99330400120119041b1c0607090a0b80e00381e00382e00383e0031485e00386e003901f8ee9ee0101008efba6038efba60501010510636f6e6e49646c6554696d656f75740004393073001664656661756c745265717565737454696d656f75740004383073000b70696e67506572696f6400043230730016766f69704865616473657455706461746552617465000531303030001a786c7370436f6e6e656374696f6e49646c6554696d656f757400043330300000a6ecf40111626174746c656669656c642d332d706300b69bb20000ba1cf0010a63656d5f65615f696400c29b24010100c2c8740103706300c34867010100c6fcf3038b7c3303c33840010931302e302e302e3100c33c00009e9102cee84001047a6c6f0000b2ec00000ab34c330501030104616d7300c33840010931302e302e302e3100c33c00009e9102cee84001047a6c6f0000cf6a640085a088d40800cb3ca3010733303032393400cf6972011d5a4c4f7a6520332e31352e30382e302028434c23203833343535362900"
	reply = BlazeFuncs.BlazePacket("0009","0007",packet.packetID,"1000")
	reply.writeBool("ANON", False)
	reply.writeString("ASRC", "300294")
	
	reply.append("8e99330400130119041b1c0607090a82e0070b80e00381e00382e00383e0031485e00386e003901f")

	reply.writeSStruct("CONF") #CONF STRUCT 1
	
	reply.writeMap("CONF")
	reply.writeMapData("connIdleTimeout", "120s")
	reply.writeMapData("defaultRequestTimeout", "80s")
	reply.writeMapData("pingPeriod", "20s")
	reply.writeMapData("voipHeadsetUpdateRate", "1000")
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
	
	reply.writeString("RSRC", "300294")
	reply.writeString("SVER", "Blaze 13.15.08.0 (CL# 9442625)")
	#reply.append(preAuth_Response)
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
	reply.writeMap("SMAP")
	reply.writeMapData("cust", "")
	reply.writeMapData("sdt", "")
	reply.writeBuildMap()
	self.transport.getHandle().sendall(reply.build().decode('Hex'))
	
def GetTelementryServer(self,data_e):
	packet = BlazeFuncs.BlazeDecoder(data_e)
	reply = BlazeFuncs.BlazePacket("0009","0005",packet.packetID,"1000")
	reply.writeString("ADRS", Globals.serverIP)
	reply.writeBool("ANON", False)
	reply.writeString("DISA", "AD,AF,AG,AI,AL,AM,AN,AO,AQ,AR,AS,AW,AX,AZ,BA,BB,BD,BF,BH,BI,BJ,BM,BN,BO,BR,BS,BT,BV,BW,BY,BZ,CC,CD,CF,CG,CI,CK,CL,CM,CN,CO,CR,CU,CV,CX,DJ,DM,DO,DZ,EC,EG,EH,ER,ET,FJ,FK,FM,FO,GA,GD,GE,GF,GG,GH,GI,GL,GM,GN,GP,GQ,GS,GT,GU,GW,GY,HM,HN,HT,ID,IL,IM,IN,IO,IQ,IR,IS,JE,JM,JO,KE,KG,KH,KI,KM,KN,KP,KR,KW,KY,KZ,LA,LB,LC,LI,LK,LR,LS,LY,MA,MC,MD,ME,MG,MH,ML,MM,MN,MO,MP,MQ,MR,MS,MU,MV,MW,MY,MZ,NA,NC,NE,NF,NG,NI,NP,NR,NU,OM,PA,PE,PF,PG,PH,PK,PM,PN,PS,PW,PY,QA,RE,RS,RW,SA,SB,SC,SD,SG,SH,SJ,SL,SM,SN,SO,SR,ST,SV,SY,SZ,TC,TD,TF,TG,TH,TJ,TK,TL,TM,TN,TO,TT,TV,TZ,UA,UG,UM,UY,UZ,VA,VC,VE,VG,VN,VU,WF,WS,YE,YT,ZM,ZW,ZZ")
	reply.writeInt("LOC ", 1701729619)
	reply.writeString("NOOK", "US,CA,MX")
	reply.writeInt("PORT", 9988)
	reply.writeInt("SDLY", 15000)
	reply.writeString("SESS", "JTO+UtGgjcB")
	reply.writeString("SKEY", "^º»Æš½×þà…ò—¥Ý¼Û’†‰³¶¦Ë¹åäÙ«¦®‹¸ãØ°¡‚†Œ˜°àÀƒ¦Ì›²æàãÆ˜–ðÆ½‘–ŒºìÊ™ËÖŒ›²­æ´·ŒË˜´äÔ©“ç›´â°©æ­Õ©Š€€")
	#reply.append("ceb9790101055e8acbddf8ecc1959899f994c0adeefccea487de8aa6cedcb0eee8e5b3f5ad9ab2e5e4b19986c78e9bb0f4c081a3a78d9cbac289d3c3ac9896a4e0c08183868c98b0e0cc8993c6cc9ae4c899e382eed897edc2cd9bd7cc99b3e5c6d1ebb2a68bb8e3d8c4a183c68c9cb6f0d0c19387cbb2ee8895d2808000")
	reply.writeInt("SPCT", 30)
	reply.writeString("STIM", "Default")
	self.transport.getHandle().sendall(reply.build().decode('Hex'))
	
def FetchClientConfig(self,data_e):
	packet = BlazeFuncs.BlazeDecoder(data_e)
	cfid = packet.getVar("CFID")
	reply = BlazeFuncs.BlazePacket("0009","0001",packet.packetID,"1000")
	if cfid == "GOSAchievements":
		reply.writeMap("CONF")
		reply.writeMapData("Achievements", "ACH32_00,ACH33_00,ACH34_00,ACH35_00,ACH36_00,ACH37_00,ACH38_00,ACH39_00,ACH40_00,XPACH01_00,XPACH02_00,XPACH03_00,XPACH04_00,XPACH05_00,XP2ACH01_00,XP2ACH04_00,XP2ACH03_00,XP2ACH05_00,XP2ACH02_00,XP3ACH01_00,XP3ACH05_00,XP3ACH03_00,XP3ACH04_00,XP3ACH02_00,XP4ACH01_00,XP4ACH02_00,XP4ACH03_00,XP4ACH04_00,XP4ACH05_00,XP5ACH01_00,XP5ACH02_00,XP5ACH03_00,XP5ach04_00,XP5ach05_00")
		reply.writeMapData("WinCodes", "r01_00,r05_00,r04_00,r03_00,r02_00,r10_00,r08_00,r07_00,r06_00,r09_00,r11_00,r12_00,r13_00,r14_00,r15_00,r16_00,r17_00,r18_00,r19_00,r20_00,r21_00,r22_00,r23_00,r24_00,r25_00,r26_00,r27_00,r28_00,r29_00,r30_00,r31_00,r32_00,r33_00,r35_00,r36_00,r37_00,r34_00,r38_00,r39_00,r40_00,r41_00,r42_00,r43_00,r44_00,r45_00,xp2rgm_00,xp2rntdmcq_00,xp2rtdmc_00,xp3rts_00,xp3rdom_00,xp3rnts_00,xp3rngm_00,xp4rndom_00,xp4rscav_00,xp4rnscv_00,xp4ramb1_00,xp4ramb2_00,xp5r502_00,xp5r501_00,xp5ras_00,xp5asw_00")
		reply.writeBuildMap()

	self.transport.getHandle().sendall(reply.build().decode('Hex'))
	
def setClientMetrics(self, data_e):
	packet = BlazeFuncs.BlazeDecoder(data_e)
	reply = BlazeFuncs.BlazePacket("0009","0016",packet.packetID,"1000")
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
	elif func == "000C":
		print("[UTIL] UserSettingsLoadAll")
		UserSettingsLoadAll(self,data_e)
	elif func == "000E":
		print("[UTIL] DeleteUserSettings")
	elif func == "00014":
		print("[UTIL] FilterForProfanity")
	elif func == "0015":
		print("[UTIL] FetchQOSConfig")
	elif func == "0016":
		print("[UTIL] SetClientMetrics")
		setClientMetrics(self, data_e)
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
	else:
		print("[UTIL] ERROR! UNKNOWN FUNC "+func)
	