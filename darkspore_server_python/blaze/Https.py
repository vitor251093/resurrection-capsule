from twisted.web import server, resource
from twisted.internet import reactor
import twisted.web.http
import os
from time import sleep
#import MySQLdb
import hashlib
from blaze.Utils import Globals
import threading


playersJoining = []

class Simple(resource.Resource):
	isLeaf = True
	def render_GET(self, request):

		urlPath = request.path.split('/')
		#print request
		#print request.args
		#print request.content.read()
		#print request.content.getvalue()
		#print request.getAllHeaders()
		#print "===================="

		#print("[HTTPS] GET Request...")

		#print(request.path)

		cwd = os.getcwd()
		path = cwd+"\\Data\\"

		if(urlPath[1] == "api"):
			if(urlPath[4] == "battledash"):
				if(urlPath[6] == "widgetdata"):
					return open(path+'battledash.json').read()
			if(urlPath[4] == "persona"):
				if(urlPath[7] == "ingame_metadata"):
					return '{"clubRank": "", "personaId": "'+urlPath[6]+'", "emblemUrl": "", "clubName": "", "countryCode": "US"}'
		elif(urlPath[1] == "bf4"):
			if(urlPath[2] == "battledash"):
				request.setHeader("Access-Control-Allow-Origin", "*")
				return open(path+'battledash.html').read()
		elif(urlPath[1] == "joinclnt"):
			print("[HTTPS-JOIN] User "+str(urlPath[2])+" auth request...")
			global playersJoining
			user_my = str(urlPath[2])
			pass_my = str(urlPath[3])

			mysqlCheckRes = self.checkUserMySql(user_my, pass_my)

			if (mysqlCheckRes == True):


				if (mysqlCheckRes == True):
					print("[HTTPS-JOIN] MySQL user found...")

					if (str(request.getClientIP())+"|"+str(urlPath[2])+"|"+str(urlPath[3]) in playersJoining):
						print("[HTTPS-JOIN] Double auth request!, dropping...")

						request.setHeader("content-type", "text/plain")
						return "doubleAuth"
					else:
						print("[HTTPS-JOIN] Add user, IP: "+str(request.getClientIP())+" Name: "+str(urlPath[2])+" Password: " + str(urlPath[3]))

						playersJoining.append(str(request.getClientIP())+"|"+str(urlPath[2])+"|"+str(urlPath[3]))

						rmvStringThread = str(request.getClientIP())+"|"+str(urlPath[2])+"|"+str(urlPath[3])

						t1 = threading.Thread(target=self.rmvPlayer, args=[rmvStringThread, 120])
						t1.start()

						request.setHeader("content-type", "text/plain")
						return "userOk"
				elif (mysqlCheckRes == False):
					print("[HTTPS-JOIN] User not in Mysql")
					request.setHeader("content-type", "text/plain")
					return "passError"

			else:
				print("[HTTPS-JOIN] MySQL User not found")

				request.setHeader("content-type", "text/plain")
				return "passFail"



		serverXML = open(path+'server.xml')

		request.setHeader("content-type", "text/xml")

		#print("[HTTPS] Response OK!")



		return serverXML.read()



	def checkUserMySql(self, username, password):
		return True
		# db = MySQLdb.connect(Globals.dbHost, Globals.dbUser, Globals.dbPass, Globals.dbDatabase)
		#
		# cursor = db.cursor()
		# password_md5=hashlib.md5(password).hexdigest()
		# #print(username+password_md5)
		#
		# cursor.execute ("SELECT * FROM `users` WHERE username = '" + username + "' && password = '" + password_md5 + "'")
		#
		# if not cursor.rowcount:
		# 	return False
		# else:
		# 	return True
		#
		#
		#
		# cursor.close()
		# db.close()


	def rmvPlayer(self, strRemove, sec):
		print("[HTTPS-JOIN] Removing player: "+str(strRemove)+" after: "+str(sec)+" sec...")
		sleep(sec)

		matching = [s for s in playersJoining if strRemove in s]
		arrayItem = str(matching)

		if arrayItem != "[]":
			playersJoining.remove(strRemove);
			print("[BETA-JOIN-SYSTEM] Player: "+str(strRemove)+" Removed! Join timeout!")


	def render_POST(self, request):
		#print request
		#print request.args
		#print request.content.read()
		#print request.content.getvalue()
		#print list(request.headers)
		#print "===================="

		return '"528591967549a51344692b9e18294e4c8240b7b7"'
