import sys
import getopt
import warnings
import time
import timeit
import json
import xml.etree.cElementTree as xml_tree
from xml.etree import ElementTree
from flask import Flask
from flask import request
from flask import render_template
from flask import Response
import logging

class DarkSporeServer(object):
    def __init__(self):
        print "Starting..."

    def getAccount(self, id):
        print "Retrieving user info..."
        return {
            "avatar_id": '1', # TODO
            "avatar_updated": '0', # Not sure of what is that
            "blaze_id": '1', # Not sure of what is that
            "chain_progression": '0', # Not sure of what is that
            "creature_rewards": '0', # Not sure of what is that
            "current_game_id": '1', # Not sure of what is that
            "current_playgroup_id": '0', # Not sure of what is that
            "default_deck_pve_id": '1', # Not sure of what is that
            "default_deck_pvp_id": None, # Not sure of what is that
            "dna": '0', # TODO
            "email": 'tom@mywebsite.com', # TODO
            "id": str(id),
            "level": '3', # TODO
            "name": 'somegamerdood', # TODO
            "new_player_inventory": '1', # Not sure of what is that
            "new_player_progress": '7000', # Not sure of what is that
            "nucleus_id": '1', # Not sure of what is that
            "tutorial_completed": 'Y', # TODO
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

app = Flask(__name__)
server = DarkSporeServer()

handler = logging.FileHandler('/darkspore_server/templates/app.log')  # errors logged to this file
handler.setLevel(logging.ERROR)  # only log errors and above
app.logger.addHandler(handler)

def jsonResponseWithObject(obj):
    json_data = json.dumps(obj)
    return Response(json_data, mimetype='application/json')

@app.route("/api", methods=['GET','POST'])
def api():
    method      = request.args.get('method',   default='')
    callback    = request.args.get('callback', default='')
    version     = request.args.get('version',  default='')
    format_type = request.args.get('format',   default='')

    if method == 'api.status.getStatus':
        if callback == 'updateServerStatus(data)' and version == '1' and format_type == 'json':
            javascript = ("var data = {status: {blaze: {health: 1}, gms: {health: 1}, nucleus: {health: 1}, game: {health: 1}}}; " +
                          "setPlayButtonActive(); " +
                          "setTimeout(function(){updateBottomleftProgressComment('Local server enabled');updateProgressBar(1);},100); " +
                          callback + ";")
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

    # api.account.getAccount progress: 0%
    if method == 'api.account.getAccount':
        include_feed  = (request.args.get('include_feed',  default='') == 'true')
        include_decks = (request.args.get('include_decks', default='') == 'true')
        include_creatures = (request.args.get('include_creatures', default='') == 'true')
        player_id = int(request.args.get('id', default='0'))

        root = xml_tree.Element("response")
        xml_tree.SubElement(root, "stat").text = 'ok'
        xml_tree.SubElement(root, "version").text = version
        xml_tree.SubElement(root, "timestamp").text = str(long(time.time()))
        xml_tree.SubElement(root, "exectime").text = '1' # Not sure of what is that

        account = xml_tree.SubElement(root, "account")
        accountInfo = server.getAccount(player_id)
        for key,val in accountInfo.items():
            if val is None:
                xml_tree.SubElement(account, key)
            else:
                xml_tree.SubElement(account, key).text = val

        # Still missing creatures, decks and feed

        tree_str = ElementTree.tostring(root, encoding='utf8', method='xml')
        return Response(tree_str, mimetype='text/xml')

    # api.config.getConfigs progress: 0%
    if method == 'api.config.getConfigs':
        include_patches  = (request.args.get('include_patches',  default='') == 'true')
        include_settings = (request.args.get('include_settings', default='') == 'true')

        root = xml_tree.Element("response")

        xml_tree.SubElement(root, "to_version").text = build
        xml_tree.SubElement(root, "from_version").text = build
        #xml_tree.SubElement(root, "date").text = ''
        #xml_tree.SubElement(root, "target").text = ''
        if include_patches:
            xml_tree.SubElement(root, "patches")
        #xml_tree.SubElement(root, "launcher_url").text = ''
        #xml_tree.SubElement(root, "launcher_action").text = ''
        settings = xml_tree.SubElement(root, "settings")
        xml_tree.SubElement(settings, "liferay_host").text = '127.0.0.1'
        xml_tree.SubElement(settings, "liferay_port").text = '80'
        xml_tree.SubElement(settings, "sporenet_host").text = '127.0.0.1'
        xml_tree.SubElement(settings, "sporenet_port").text = '80'
        xml_tree.SubElement(settings, "sporenet_db_name").text = 'darkspore'
        xml_tree.SubElement(settings, "sporenet_db_port").text = '80'
        xml_tree.SubElement(settings, "sporenet_db_host").text = '127.0.0.1'
        xml_tree.SubElement(settings, "blaze_env").text = ''
        xml_tree.SubElement(settings, "blaze_secure").text = ''
        xml_tree.SubElement(settings, "blaze_service_name").text = ''

        tree_str = ElementTree.tostring(root, encoding='utf8', method='xml')
        return Response(tree_str, mimetype='text/xml')

    return jsonResponseWithObject({})

@app.route("/bootstrap/launcher/notes")
def bootstrapLauncherNotes():
    return render_template('launcher_notes.html')

@app.route("/favicon.ico")
def favicon():
    return render_template('favicon.ico')

@app.route("/")
def index():
    return render_template('index.html')

@app.route('/', defaults={'path': ''})
@app.route('/<path:path>', methods=['GET', 'POST'])
def otherRequests(path):
    print request.host
    print request.args
    print 'You want path: %s' % path
    return ""

if __name__ == "__main__":
    # Needed by Flask if the Python file is called directly
    app.run(host='0.0.0.0')
