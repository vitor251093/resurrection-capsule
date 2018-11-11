import sys
import getopt
import warnings
import timeit
import json
from flask import Flask
from flask import request
from flask import render_template
from flask import Response
import logging

class DarkSporeServer(object):
    def __init__(self):
        print "Starting..."

    def run(self):
        print "Running"

app = Flask(__name__)
server = DarkSporeServer()

handler = logging.FileHandler('/darkspore_server/templates/app.log')  # errors logged to this file
handler.setLevel(logging.ERROR)  # only log errors and above
app.logger.addHandler(handler)

def jsonResponseWithObject(obj):
    json_data = json.dumps(obj)
    return Response(json_data, mimetype='application/json')

@app.route("/api", methods=['GET','POST'])
@app.route("/bootstrap/api", methods=['GET','POST'])
def api():
    print request.args
    version = request.args.get('version', default='1')
    callback = request.args.get('callback', default='')
    method  = request.args.get('method', default='')
    build   = request.args.get('build',  default='')
    format_type = request.args.get('format', default='json')
    include_patches  = (request.args.get('include_patches',  default='true') == 'true')
    include_settings = (request.args.get('include_settings', default='true') == 'true')

    if method == 'api.config.getConfigs':
        # First method called by the game on startup
        data = {}
        return jsonResponseWithObject(data)
    
    if method == 'api.status.getStatus':
        if callback == 'updateServerStatus(data)':
            data = {}
            return jsonResponseWithObject(data)

    return jsonResponseWithObject({})

@app.route("/bootstrap/launcher/notes")
def bootstrapLauncherNotes():
    # Still not working
    return "<html><body>Darkspore Reloaded</body></html>"

@app.route("/favicon.ico")
def favicon():
    return ""

@app.route('/', defaults={'path': ''})
@app.route('/<path:path>', methods=['GET', 'POST'])
def otherRequests(path):
    print request.args
    print 'You want path: %s' % path
    return ""

if __name__ == "__main__":
    # Needed by Flask if the Python file is called directly
    app.run(host='0.0.0.0')
