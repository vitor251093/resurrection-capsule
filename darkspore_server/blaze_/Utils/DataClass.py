class BF3Client:
	Type       	= 'Client'
	UserID     	= 1
	PersonaID  	= 1
	Name		= 'Test'
	SessionKey	= ''
	EMail		= 'test@test.com'
	LoginKey	= ''
	
	Ver        	= ''
	Platform   	= ''

	CurServer	= None
	
	#Networking
	INTIP	  	= 0
	INTPORT   	= 3659
	EXTIP     	= 0
	EXTPORT   	= 3659
	MacAddr   	= 0
	NetworkInt 	= None
	
	IsUp   		= False

class BF3Server:
	Type       = 'Server'
	
	#Defaults
	UserID     	= 100000
	PersonaID  	= 100000
	Name		= 'bf4-pc'
	SessionKey	= ''
	
	#Default Blaze Handling
	GameName 	= ""
	GameID    	= 0
	GameState 	= 0
	GameSet 	= 0
	GameRegis	= 0
	PGID      	= None
	UUID      	= None
	
	#Networking
	INTIP	  	= 0
	INTPORT   	= 25200
	EXTIP     	= 0
	EXTPORT   	= 25200
	NetworkInt 	= None

	#Data
	AttrNames 	= None
	AttrData 	= None
	Map       	= None
	
	PingSite	= ''
	
	MaxPlayers  = 128
	MaxSpectat  = 8
	Players   	= None
	Commanders 	= [0,0]
	Spectators	= []
	IsUp   		= False


	def __init__(self):
		self.Players = list()
		for i in range(self.MaxPlayers):
			self.Players.append(0)

	def playerJoin(self, pid):
		for i in range(self.MaxPlayers):
			if self.Players[i] != pid and self.Players[i] == 0:
				self.Players[i] = pid
				return i
		return False

	def playerLeave(self, pid):
		for i in range(self.MaxPlayers):
			if self.Players[i] == pid:
				self.Players[i] = 0
				return True
		return False
		
	def getPIDFromMesh(self, mesh):
		if self.Players[mesh] != 0:
			return self.Players[mesh]
		return False

	def getPlayers(self):
		tPlayers = 0
		for i in range(self.MaxPlayers):
			if self.Players[i] != 0:
				tPlayers += 1
		return tPlayers