#! python2.7
import sys
sys.dont_write_bytecode = True

import BlazeMain_Client
import BlazeMain_Server
import GosRedirector
import Https
#import MySQLdb
import sys
import os

from blaze.Utils import garbage
import blaze.Utils.Globals as Globals

from OpenSSL import SSL
from twisted.internet import ssl, reactor
from twisted.internet.protocol import Factory, Protocol
from twisted.web import server, resource

def verifyCallback(connection, x509, errnum, errdepth, ok):
    if not ok:
        print 'invalid cert from subject:', x509.get_subject()
        return False
    else:
        print "Certs are fine"
    return True

def startBlaze():
	#Globals.serverIP = "192.168.1.5"
	Globals.serverIP = "127.0.0.1"

	#MySQL Data
	Globals.dbHost = "127.0.0.1"
	Globals.dbUser = "user"
	Globals.dbPass = "pass"
	Globals.dbDatabase = "db"

	CheckMySqlConn()

	serverPath = os.path.abspath(os.path.dirname(__file__))
	privkeyPath = os.path.join(serverPath,'crt','privkey.pem')
	cacertPath = os.path.join(serverPath,'crt','cacert.pem')

	# SSLInfo = ssl.DefaultOpenSSLContextFactory(privkeyPath, cacertPath)
	# ctx = SSLInfo.getContext()
	# ctx.set_verify((SSL.VERIFY_PEER | SSL.VERIFY_FAIL_IF_NO_PEER_CERT), verifyCallback)

	factory = Factory()
	factory.protocol = GosRedirector.GOSRedirector
	reactor.listenTCP(42127, factory)
	print("[SSL REACTOR] GOSREDIRECTOR STARTED [42127]")

	factory = Factory()
	factory.protocol = BlazeMain_Client.BLAZEHUB
	reactor.listenTCP(10041, factory)
	print("[TCP REACTOR] BLAZE CLIENT [10041]")

	factory = Factory()
	factory.protocol = BlazeMain_Server.BLAZEHUB
	reactor.listenTCP(10071, factory)
	print("[TCP REACTOR] BLAZE SERVER [10071]")

	# sites = server.Site(Https.Simple())
	# reactor.listenSSL(443, sites, SSLInfo)
	# print("[WEB REACTOR] Https [443]")

	reactor.run()

def CheckMySqlConn():
	print("[MySQL] Checking server connection...")

	# try:
	# 	db = MySQLdb.connect(Globals.dbHost, Globals.dbUser, Globals.dbPass, Globals.dbDatabase)
	# 	cursor = db.cursor()
	# 	cursor.execute("SELECT VERSION()")
	# 	results = cursor.fetchone()
	#
	# 	print("[MySQL] Server connection ok!")
	#
	#
	# except MySQLdb.Error, e:
	# 	print "[MySQL] Server connection failed! Error: %d in connection: %s" % (e.args[0], e.args[1])
	# 	sys.exit()
