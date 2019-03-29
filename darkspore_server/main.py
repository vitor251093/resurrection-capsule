import os
import sys
import getopt
import warnings
import time
import timeit
import datetime
import magic
import socket
import logging
import threading
from threading import Thread
from flask import Flask
from flask import request
from flask import render_template
from flask import Response
from flask import send_from_directory
from flask import send_file

from blaze.Init import *

from models.account import *
from models.server import *
from controllers.gameApi import *
from controllers.config import *

from utils import request as requestUtils
from utils import response as responseUtils
from utils import launcher as launcherUtils
from utils import path as pathUtils

serverConfig = DarkSporeServerConfig()
server = DarkSporeServer(serverConfig)
serverApi = DarkSporeServerApi(server)

mime = magic.Magic(mime=True)
staticFolderPath = pathUtils.join(pathUtils.join(serverConfig.storagePath(), 'www'), 'static')
app = Flask(__name__, static_url_path='/static', static_folder=staticFolderPath)

debugMode = ("debug" in sys.argv)
if debugMode:
    now = datetime.datetime.now()
    logFileName = 'dls-' + now.strftime("%Y-%m-%d_%H-%M-%S") + '.log'
    handler = logging.FileHandler(pathUtils.join(pathUtils.join(serverConfig.storagePath(), "logs"), logFileName))  # errors logged to this file
    handler.setLevel(logging.ERROR)  # only log errors and above
    app.logger.addHandler(handler)

@app.route("/dls/api", methods=['GET','POST'])
def dlsApi():
    method = requestUtils.get(request,'method',str)

    if method == 'api.launcher.setTheme':
        theme = requestUtils.get(request,'theme',str)
        server.setActiveTheme(theme)
        return responseUtils.jsonResponseWithObject({'stat': 'ok'})

    if method == 'api.launcher.listThemes':
        selectedTheme = server.getActiveTheme()
        themesList = server.availableThemes()
        return responseUtils.jsonResponseWithObject({'stat': 'ok', 'themes': themesList, 'selectedTheme': selectedTheme})

    return Response(status=500)

@app.route("/api", methods=['GET','POST'])
def api():
    method      = requestUtils.get(request,'method',  str)
    version     = requestUtils.get(request,'version', str)
    callback    = requestUtils.get(request,'callback',str)
    format_type = requestUtils.get(request,'format',  str)

    if method == 'api.status.getStatus':
        if callback == 'updateServerStatus(data)' and version == '1' and format_type == 'json':
            javascript = serverApi.api_getStatus_javascript(callback)
            return Response(javascript, mimetype='application/javascript')

    print " "
    print "http://" + request.host + "/api"
    print request.args
    print " "

    return ""

@app.route("/game/api", methods=['GET','POST'])
def gameApi():
    version = requestUtils.get(request,'version',str)
    build   = requestUtils.get(request,'build',  str)
    method  = requestUtils.get(request,'method', str)

    # Different players must be using the same version of the game, otherwise
    # there may be issue during a match.
    validVersion = server.setGameVersion(build)
    if serverConfig.versionLockEnabled() and validVersion == False:
        return responseUtils.xmlResponseWithObject(serverApi.bootstrapApi_error_object())

    if method == 'api.status.getStatus':
        include_broadcasts = requestUtils.get(request,'include_broadcasts', str)
        return responseUtils.xmlResponseWithObject(serverApi.gameApi_getStatus_object(include_broadcasts))

    print " "
    print "http://" + request.host + "/api"
    print request.args
    print " "

    return responseUtils.xmlResponseWithObject({})

@app.route("/bootstrap/api", methods=['GET','POST'])
def bootstrapApi():
    version = requestUtils.get(request,'version',str)
    build   = requestUtils.get(request,'build',  str)
    method  = requestUtils.get(request,'method', str)

    # Different players must be using the same version of the game, otherwise
    # there may be issue during a match.
    validVersion = server.setGameVersion(build)
    if serverConfig.versionLockEnabled() and validVersion == False:
        return responseUtils.xmlResponseWithObject(serverApi.bootstrapApi_error_object())

    if method.startswith('api.account.'):
        if method == 'api.account.getAccount':
            include_feed      = requestUtils.get(request,'include_feed',      bool)
            include_decks     = requestUtils.get(request,'include_decks',     bool)
            include_creatures = requestUtils.get(request,'include_creatures', bool)
            player_id         = requestUtils.get(request,'id', int)
            return responseUtils.xmlResponseWithObject(serverApi.bootstrapApi_getAccount_object(player_id, include_feed, include_decks, include_creatures))

        if method == 'api.account.searchAccounts':
            count = requestUtils.get(request,'count', int)
            terms = requestUtils.get(request,'terms', str)
            return responseUtils.xmlResponseWithObject(serverApi.bootstrapApi_searchAccounts_object(count, terms))


    if method.startswith('api.config.'):
        if method == 'api.config.getConfigs':
            include_settings = requestUtils.get(request,'include_settings', bool)
            include_patches  = requestUtils.get(request,'include_patches',  bool)
            return responseUtils.xmlResponseWithObject(serverApi.bootstrapApi_getConfigs_object(build, include_settings, include_patches))


    if method.startswith('api.creature.'):
        if method == 'api.creature.getCreature':
            creature_id       = requestUtils.get(request,'id', int)
            include_parts     = requestUtils.get(request,'include_parts', bool)
            include_abilities = requestUtils.get(request,'include_abilities', bool)
            return responseUtils.xmlResponseWithObject(serverApi.bootstrapApi_getCreature_object(creature_id, include_parts, include_abilities))

        if method == 'api.creature.getTemplate':
            creature_id       = requestUtils.get(request,'id', int)
            include_abilities = requestUtils.get(request,'include_abilities', bool)
            return responseUtils.xmlResponseWithObject(serverApi.bootstrapApi_getCreatureTemplate_object(creature_id, include_abilities))


    if method.startswith('api.friend.'):
        if method == 'api.friend.getList':
            start     = requestUtils.get(request,'start',int)
            sort      = requestUtils.get(request,'sort', str) # eg. 'name'
            list_type = requestUtils.get(request,'list', str) # eg. 'following'
            return responseUtils.xmlResponseWithObject(serverApi.bootstrapApi_getFriendsList_object(start, sort, list))

        if method == 'api.friend.follow':
            name = requestUtils.get(request,'name',str)
            return responseUtils.xmlResponseWithObject(serverApi.bootstrapApi_followFriend_object(name))

        if method == 'api.friend.unfollow':
            name = requestUtils.get(request,'name',str)
            return responseUtils.xmlResponseWithObject(serverApi.bootstrapApi_unfollowFriend_object(name))

        if method == 'api.friend.block':
            name = requestUtils.get(request,'name',str)
            return responseUtils.xmlResponseWithObject(serverApi.bootstrapApi_blockFriend_object(name))

        if method == 'api.friend.unblock':
            name = requestUtils.get(request,'name',str)
            return responseUtils.xmlResponseWithObject(serverApi.bootstrapApi_unblockFriend_object(name))


    if method.startswith('api.leaderboard.'):
        if method == 'api.leaderboard.getLeaderboard':
            name    = requestUtils.get(request,'name',   str) # eg. 'xp' / 'pvp_wins' / 'team'
            varient = requestUtils.get(request,'varient',str) # eg. 'friends'
            count   = requestUtils.get(request,'count',  int)
            start   = requestUtils.get(request,'start',  int)
            return responseUtils.xmlResponseWithObject(serverApi.bootstrapApi_getLeaderboard_object(name, varient, count, start))

    print " "
    print "http://" + request.host + "/bootstrap/api"
    print request.args
    print " "

    return responseUtils.xmlResponseWithObject(serverApi.bootstrapApi_error_object())

@app.route("/web/sporelabs/alerts", methods=['GET','POST'])
def webSporeLabsAlerts():
    return ""

@app.route("/web/sporelabsgame/announceen", methods=['GET','POST'])
def webSporeLabsGameAnnounceen():
    return "<html><head><title>x</title></head><body>Announces</body></html>"

@app.route("/survey/api", methods=['GET','POST'])
def surveyApi():
    method = requestUtils.get(request,'method', str)

    if method == "api.survey.getSurveyList":
        return responseUtils.xmlResponseWithObject(serverApi.surveyApi_getSurveyList_object())

    return responseUtils.xmlResponseWithObject({})

@app.route("/game/service/png", methods=['GET','POST'])
def gameServicePng():
    account_id = requestUtils.get(request,'account_id', int)
    if account_id != None:
        print("Should return user account PNG")
        return ""

    creature_id = requestUtils.get(request,'creature_id', int)
    if creature_id != None:
        print("Should return creature PNG")
        return ""

    template_id = requestUtils.get(request,'template_id', int)
    if template_id != None:
        size = requestUtils.get(request,'size',int)
        print("Should return creature template PNG")
        return ""

    ability_id = requestUtils.get(request,'ability_id', int)
    if ability_id != None:
        print("Should return ability PNG")
        return ""

    rigblock_id = requestUtils.get(request,'rigblock_id', int)
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
    return send_from_directory(pathUtils.join(serverConfig.storagePath(), 'www'), indexPath, mimetype='text/html')

@app.route("/bootstrap/launcher/", methods=['GET','POST'])
def bootstrapLauncher():
    version = requestUtils.get(request,'version', str)

    if serverConfig.skipLauncher():
        launcherNotesHtml = launcherUtils.directToGameHtml()
        return Response(launcherNotesHtml, mimetype='text/html')

    launcherPath = serverConfig.darksporeLauncherThemesPath()
    launcherPath = pathUtils.join(pathUtils.join(launcherPath, server.getActiveTheme()), "index.html")

    file = open(pathUtils.join(pathUtils.join(serverConfig.storagePath(), 'www'), launcherPath), "r")
    launcherHtml = file.read()

    dlsClientScript = launcherUtils.dlsClientScript(server.config.host())
    launcherHtml = launcherHtml.replace('</head>', dlsClientScript + '</head>')

    return Response(launcherHtml, mimetype='text/html')

@app.route("/bootstrap/launcher/notes")
def bootstrapLauncherNotes():
    notesPath = serverConfig.darksporeLauncherNotesPath()
    file = open(pathUtils.join(pathUtils.join(serverConfig.storagePath(), 'www'), notesPath), "r")
    launcherNotesHtml = file.read()

    versionLocked = serverConfig.versionLockEnabled()
    singleplayer = serverConfig.singlePlayerOnly()
    isLatest = (server.gameVersion == darksporeBuild_latestOfficial)

    launcherNotesHtml = launcherNotesHtml.replace("{{dls-version}}", server.version)
    launcherNotesHtml = launcherNotesHtml.replace("{{version-lock}}", server.gameVersion if versionLocked else "no")
    launcherNotesHtml = launcherNotesHtml.replace("{{game-mode}}", "singleplayer" if singleplayer else "multiplayer")
    launcherNotesHtml = launcherNotesHtml.replace("{{display-latest-version}}", "block" if versionLocked else "none")
    launcherNotesHtml = launcherNotesHtml.replace("{{latest-version}}", "yes" if isLatest else "no")

    return Response(launcherNotesHtml, mimetype='text/html')

@app.route("/bootstrap/launcher/<path:path>")
def bootstrapLauncherImages(path):
    notesPath = serverConfig.darksporeLauncherThemesPath()
    launcherFolder = pathUtils.join(notesPath, server.getActiveTheme())
    resourcePath = pathUtils.join(launcherFolder, path)

    filePath = pathUtils.join(pathUtils.join(serverConfig.storagePath(), 'www'), resourcePath)
    return send_file(filePath, mimetype=mime.from_file(filePath))

@app.route('/favicon.ico')
def favicon():
    return send_from_directory(staticFolderPath, 'favicon.ico', mimetype='image/vnd.microsoft.icon')

@app.route('/images/<file_name>.jpg')
def steamDemoImages(file_name):
    return send_from_directory(pathUtils.join(staticFolderPath,'images'), file_name + '.jpg', mimetype='image/jpeg')

@app.route('/launcher/<file_name>.html')
def steamDemoLinks(file_name):
    return send_from_directory(pathUtils.join(staticFolderPath,'launcher'), file_name + '.html', mimetype='text/html')

@app.route('/', defaults={'path': ''})
@app.route('/<path:path>', methods=['GET', 'POST'])
def otherRequests(path):
    print request.host
    print request.args
    print 'You want path: %s' % path
    return ""
    return Response(status=404)

def startApi(debug):
    runInDebug = debugMode and debug
    runningInDocker = ("docker" in sys.argv)
    if runningInDocker or serverConfig.singlePlayerOnly() == False:
        app.run(debug=runInDebug, host='0.0.0.0', port=80, threaded=True)
    else:
        app.run(debug=runInDebug, port=80, threaded=True)

if __name__ == "__main__":
    if "blaze-only" in sys.argv:
        startBlaze()
        sys.exit()

    if "api-only" in sys.argv:
        startApi(True)
        sys.exit()

    apiThread = Thread(target=startApi, args=(False, ))
    apiThread.start()

    startBlaze()
