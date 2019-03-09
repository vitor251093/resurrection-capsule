import struct

class BlazeDecoder:
	fullPacket =  ""
	packetBuffer = ""

	##
	packetSize = ""
	packetComponent = ""
	packetCommand = ""
	packetError = ""
	packetID = ""
	packetType = ""

	mapDName = []
	mapDContent = []

	arrayContent = []

	def __init__(self,packet):
		self.fullPacket = packet
		self.packetBuffer = packet

		# https://github.com/openBlase/openBlase/blob/master/BlaseProxy/main.cpp
		# Line 319

		## Read Packet Size
		self.packetSize = self.__bufferSplice(4)
		self.packetComponent = self.__bufferSplice(4)
		self.packetCommand = self.__bufferSplice(4)
		self.packetError = self.__bufferSplice(4)
		self.packetType = self.__bufferSplice(4)

		##splice trailing zeros
		self.packetID = self.__bufferSplice(4)

	def __bufferSplice(self,pos):
		tempBuff = self.packetBuffer[:pos]
		self.packetBuffer = self.packetBuffer[pos:]
		return tempBuff

	def splice(self,pos):
		self.__bufferSplice(pos)

	def makeBlazeTag(self,tag):
		buffer = 0x00

		test = tag[0].encode('hex')
		test = (hex(int(test, 16) - int('20',16)))
		buffer += int(test,16) << 18

		test = tag[1].encode('hex')
		test = (hex(int(test, 16) - int('20',16)))
		buffer += int(test,16) << 12

		test = tag[2].encode('hex')
		test = (hex(int(test, 16) - int('20',16)))
		buffer += int(test,16) << 6

		test = tag[3].encode('hex')
		test = (hex(int(test, 16) - int('20',16)))
		buffer += int(test,16)

		return hex(buffer).split('x')[1]

	def __splitCount(self,s, count):
		return [''.join(x) for x in zip(*[list(s[z::count]) for z in range(count)])]

	def __decodeInt(self,value):
		if value[:2] == "00":
			value = value[::2]
		x = self.__splitCount(str(value).upper(), 2)
		if (len(x) == 0):
			return 0
		result = int(x[0],16) & 0x3F
		currshift = 6
		for i in range(1, len(x)):
			curByte = int(x[i], 16)
			l1 = (curByte & 0x7F) << currshift
			result|=l1
			currshift += 7
		return result

	def readInt(self,start,data):
		sVal = int(start+2)
		tmpStr = data[start:sVal]
		if int(tmpStr, 16) < 0x80:
			return self.__decodeInt(tmpStr)
		else:
			sVal += 2
			start += 2
			while(int(data[start:sVal], 16) > 0x80):
				tmpStr += data[start:sVal]
				start += 2
				sVal += 2
			tmpStr += data[start:sVal]
			return self.__decodeInt(tmpStr)

	def readArrayInt(self,start,data):
		sVal = int(start+2)
		tmpStr = data[start:sVal]
		if int(tmpStr, 16) < 0x80:
			return (self.__decodeInt(tmpStr), len(tmpStr))
		else:
			sVal += 2
			start += 2
			while(int(data[start:sVal], 16) > 0x80):
				tmpStr += data[start:sVal]
				start += 2
				sVal += 2
			tmpStr += data[start:sVal]
			return (self.__decodeInt(tmpStr), len(tmpStr))

	def readString(self,start,data):
		strLen = start+(int(data[start:start+2], 16)*2)
		string = data[start+2:strLen]
		return(string.decode('Hex'))

	def readVec3(self, start, data):
		dta = []
		data = data[start:]
		#print data
		while(len(data) >= 2):
			start = 0
			sVal = int(start+2)
			tmpStr = data[start:sVal]
			if int(tmpStr, 16) >= 0x80:
				sVal += 2
				start += 2
				while(int(data[start:sVal], 16) > 0x80):
					tmpStr += data[start:sVal]
					start += 2
					sVal += 2
				tmpStr += data[start:sVal]
			sVal = 0
			start = 0
			dta.append(self.__decodeInt((tmpStr)))
			data = data[len(tmpStr):]
		return dta

	def readMap(self, start, data):
		#print data
		#print start
		self.__bufferSplice(start+4)
		amount = int(self.__bufferSplice(2), 16)
		#print "MAP AMOUNT " + str(amount)
		for i in range(amount):
			nameLen = int(self.__bufferSplice(2), 16)*2
			name = self.__bufferSplice(nameLen)
			if len(name)%2 == 1:
				name+="0"
			self.mapDName.append(name[:-2].decode('Hex'))

			#dataLen = int(self.__bufferSplice(2), 16)*2
			dataLen = self.__bufferSplice(2)
			if self.packetBuffer[:1] == "0" and self.packetBuffer[:2] != "00":
				multi = self.__bufferSplice(2)
				dataLen = dataLen+multi
				dataLen = self.__decodeInt(dataLen)

			if type(dataLen) is str:
				dataLen = int(dataLen, 16)*2
			else:
				dataLen = dataLen*2
			data = self.__bufferSplice(dataLen)
			if len(data)%2 == 1:
				data+="0"
			self.mapDContent.append(data[:-2].decode('Hex'))
			#print name[:-2].decode('Hex') + " = " + data[:-2].decode('Hex')

		res1 = list(self.mapDName)
		res2 = list(self.mapDContent)

		for i in range(amount):
			self.mapDName.pop()
			self.mapDContent.pop()

		self.packetBuffer = self.fullPacket
		return (res1, res2)

	def readStatsMap(self, start, data):
		self.__bufferSplice(start+4)
		amount = int(self.__bufferSplice(2), 16)
		for i in range(amount):
			data = list()
			pid, length = self.readArrayInt(0, self.packetBuffer)
			self.__bufferSplice(length)
			self.mapDName.append(pid)
			self.__bufferSplice(12)

			values, length = self.readArrayInt(0, self.packetBuffer)
			self.__bufferSplice(length)
			for i in range(values):
				vals = ["", 0]
				nameLen = int(self.__bufferSplice(2), 16)*2
				name = self.__bufferSplice(nameLen)
				if len(name)%2 == 1:
					name+="0"

				vals[0] = name[:-2].decode('hex')
				vals[1] = struct.unpack('!f', self.__bufferSplice(8).decode('hex'))[0]

				data.append(vals)
			self.__bufferSplice(2)

			self.mapDContent.append(data)

		res1 = list(self.mapDName)
		res2 = list(self.mapDContent)

		for i in range(amount):
			self.mapDName.pop()
			self.mapDContent.pop()

		self.packetBuffer = self.fullPacket
		return (res1, res2)

	def readArray(self, start, data):
		self.__bufferSplice(start)
		aType = self.__bufferSplice(2)
		amount = int(self.__bufferSplice(2), 16)
		if aType == "00":
			for x in (range(amount)):
				num,length = self.readArrayInt(0, self.packetBuffer)
				self.arrayContent.append(num)
				self.__bufferSplice(length)
		elif aType == "01":
			for x in (range(amount)):
				string = self.readString(0,self.packetBuffer)
				self.arrayContent.append(string)
				self.__bufferSplice(len(string.encode('hex'))+4)
		elif aType == "03":
			print "Array type of Struct"
		elif aType == "04":
			print "Array type of Array"
		elif aType == "05":
			print "Array type of Map"
		else:
			print "Array type of Unknown"

		res1 = list(self.arrayContent)
		for i in range(amount):
			self.arrayContent.pop()
		self.packetBuffer = self.fullPacket
		return res1

	def getVarFunc(self, tag, data):
		startVal = data.find(tag)
		tType = data[startVal+6:startVal+8]

		print "tType"
		print tType

		if tType == "00":
			return self.readInt(startVal+8,data)
		elif tType == "01":
			return self.readString(startVal+8,data)
		elif tType == "02":
			print "Blob"
		elif tType == "03":
			print "Struct"
		elif tType == "04":
			return self.readArray(startVal+8, data)
		elif tType == "05":
			return (self.readMap(startVal+8,data))
		elif tType == "06":
			print "Union"
		elif tType == "07":
			print "07 (Unknown)"
		elif tType == "08":
			print "Vector"
		elif tType == "09":
			print "Vector3"
			return (self.readVec3(startVal+8,data))
		else:
			print "Unknown"

	def getStatsVarFunc(self, tag, data):
		startVal = data.find(tag)
		tType = data[startVal+6:startVal+8]

		return self.readStatsMap(startVal+8,data)

	def getVar(self, tag):
		tag = self.makeBlazeTag(tag)
		return (self.getVarFunc(tag, self.packetBuffer))

	def getStatsVar(self, tag):
		tag = self.makeBlazeTag(tag)
		return (self.getStatsVarFunc(tag, self.packetBuffer))

	def getVarValue(self, tag, value):
		tag = self.makeBlazeTag(tag)
		#Todo Splice packetBuffer
		data = self.packetBuffer
		for x in range(0,value-1):
			startVal = data.find(tag)
			data = data[startVal+6:]

		return self.getVarFunc(tag, data)


class BlazePacket:
	packetHeader =  ""
	packetBuffer = ""

	##
	packetSize = ""
	packetComponent = ""
	packetCommand = ""
	packetID = ""
	packetType = ""

	tmapName = ""
	tmapDName = []
	tmapDConent = []

	tarrayName = ""
	tarrayString = ""
	tarrayVals = 0

	tIarrayName = ""
	tIarrayString = ""
	tIarrayVals = 0

	def __init__(self,component,command,packetid,type):
		self.packetComponent = component
		self.packetCommand = command
		self.packetID = packetid
		self.packetType = type
		self.packetHeader = self.packetComponent + self.packetCommand + "0000" + self.packetType + self.packetID

	def __packetSize(self):
		packetStringBUFF = ""
		packetSizeTEMP = int(len(self.packetBuffer)/2)
		if packetSizeTEMP < 16:
			packetStringBUFF = "000"
		elif packetSizeTEMP < 255:
			packetStringBUFF = "00"
		elif packetSizeTEMP < 4096:
			packetStringBUFF = "0"
		packetStringBUFF += hex(len(self.packetBuffer)/2).split('x')[1]
		return packetStringBUFF

	def __encodeInt(self,value):
		result = []
		val = value
		if val < 0x32:
			result.append(val)
		else:
			curbyte = ((val & 0x3F) | 0x80)
			result.append(curbyte)
			currshift = val >> 6
			while (currshift > 0x80):
				curbyte = ((currshift & 0x7F) | 0x80)
				currshift = currshift >> 7
				result.append(curbyte)
			result.append(currshift)
		for i in range(0, len(result)):
			if len(hex(result[i])[2:]) == 1:
				result[i] = "0"+hex(result[i])[2:]
			else:
				result[i] = hex(result[i])[2:]
		while len(result) < 4:
			result[:0] = "0"
			result[0] = "00"

		for i in range(0, len(result)):
			if (result[0] == "00" and len(result) != 1):
				result.pop(0)
		result = "".join(result)
		return result.replace("L", "")

	def makeInt(self, numb):
		return self.__encodeInt(numb)

	def makeBlazeTag(self,tag):
		buffer = 0x00

		test = tag[0].encode('hex')
		test = (hex(int(test, 16) - int('20',16)))
		buffer += int(test,16) << 18

		test = tag[1].encode('hex')
		test = (hex(int(test, 16) - int('20',16)))
		buffer += int(test,16) << 12

		test = tag[2].encode('hex')
		test = (hex(int(test, 16) - int('20',16)))
		buffer += int(test,16) << 6

		test = tag[3].encode('hex')
		test = (hex(int(test, 16) - int('20',16)))
		buffer += int(test,16)

		return hex(buffer).split('x')[1]

	def append(self,str):
		self.packetBuffer += str

	def writeSUnion(self,tag):
		self.append(self.makeBlazeTag(tag))
		self.append("0600")

	def writeEUnion(self):
		self.append("00")

	def writeSStruct(self,tag):
		self.append(self.makeBlazeTag(tag))
		self.append("03")

	def writeString(self,tag,string):
		BUFF = self.makeBlazeTag(tag)
		BUFF += "01"

		BUFF += self.makeInt(int(len(string))+1)

		BUFF += string.encode('Hex')
		BUFF += "00"

		self.append(BUFF)

	def writeSArray(self,tag):
		self.append(self.makeBlazeTag(tag))
		self.append("04")

	def writeArray(self, tag):
		self.tarrayName = tag

	def writeIntArray(self, tag):
		self.tIarrayName = tag

	def writeIntArray_Int(self,data):
		BUFF = ""
		BUFF += self.__encodeInt(data)
		self.tIarrayString += BUFF
		self.tIarrayVals += 1

	def writeArray_TString(self,tag,string):
		BUFF = ""
		BUFF += self.makeBlazeTag(tag)
		BUFF += "01"

		packetSizeTEMP = int(len(string))

		if packetSizeTEMP < 15:
			BUFF += "0"

		BUFF += hex(len(string) + 1).split('x')[1]
		BUFF += string.encode('Hex')
		BUFF += "00"
		self.tarrayString += BUFF

	def writeArray_String(self,string):
		BUFF = ""
		packetSizeTEMP = int(len(string))

		if packetSizeTEMP < 15:
			BUFF += "0"

		BUFF += hex(len(string) + 1).split('x')[1]
		BUFF += string.encode('Hex')
		BUFF += "00"
		self.tarrayString += BUFF
		self.tarrayVals += 1

	def writeArray_Bool(self,bool):
		BUFF = ""
		if(bool):
			BUFF += "01"
		else:
			BUFF += "00"

		self.tarrayString += BUFF
		self.tarrayVals += 1

	def writeArray_Int(self,data):
		BUFF = ""
		BUFF += self.__encodeInt(data)
		self.tarrayString += BUFF

	def writeArray_TInt(self,tag,data):
		BUFF = ""
		BUFF += self.makeBlazeTag(tag)
		BUFF += "00"
		BUFF += self.__encodeInt(data)
		self.tarrayString += BUFF

	def writeArray_ValEnd(self):
		self.tarrayVals += 1
		self.tarrayString += "00"

	def writeBuildArray(self, tpe):
		BUFF = self.makeBlazeTag(self.tarrayName)
		if tpe == "Struct":
			BUFF += "0403"
		elif tpe == "String":
			BUFF += "0401"
		elif (tpe == "Bool" or tpe == "Int"):
			BUFF += "0400"
		BUFF += self.__encodeInt(self.tarrayVals)
		BUFF += self.tarrayString
		self.append(BUFF)
		self.tarrayString = ""
		self.tarrayVals = 0

	def writeBuildIntArray(self):
		BUFF = self.makeBlazeTag(self.tIarrayName)
		BUFF += "0400"
		BUFF += self.__encodeInt(self.tIarrayVals)
		BUFF += self.tIarrayString
		self.append(BUFF)
		self.tIarrayString = ""
		self.tIarrayVals = 0

	def writeMap(self,tag):
		self.tmapName = tag

	def writeMapData(self,dname,data):
		self.tmapDName.append(dname)
		self.tmapDConent.append(data)

	def writeBuildMap(self):
		BUFF = ""
		BUFF  += self.makeBlazeTag(self.tmapName)
		BUFF  += "050101"
		BUFF  += self.__encodeInt(len(self.tmapDName))
		vars = len(self.tmapDName)
		for i in range(vars):
			name = self.tmapDName[i]
			data = self.tmapDConent[i]
			tmp = ""
			#print "BUILD [" + name + " = " + data + "] LEN("+str(len(name)+1)+","+str(len(data)+1)+") HX("+str(self.__encodeInt(len(name) + 1))+","+str(self.__encodeInt(len(data) + 1))+")"

			tmp += self.__encodeInt(len(name) + 1)
			tmp += name.encode('Hex')
			tmp += "00"

			tmp += self.__encodeInt(len(data) + 1)
			tmp += data.encode('Hex')
			tmp += "00"
			BUFF += tmp
			#print tmp
			#print "========================================"

		self.append(BUFF)

		for i in range(vars):
			self.tmapDName.pop()
			self.tmapDConent.pop()

	def writeBool(self,tag,bool):
		self.append(self.makeBlazeTag(tag))
		self.append("00")
		if bool:
			self.append("01")
		else:
			self.append("00")

	def writeInt(self,tag,num):
		self.append(self.makeBlazeTag(tag))
		self.append("00")
		self.append(self.__encodeInt(num))

	def writeVector(self,tag,num1,num2):
		self.append(self.makeBlazeTag(tag))
		self.append("00")
		self.append(self.__encodeInt(num1))
		self.append(self.__encodeInt(num2))

	def writeVector3(self,tag,num1,num2,num3):
		self.append(self.makeBlazeTag(tag))
		self.append("00")
		self.append(self.__encodeInt(num1))
		self.append(self.__encodeInt(num2))
		self.append(self.__encodeInt(num3))

	def build(self):
		if self.packetType == "2000":
			return (self.__packetSize()+self.packetHeader, self.packetBuffer)

		if len(self.packetBuffer) > 0xFFFF:
			size = self.__packetSize()
			size1 = size[-4:]
			size2 = size[:-4]
			if (len(size2) == 1):
				size2 = "000"+size2
			elif (len(size2) == 2):
				size2 = "00"+size2
			elif len(size2) == 3:
				size2 = "0"+size2
			return size1+self.packetHeader+size2+self.packetBuffer

		return self.__packetSize()+self.packetHeader+self.packetBuffer
