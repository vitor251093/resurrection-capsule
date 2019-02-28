from models.account import *
from controllers.gameApi import *
from controllers.config import *

import os
import sys
import getopt
import warnings
import time
import timeit
import datetime
import json
import socket
import threading
from threading import Thread

import xml.etree.cElementTree as xml_tree
from xml.etree import ElementTree

from flask import Flask
from flask import request
from flask import render_template
from flask import Response
from flask import send_from_directory

import logging

class DarkSporeServer(object):
    def __init__(self):
        self.accounts = {}
        self.accountsSequenceNext = 0
        self.gameVersion = None
        self.version = "0.1"

    def setGameVersion(self, gameVersion):
        if self.gameVersion == None:
            self.gameVersion = gameVersion
        else:
            if self.gameVersion != gameVersion:
                return False
        return True

    def getAccount(self, id):
        return self.accounts[str(id)]

    def createAccount(self, email, name):
        id = self.accountsSequenceNext
        if self.accounts[str(id)] != None:
            return -1

        self.accounts[str(id)] = Account(id, email, name)
        self.accountsSequenceNext += 1
        return id

server = DarkSporeServer()
serverConfig = DarkSporeServerConfig()
serverApi = DarkSporeServerApi(serverConfig, server)
staticFolderPath = os.path.join(os.path.join(serverConfig.storagePath(), 'www'), 'static')
app = Flask(__name__, static_url_path='/static', static_folder=staticFolderPath)

now = datetime.datetime.now()
logFileName = 'dls-' + now.strftime("%Y-%m-%d_%H-%M-%S") + '.log'
handler = logging.FileHandler(os.path.join(serverConfig.storagePath(), logFileName))  # errors logged to this file
handler.setLevel(logging.ERROR)  # only log errors and above
app.logger.addHandler(handler)

darksporeBuild_limitedEditionDvd = "5.3.0.15"
darksporeBuild_onlineInstaller   = "5.3.0.84"  # Released at 27/04/2011
darksporeBuild_steamDemo         = "5.3.0.103" # Released between 23/05/2011 and 14/06/2011
darksporeBuild_latestOfficial    = "5.3.0.127" # Released between 15/11/2011 and 30/11/2012

def setXmlValues(xml, values):
    for key,val in values.items():
        if val is None:
            xml_tree.SubElement(xml, key)
        else:
            xml_tree.SubElement(xml, key).text = val

def jsonResponseWithObject(obj):
    json_data = json.dumps(obj)
    return Response(json_data, mimetype='application/json')

def xmlResponseWithXmlElement(xmlElement):
    tree_str = ElementTree.tostring(xmlElement, encoding='iso-8859-1', method='xml')
    return Response(tree_str, mimetype='text/xml')

def handleBlazeConnection(client_socket):
    request = client_socket.recv(1024)
    print 'Received {}'.format(request)
    client_socket.send('ACK!')
    client_socket.close()

def startBlazeConnection(bind_port,callback):
    bind_ip = '0.0.0.0'

    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind((bind_ip, bind_port))
    server.listen(5)  # max backlog of connections

    print 'Listening on {}:{}'.format(bind_ip, bind_port)

    while True:
        client_sock, address = server.accept()
        print 'Accepted connection from {}:{}'.format(address[0], address[1])
        client_handler = threading.Thread(
            target=callback,
            args=(client_sock,)
        )
        client_handler.start()

@app.route("/api", methods=['GET','POST'])
def api():
    method   = request.args.get('method',   default='')
    version  = request.args.get('version',  default='')
    callback = request.args.get('callback', default='')
    format_type = request.args.get('format',   default='')

    if method == 'api.status.getStatus':
        if callback == 'updateServerStatus(data)' and version == '1' and format_type == 'json':
            javascript = serverApi.api_getStatus_javascript(callback)
            return Response(javascript, mimetype='application/javascript')

    print " "
    print "http://" + request.host + "/api"
    print request.args
    print " "

    return "";

@app.route("/game/api", methods=['GET','POST'])
def gameApi():
    method  = request.args.get('method',  default='')
    version = request.args.get('version', default='')

    if method == 'api.status.getStatus':
        include_broadcasts = request.args.get('include_broadcast', default='')
        statusObj = serverApi.api_getStatus_object(include_broadcasts)
        return jsonResponseWithObject(statusObj)

    print " "
    print "http://" + request.host + "/api"
    print request.args
    print " "

    return jsonResponseWithObject({})

@app.route("/bootstrap/api", methods=['GET','POST'])
def bootstrapApi():
    version = request.args.get('version', default='')
    build   = request.args.get('build',   default='')
    method  = request.args.get('method',  default='')

    # Different players must be using the same version of the game, otherwise
    # there may be issue during a match.
    validVersion = server.setGameVersion(build)
    if serverConfig.versionLockEnabled() and validVersion == False:
        return jsonResponseWithObject({})

    if method == 'api.account.getAccount':
        include_feed  = (request.args.get('include_feed',  default='') == 'true')
        include_decks = (request.args.get('include_decks', default='') == 'true')
        include_creatures = (request.args.get('include_creatures', default='') == 'true')
        player_id = int(request.args.get('id', default='0'))
        callback = request.args.get('callback', default='') # targetaccountinfocallback

        root = xml_tree.Element("response")
        setXmlValues(account, serverApi.bootstrapApi_response_object(version))

        account = xml_tree.SubElement(root, "account")
        setXmlValues(account, serverApi.bootstrapApi_getAccount_object(player_id))

        # Still missing creatures, decks and feed
        if include_feed:
            print "include feed"
        if include_decks:
            print "include decks"
        if include_creatures:
            print "include creatures"

        return xmlResponseWithXmlElement(root)

    if method == 'api.creature.getCreature': # Not template
        include_parts  = (request.args.get('include_parts',  default='') == 'true')
        include_abilities = (request.args.get('include_abilities', default='') == 'true')
        include_creatures = (request.args.get('include_creatures', default='') == 'true')
        creature_id = int(request.args.get('id', default='0'))
        callback = request.args.get('callback', default='') #spgetcreaturecallback

        return jsonResponseWithObject({})

    if method == 'api.config.getConfigs':
        include_patches  = (request.args.get('include_patches',  default='') == 'true')
        include_settings = (request.args.get('include_settings', default='') == 'true')

        root = xml_tree.Element("response")
        setXmlValues(root, serverApi.bootstrapApi_response_object(version))

        configs = xml_tree.SubElement(root, "configs")
        config  = xml_tree.SubElement(configs, "config")
        setXmlValues(config, serverApi.bootstrapApi_getConfigs_object())

        if include_settings:
            settings = xml_tree.SubElement(root, "settings") # --CONFIRMED--
            xml_tree.SubElement(settings, "open").text = 'false'
            xml_tree.SubElement(settings, "telemetry-rate").text = '256'
            xml_tree.SubElement(settings, "telemetry-setting").text = '0' # --NUMBER--

        if include_patches:
            xml_tree.SubElement(root, "patches")

        return xmlResponseWithXmlElement(root)

    print " "
    print "http://" + request.host + "/bootstrap/api"
    print request.args
    print " "

    return jsonResponseWithObject({})

@app.route("/web/sporelabs/alerts", methods=['GET','POST'])
def webSporeLabsAlerts():
    return ""

@app.route("/web/sporelabsgame/announceen", methods=['GET','POST'])
def webSporeLabsGameAnnounceen():
    return "Can you see me"

@app.route("/survey/api", methods=['GET','POST'])
def surveyApi():
    method = request.args.get('method', default='')

    if method == "api.survey.getSurveyList":
        root = xml_tree.Element("response")
        setXmlValues(root, serverApi.bootstrapApi_response_object(server.gameVersion))

        surveys = xml_tree.SubElement(root, "surveys")
        survey  = xml_tree.SubElement(surveys, "survey")
        #xml_tree.SubElement(survey, "").text = ""

        return xmlResponseWithXmlElement(root)

    return ""

@app.route("/game/service/png", methods=['GET','POST'])
def gameServicePng():
    account_id = request.args.get('account_id')
    if account_id != None:
        print("Should return user account PNG")
        return ""

    creature_id = request.args.get('creature_id')
    if creature_id != None:
        print("Should return creature PNG")
        return ""

    template_id = request.args.get('template_id')
    if template_id != None:
        size = request.args.get('size')
        print("Should return creature template PNG")
        return ""

    ability_id = request.args.get('ability_id')
    if ability_id != None:
        print("Should return ability PNG")
        return ""

    rigblock_id = request.args.get('rigblock_id')
    if rigblock_id != None:
        # Rig blocks are:
        # Eyes, noses, mouths, hands, feet, tails, wings, armor,
        # weapons, jewelry and all types of assets used in character creation.
        print("Should return creature part PNG")
        return ""

    print " "
    print "http://" + request.host + "/game/service/png"
    print request.args
    print " "

    return ""

@app.route("/")
def index():
    indexPath = serverConfig.darksporeIndexPagePath()
    return send_from_directory(os.path.join(serverConfig.storagePath(), 'www'), indexPath, mimetype='text/html')

@app.route("/bootstrap/launcher/notes")
def bootstrapLauncherNotes():
    notesPath = serverConfig.darksporeLauncherNotesPath()
    file = open(os.path.join(os.path.join(serverConfig.storagePath(), 'www'), notesPath), "r")
    launcherNotesHtml = file.read()

    launcherNotesHtml = launcherNotesHtml.replace("{{dls-version}}", server.version)
    if serverConfig.versionLockEnabled():
        launcherNotesHtml = launcherNotesHtml.replace("{{game-version}}", server.gameVersion)
    else:
        launcherNotesHtml = launcherNotesHtml.replace("{{game-version}}", "available")

    return Response(launcherNotesHtml, mimetype='text/html')

@app.route('/favicon.ico')
def favicon():
    return send_from_directory(staticFolderPath, 'favicon.ico', mimetype='image/vnd.microsoft.icon')

@app.route('/images/<file_name>.jpg')
def steamDemoImages(file_name):
    return send_from_directory(os.path.join(staticFolderPath,'images'), file_name + '.jpg', mimetype='image/jpeg')

@app.route('/launcher/<file_name>.html')
def steamDemoLinks(file_name):
    return send_from_directory(os.path.join(staticFolderPath,'launcher'), file_name + '.html', mimetype='text/html')

@app.route('/', defaults={'path': ''})
@app.route('/<path:path>', methods=['GET', 'POST'])
def otherRequests(path):
    print request.host
    print request.args
    print 'You want path: %s' % path
    return ""

if __name__ == "__main__":
    blaze = Thread(target=startBlazeConnection,args=[42127,handleBlazeConnection])
    blaze.start()
    app.run(host='0.0.0.0', port=80)
