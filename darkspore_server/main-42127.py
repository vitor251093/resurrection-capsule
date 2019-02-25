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
import xml.etree.cElementTree as xml_tree
from xml.etree import ElementTree

from flask import Flask
from flask import request
from flask import render_template
from flask import Response
from flask import send_from_directory

import logging

serverConfig = DarkSporeServerConfig()
staticFolderPath = os.path.join(os.path.join(serverConfig.storagePath(), 'www'), 'static')
app = Flask(__name__, static_url_path='/static', static_folder=staticFolderPath)

now = datetime.datetime.now()
logFileName = 'dls-42127-' + now.strftime("%Y-%m-%d_%H-%M-%S") + '.log'
handler = logging.FileHandler(os.path.join(serverConfig.storagePath(), logFileName))  # errors logged to this file
handler.setLevel(logging.ERROR)  # only log errors and above
app.logger.addHandler(handler)

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

@app.route('/', defaults={'path': ''})
@app.route('/<path:path>', methods=['GET', 'POST'])
def otherRequests(path):
    print request.host
    print request.args
    print 'You want path: %s' % path
    return ""

if __name__ == "__main__":
    # Needed by Flask if the Python file is called directly
    app.run(host='0.0.0.0', ssl_context='adhoc', port=42127)
