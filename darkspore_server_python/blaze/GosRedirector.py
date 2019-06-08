import threading
from twisted.internet import ssl, reactor
from twisted.internet.protocol import Factory, Protocol
import struct, socket

import blaze.Utils.Globals as Globals
import blaze.Utils.BlazeFuncs as BlazeFuncs

class GOSRedirector(Protocol):
	def dataReceived(self, data):
		data_e = data.encode('Hex')
		packet = BlazeFuncs.BlazeDecoder(data_e)

		print "packet.fullPacket"
		print packet.fullPacket
		print "packet.packetBuffer"
		print packet.packetBuffer
		print "packet.packetSize"
		print packet.packetSize
		print "packet.packetComponent"
		print packet.packetComponent
		print "packet.packetCommand"
		print packet.packetCommand
		print "packet.packetError"
		print packet.packetError
		print "packet.packetType"
		print packet.packetType
		print "packet.packetID"
		print packet.packetID

		if packet.packetComponent == '0000' and packet.packetCommand == '2f01':
			self.transport.write(("00"+"00"+"00"+"00"+"00"+"00"+"00"+"00"+
								  "00"+"00"+"00"+"00"+"00"+"00"+"00"+"00"+
								  "00"+"00"+"00"+"00"+
								  "01"+ # 20. Most not be zero
								  "00"+"00"+"00"+
								  "00"+ # 24. Must be zero
								  "00"+"00"+"00"+"00"+"00"+"00"+"00"+"00"+
								  "00"+"00"+"00"+"00"+"00"+"00"+"00"+"00"+
								  "00"+
								  "00"+"00"+ # 42, 43. Must be zero (?)
								  "00"+"00"+"00"+"00"+"00"+
								  "00"+"00"+"00"+"00"+"00"+"00"+"00"+"00"+
								  "00"+"00"+"00"+"00"+"00"+"00"+"00"+"00"+
								  "00"+"00"+"00"+"00"+"00"+"00"+"00"+"00"+
								  "00"+"00"+"00"+"00"+"00"+"00"+"00"+"00"+
								  "00"+"00"+"00"+"00"+"00"+"00"+"00"+"00"+
								  "00"+"00"+"00"+"00"+"00"+"00"+"00"+"00"+
								  "00"+"00"+"00"+"00"+"00"+"00"+"00"+
								  "01"+ # 104. Must not be zero
								  ""
								  ).decode('Hex'))
			# clnt = packet.getVar("CLNT")
			# print "clnt"
			# print clnt
			#
			# srvr = packet.getVar("SRVR")
			# print "srvr"
			# print srvr
			#
			# port = 10041
			# if clnt == "warsaw server":
			# 	port = 10071
			#
			# #ip = ''.join([ bin(int(x))[2:].rjust(8,'0') for x in Globals.serverIP.split('.')])
			# ip = struct.unpack("!I", socket.inet_aton(Globals.serverIP))[0]
			# print "ip"
			# print ip
			#
			# reply = BlazeFuncs.BlazePacket(packet.packetComponent,packet.packetCommand,packet.packetID,packet.packetType)
			#
			# reply.writeSUnion("ADDR")
			# reply.writeSStruct("VALU")
			# reply.writeString("HOST", Globals.serverIP)
			# reply.writeInt("IP  ", ip)
			# reply.writeInt("PORT", port)
			# reply.writeEUnion()
			#
			# reply.writeInt("SECU", 0)
			# reply.writeInt("XDNS", 0)
			# self.transport.write(reply.build().decode('Hex'))

		## REDIRECTORCOMPONENT
		# if packet.packetComponent == '0005' and packet.packetCommand == '0001':
		# 	clnt = packet.getVar("CLNT")
		# 	print "clnt"
		# 	print clnt
		#
		# 	port = 10041
		# 	if clnt == "warsaw server":
		# 		port = 10071
		#
		# 	#ip = ''.join([ bin(int(x))[2:].rjust(8,'0') for x in Globals.serverIP.split('.')])
		# 	ip = struct.unpack("!I", socket.inet_aton(Globals.serverIP))[0]
		# 	print "ip"
		# 	print ip
		#
		# 	reply = BlazeFuncs.BlazePacket("0005","0001",packet.packetID,"1000")
		#
		# 	reply.writeSUnion("ADDR")
		# 	reply.writeSStruct("VALU")
		# 	reply.writeString("HOST", Globals.serverIP)
		# 	reply.writeInt("IP  ", ip)
		# 	reply.writeInt("PORT", port)
		# 	reply.writeEUnion()
		#
		# 	reply.writeInt("SECU", 0)
		# 	reply.writeInt("XDNS", 0)
		# 	self.transport.write(reply.build().decode('Hex'))
