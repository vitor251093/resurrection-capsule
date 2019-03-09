import Utils.BlazeFuncs as BlazeFuncs

def getClubMembership(self, data_e):
	packet = BlazeFuncs.BlazeDecoder(data_e)
	reply = BlazeFuncs.BlazePacket("000B","0A8C",packet.packetID,"1000")
	self.transport.getHandle().sendall(reply.build().decode('Hex'))

def ReciveComponent(self,func,data_e):
	func = func.upper()
	if func == '0A8C':
		print("[CLUBS] getClubMembershipForUsers")
	else:
		print("[CLUBS] ERROR! UNKNOWN FUNC "+func)