from models.account import *
from controllers.config import *
import os
import sys
import getopt
import warnings
import time
import timeit
import datetime
import json
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
        self.version = None

    def setVersion(self, version):
        if self.version == None:
            self.version = version
        else:
            if self.version != version:
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

class DarkSporeServerApi(object):
    @staticmethod
    def getStatus_javascript(callback):
        javascript = ("var data = {status: {blaze: {health: 1}, gms: {health: 1}, nucleus: {health: 1}, game: {health: 1}}}; " +
                        "setTimeout(function(){" +
                            "oncriticalerror = false; " +
                            "setPlayButtonActive(); " +
                            "updateBottomleftProgressComment('Local server enabled');" +
                            "updateProgressBar(1);" +
                            "document.getElementById('Patch_Content_Frame').style.display = 'block'; " +
                            "document.getElementById('ERROR_MESSAGE').style.height = '0px'; " +
                        "},200); " +
                        callback + ";")
        if serverConfig.shouldSkipLauncher():
            javascript = ("var data = {status: {blaze: {health: 1}, gms: {health: 1}, nucleus: {health: 1}, game: {health: 1}}}; "
                            "clickPlayButton();" + 
                            "var runNow = function(){" +
                                "oncriticalerror = false; " +
                                "setPlayButtonActive(); " +
                                "updateBottomleftProgressComment('Local server enabled');" +
                                "updateProgressBar(1);" +
                                "document.getElementById('Patch_Content_Frame').style.display = 'block'; " +
                                "document.getElementById('ERROR_MESSAGE').style.height = '0px'; " +
                                "clickPlayButton();" + 
                                "setTimeout(runNow,1); " +
                            "}; " + 
                            "runNow(); " +
                            callback + ";")
        return javascript

    @staticmethod
    def getAccount_object(id):
        print "Retrieving user info..."
        account = server.getAccount(id)
        return {
            "avatar_id": '1', # TODO
            "avatar_updated": '0', # Not sure of what is that
            "blaze_id": str(account.id),
            "chain_progression": str(account.chainProgression()), 
            "creature_rewards": '0', # Not sure of what is that
            "current_game_id": '1', # Not sure of what is that
            "current_playgroup_id": '0', # Not sure of what is that
            "default_deck_pve_id": '1', # Not sure of what is that
            "default_deck_pvp_id": None, # Not sure of what is that
            "dna": '0', # TODO
            "email": account.email,
            "grant_online_access": '0', # Not sure of what is that
            "id": str(account.id),
            "level": str(account.level),
            "name": account.name,
            "new_player_inventory": '1', # Not sure of what is that
            "new_player_progress": '7000', # Not sure of what is that
            "nucleus_id": '1', # A different per-user ID (may be the same here?)
            "star_level": '0', # Not sure of what is that for, but it can't be 65536 or bigger
            "tutorial_completed": ('Y' if account.tutorialCompleted else 'N'),
            "unlock_catalysts": '0', # Not sure of what is that
            "unlock_diagonal_catalysts": '0', # Not sure of what is that
            "unlock_editor_flair_slots": '3', # Not sure of what is that
            "unlock_fuel_tanks": '2', # Not sure of what is that
            "unlock_inventory": '180', # Not sure of what is that
            "unlock_inventory_identify": '0', # Not sure of what is that
            "unlock_pve_decks": '1', # Not sure of what is that
            "unlock_pvp_decks": '0', # Not sure of what is that
            "unlock_stats": '0', # Not sure of what is that
            "xp": '201' # TODO
        }

server = DarkSporeServer()
serverConfig = DarkSporeServerConfig()
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

def jsonResponseWithObject(obj):
    json_data = json.dumps(obj)
    return Response(json_data, mimetype='application/json')

def xmlResponseWithXmlElement(xmlElement):
    tree_str = ElementTree.tostring(xmlElement, encoding='utf8', method='xml')
    return Response(tree_str, mimetype='text/xml')

@app.route("/api", methods=['GET','POST'])
def api():
    method      = request.args.get('method',   default='')
    callback    = request.args.get('callback', default='')
    version     = request.args.get('version',  default='')
    format_type = request.args.get('format',   default='')

    if method == 'api.status.getStatus':
        if callback == 'updateServerStatus(data)' and version == '1' and format_type == 'json':
            javascript = DarkSporeServerApi.getStatus_javascript(callback)
            return Response(javascript, mimetype='application/javascript')

    print " "
    print "http://" + request.host + "/api"
    print request.args
    print " "

    return jsonResponseWithObject({})

@app.route("/bootstrap/api", methods=['GET','POST'])
def bootstrapApi():
    print " "
    print "http://" + request.host + "/bootstrap/api"
    print request.args
    print " "

    method  = request.args.get('method',  default='')
    version = request.args.get('version', default='')
    build   = request.args.get('build',   default='')

    # Different players must be using the same version of the game, otherwise
    # there may be issue during a match.
    validVersion = server.setVersion(build)
    if serverConfig.versionLockEnabled() and validVersion == False:
        return jsonResponseWithObject({})

    # api.account.getAccount progress: 3.5%
    if method == 'api.account.getAccount':
        include_feed  = (request.args.get('include_feed',  default='') == 'true')
        include_decks = (request.args.get('include_decks', default='') == 'true')
        include_creatures = (request.args.get('include_creatures', default='') == 'true')
        player_id = int(request.args.get('id', default='0'))

        root = xml_tree.Element("response")
        xml_tree.SubElement(root, "stat").text = 'ok'
        xml_tree.SubElement(root, "version").text = version
        xml_tree.SubElement(root, "timestamp").text = str(long(time.time()))
        xml_tree.SubElement(root, "exectime").text = '1' # Not sure of what is that; amount of logged times?

        account = xml_tree.SubElement(root, "account")
        accountInfo = DarkSporeServerApi.getAccount_object(player_id)
        for key,val in accountInfo.items():
            if val is None:
                xml_tree.SubElement(account, key)
            else:
                xml_tree.SubElement(account, key).text = val

        # Still missing creatures, decks and feed
        if include_feed:
            print "include feed"
        if include_decks:
            print "include decks"
        if include_creatures:
            print "include creatures"

        return xmlResponseWithXmlElement(root)


    # api.config.getConfigs progress: 0%
    if method == 'api.config.getConfigs':
        include_patches  = (request.args.get('include_patches',  default='') == 'true')
        include_settings = (request.args.get('include_settings', default='') == 'true')

        root = xml_tree.Element("response")
        xml_tree.SubElement(root, "stat").text = 'ok'
        xml_tree.SubElement(root, "version").text = version
        xml_tree.SubElement(root, "timestamp").text = str(long(time.time()))
        xml_tree.SubElement(root, "exectime").text = '10' # Not sure of what is that

        configs = xml_tree.SubElement(root, "configs")
        config = xml_tree.SubElement(configs, "config")
        xml_tree.SubElement(config, "open").text = 'true'
        xml_tree.SubElement(config, "launcher_url").text = 'http://127.0.0.1/'
        xml_tree.SubElement(config, "launcher_action").text = 'x'
        xml_tree.SubElement(config, "liferay_host").text = '127.0.0.1'
        xml_tree.SubElement(config, "liferay_port").text = '57371' #LSDS1 / Local Server Darkspore 1
        xml_tree.SubElement(config, "sporenet_host").text = '127.0.0.1'
        xml_tree.SubElement(config, "sporenet_port").text = '57372' #LSDS2
        xml_tree.SubElement(config, "sporenet_db_name").text = 'darkspore'
        xml_tree.SubElement(config, "sporenet_db_port").text = '57373' #LSDS3
        xml_tree.SubElement(config, "sporenet_db_host").text = '127.0.0.1'
        xml_tree.SubElement(config, "http_secure").text = 'false'
        xml_tree.SubElement(config, "sporenet_cdn_host").text = '127.0.0.1'
        xml_tree.SubElement(config, "sporenet_cdn_port").text = '57373' #LSDS4
        xml_tree.SubElement(config, "blaze_env").text = 'production' # Reblaze to protect the servers?
        xml_tree.SubElement(config, "blaze_secure").text = 'low'
        xml_tree.SubElement(config, "blaze_service_name").text = 'darkspore'
        xml_tree.SubElement(config, "telemetry_rate").text = '1'
        xml_tree.SubElement(config, "telemetry_setting").text = '{}'

        if include_patches:
            xml_tree.SubElement(root, "patches")

        if include_settings:
            settings = xml_tree.SubElement(root, "settings")
            xml_tree.SubElement(settings, "active").text = 'true'
            xml_tree.SubElement(settings, "locale").text = 'en-US'
            xml_tree.SubElement(settings, "sessionId").text = '1'
            xml_tree.SubElement(settings, "token").text = 'abcdef0123456789'
            xml_tree.SubElement(settings, "count").text = '1'
            xml_tree.SubElement(settings, "round_id").text = '1'
            xml_tree.SubElement(settings, "reference_id").text = '1'
            xml_tree.SubElement(settings, "new_player_inventory").text = '1'
            xml_tree.SubElement(settings, "new_player_progress").text = '0'

        return xmlResponseWithXmlElement(root)

    return jsonResponseWithObject({})

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

    if serverConfig.versionLockEnabled():
        launcherNotesHtml = launcherNotesHtml.replace("{{version}}", server.version)
    else:
        launcherNotesHtml = launcherNotesHtml.replace("{{version}}", "available")

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
    # Needed by Flask if the Python file is called directly
    app.run(host='0.0.0.0', port=80)
