from blaze.Init import *

from models.account import *
from models.server import *
from controllers.gameApi import *
from controllers.config import *
from utils.response import *
from utils.launcher import *
from utils.path import *

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

serverConfig = DarkSporeServerConfig()
server = DarkSporeServer(serverConfig)
serverApi = DarkSporeServerApi(server)

mime = magic.Magic(mime=True)
staticFolderPath = pathJoin(pathJoin(serverConfig.storagePath(), 'www'), 'static')
app = Flask(__name__, static_url_path='/static', static_folder=staticFolderPath)

debugMode = ("debug" in sys.argv)
if debugMode:
    now = datetime.datetime.now()
    logFileName = 'dls-' + now.strftime("%Y-%m-%d_%H-%M-%S") + '.log'
    handler = logging.FileHandler(pathJoin(pathJoin(serverConfig.storagePath(), "logs"), logFileName))  # errors logged to this file
    handler.setLevel(logging.ERROR)  # only log errors and above
    app.logger.addHandler(handler)

@app.route("/dls/api", methods=['GET','POST'])
def dlsApi():
    method = request.args.get('method', default='')

    if method == 'api.launcher.setTheme':
        theme = request.args.get('theme', default='')
        server.setActiveTheme(theme)
        return jsonResponseWithObject({'stat': 'ok'})

    if method == 'api.launcher.listThemes':
        selectedTheme = server.getActiveTheme()
        themesList = server.availableThemes()
        return jsonResponseWithObject({'stat': 'ok', 'themes': themesList, 'selectedTheme': selectedTheme})

    return Response(status=500)

@app.route("/api", methods=['GET','POST'])
def api():
    method      = request.args.get('method',   default='')
    version     = request.args.get('version',  default='')
    callback    = request.args.get('callback', default='')
    format_type = request.args.get('format',   default='')

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
    version = request.args.get('version', default='')
    build   = request.args.get('build',   default='')
    method  = request.args.get('method',  default='')

    # Different players must be using the same version of the game, otherwise
    # there may be issue during a match.
    validVersion = server.setGameVersion(build)
    if serverConfig.versionLockEnabled() and validVersion == False:
        return xmlResponseWithObject(serverApi.bootstrapApi_error_object())

    if method == 'api.status.getStatus':
        include_broadcasts = request.args.get('include_broadcasts', default='')
        return xmlResponseWithObject(serverApi.gameApi_getStatus_object(include_broadcasts))

    print " "
    print "http://" + request.host + "/api"
    print request.args
    print " "

    return xmlResponseWithObject({})

@app.route("/bootstrap/api", methods=['GET','POST'])
def bootstrapApi():
    version = request.args.get('version', default='')
    build   = request.args.get('build',   default='')
    method  = request.args.get('method',  default='')

    # Different players must be using the same version of the game, otherwise
    # there may be issue during a match.
    validVersion = server.setGameVersion(build)
    if serverConfig.versionLockEnabled() and validVersion == False:
        return xmlResponseWithObject(serverApi.bootstrapApi_error_object())

    if method == 'api.account.getAccount':
        include_feed      =    (request.args.get('include_feed',      default='') == 'true')
        include_decks     =    (request.args.get('include_decks',     default='') == 'true')
        include_creatures =    (request.args.get('include_creatures', default='') == 'true')
        player_id         = int(request.args.get('id',                default='0'))
        return xmlResponseWithObject(serverApi.bootstrapApi_getAccount_object(player_id, include_feed, include_decks, include_creatures))

    if method == 'api.account.searchAccounts':
        count = int(request.args.get('count', default='0'))
        terms =     request.args.get('terms', default='')
        return xmlResponseWithObject(serverApi.bootstrapApi_searchAccounts_object(count, terms))

    if method == 'api.config.getConfigs':
        include_settings = (request.args.get('include_settings', default='') == 'true')
        include_patches  = (request.args.get('include_patches',  default='') == 'true')
        return xmlResponseWithObject(serverApi.bootstrapApi_getConfigs_object(build, include_settings, include_patches))

    if method == 'api.creature.getCreature':
        creature_id       = int(request.args.get('id',                default='0'))
        include_parts     =    (request.args.get('include_parts',     default='') == 'true')
        include_abilities =    (request.args.get('include_abilities', default='') == 'true')
        return xmlResponseWithObject(serverApi.bootstrapApi_getCreature_object(creature_id, include_parts, include_abilities))

    if method == 'api.creature.getTemplate':
        creature_id       = int(request.args.get('id',                default='0'))
        include_abilities    = (request.args.get('include_abilities', default='') == 'true')
        return xmlResponseWithObject(serverApi.bootstrapApi_getCreatureTemplate_object(creature_id, include_abilities))

    if method == 'api.friend.getList':
        start = int(request.args.get('start', default='0'))
        sort  =     request.args.get('sort',  default='') # eg. 'name'
        list  =     request.args.get('list',  default='') # eg. 'following'
        return xmlResponseWithObject(serverApi.bootstrapApi_getFriendsList_object(start, sort, list))

    if method == 'api.friend.follow':
        name = request.args.get('name', default='')
        return xmlResponseWithObject(serverApi.bootstrapApi_followFriend_object(name))

    if method == 'api.friend.unfollow':
        name = request.args.get('name', default='')
        return xmlResponseWithObject(serverApi.bootstrapApi_unfollowFriend_object(name))

    if method == 'api.friend.block':
        name = request.args.get('name', default='')
        return xmlResponseWithObject(serverApi.bootstrapApi_blockFriend_object(name))

    if method == 'api.friend.unblock':
        name = request.args.get('name', default='')
        return xmlResponseWithObject(serverApi.bootstrapApi_unblockFriend_object(name))

    if method == 'api.leaderboard.getLeaderboard':
        name    =     request.args.get('name',    default='') # eg. 'xp' / 'pvp_wins' / 'team'
        varient =     request.args.get('varient', default='') # eg. 'friends'
        count   = int(request.args.get('count',   default='0'))
        start   = int(request.args.get('start',   default='0'))
        return xmlResponseWithObject(serverApi.bootstrapApi_getLeaderboard_object(name, varient, count, start))

    print " "
    print "http://" + request.host + "/bootstrap/api"
    print request.args
    print " "

    return xmlResponseWithObject(serverApi.bootstrapApi_error_object())

@app.route("/web/sporelabs/alerts", methods=['GET','POST'])
def webSporeLabsAlerts():
    return ""

@app.route("/web/sporelabsgame/announceen", methods=['GET','POST'])
def webSporeLabsGameAnnounceen():
    return "<html><head><title>x</title></head><body>Announces</body></html>"

@app.route("/survey/api", methods=['GET','POST'])
def surveyApi():
    method = request.args.get('method', default='')

    if method == "api.survey.getSurveyList":
        return xmlResponseWithObject(serverApi.surveyApi_getSurveyList_object())

    return xmlResponseWithObject({})

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
    return send_from_directory(pathJoin(serverConfig.storagePath(), 'www'), indexPath, mimetype='text/html')

@app.route("/bootstrap/launcher/", methods=['GET','POST'])
def bootstrapLauncher():
    version = request.args.get('version', default='')

    if serverConfig.skipLauncher():
        launcherNotesHtml = launcher_directToGameHtml()
        return Response(launcherNotesHtml, mimetype='text/html')

    launcherPath = serverConfig.darksporeLauncherThemesPath()
    launcherPath = pathJoin(pathJoin(launcherPath, server.getActiveTheme()), "index.html")

    file = open(pathJoin(pathJoin(serverConfig.storagePath(), 'www'), launcherPath), "r")
    launcherHtml = file.read()

    dlsClientScript = launcher_dlsClientScript()
    launcherHtml = launcherHtml.replace('</head>', dlsClientScript + '</head>')

    return Response(launcherHtml, mimetype='text/html')

@app.route("/bootstrap/launcher/notes")
def bootstrapLauncherNotes():
    notesPath = serverConfig.darksporeLauncherNotesPath()
    file = open(pathJoin(pathJoin(serverConfig.storagePath(), 'www'), notesPath), "r")
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
    launcherFolder = pathJoin(notesPath, server.getActiveTheme())
    resourcePath = pathJoin(launcherFolder, path)

    filePath = pathJoin(pathJoin(serverConfig.storagePath(), 'www'), resourcePath)
    return send_file(filePath, mimetype=mime.from_file(filePath))

@app.route('/favicon.ico')
def favicon():
    return send_from_directory(staticFolderPath, 'favicon.ico', mimetype='image/vnd.microsoft.icon')

@app.route('/images/<file_name>.jpg')
def steamDemoImages(file_name):
    return send_from_directory(pathJoin(staticFolderPath,'images'), file_name + '.jpg', mimetype='image/jpeg')

@app.route('/launcher/<file_name>.html')
def steamDemoLinks(file_name):
    return send_from_directory(pathJoin(staticFolderPath,'launcher'), file_name + '.html', mimetype='text/html')

@app.route('/', defaults={'path': ''})
@app.route('/<path:path>', methods=['GET', 'POST'])
def otherRequests(path):
    print request.host
    print request.args
    print 'You want path: %s' % path
    return ""
    return Response(status=404)

def startApi():
    runningInDocker = ("docker" in sys.argv)
    if runningInDocker or serverConfig.singlePlayerOnly() == False:
        app.run(debug=debugMode, host='0.0.0.0', port=80, threaded=True)
    else:
        app.run(debug=debugMode, port=80, threaded=True)

if __name__ == "__main__":
    if "blaze" in sys.argv:
        startBlaze()
    else:
        startApi()
