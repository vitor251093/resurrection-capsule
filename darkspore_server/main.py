import sys
import getopt
import warnings
import timeit
from flask import Flask
from flask import request
from flask import render_template
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

@app.route("/favicon.ico")
def favicon():
    return ""

@app.route("/api", methods=['GET','POST'])
def api():
    print request.args
    version     = request.args.get('version',  default='1')
    callback    = request.args.get('callback', default='')
    method      = request.args.get('method',   default='')
    format_type = request.args.get('format',   default='json')
    
    # server.run()
    
    return "{}"

@app.route("/bootstrap/api", methods=['GET','POST'])
def bootstrapApi():
    print request.args
    version = request.args.get('version', default='1')
    method  = request.args.get('method',  default='')
    build   = request.args.get('build',   default='')
    include_patches  = (request.args.get('include_patches',  default='true') == 'true')
    include_settings = (request.args.get('include_settings', default='true') == 'true')

    # server.run()
    
    return "{}"

@app.route("/bootstrap/launcher/notes")
def bootstrapLauncherNotes():
    return ""

@app.route('/', defaults={'path': ''})
@app.route('/<path:path>', methods=['GET', 'POST'])
def otherRequests(path):
    print request.args
    return 'You want path: %s' % path

if __name__ == "__main__":
    # Needed for Flask is the Python file is called directly
    app.run(host='0.0.0.0')
