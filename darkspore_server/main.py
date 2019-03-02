from models.account import *
from models.server import *
from controllers.gameApi import *
from controllers.config import *
from utils.response import *

import os
import sys
import getopt
import warnings
import time
import timeit
import datetime
import socket
import logging
import threading
from threading import Thread

from flask import Flask
from flask import request
from flask import render_template
from flask import Response
from flask import send_from_directory

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
        return jsonResponseWithObject(serverApi.gameApi_getStatus_object(include_broadcasts))

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

        return xmlResponseWithObject(serverApi.bootstrapApi_getAccount_object(player_id,include_feed,include_decks,include_creatures))

    if method == 'api.creature.getCreature': # Not template
        include_parts  = (request.args.get('include_parts',  default='') == 'true')
        include_abilities = (request.args.get('include_abilities', default='') == 'true')
        include_creatures = (request.args.get('include_creatures', default='') == 'true')
        creature_id = int(request.args.get('id', default='0'))
        callback = request.args.get('callback', default='') #spgetcreaturecallback

        return xmlResponseWithObject({})

    if method == 'api.config.getConfigs':
        include_settings = (request.args.get('include_settings', default='') == 'true')
        include_patches  = (request.args.get('include_patches',  default='') == 'true')

        return xmlResponseWithObject(serverApi.bootstrapApi_getConfigs_object(include_settings, include_patches))

    print " "
    print "http://" + request.host + "/bootstrap/api"
    print request.args
    print " "

    return xmlResponseWithObject({})

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
        return xmlResponseWithObject(serverApi.surveyApi_getSurveyList_object())

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
    app.run(host='0.0.0.0', port=80)
