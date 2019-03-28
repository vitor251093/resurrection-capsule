
def directToGameHtml():
    return ('<!DOCTYPE html PUBLIC "-//IETF//DTD HTML 2.0//EN">'
          + '<html><head><script type="text/javascript">'
          + 'window.onload = function(){ Client.playCurrentApp(); }'
          + '</script></head><body></body></html>')

def dlsClientScript():
    return ('\n<script>'
          + '\n    var DLSClient = {};'
          + '\n    DLSClient.getRequest = function(url, callback) {'
          + '\n        var xmlHttp = new XMLHttpRequest(); '
          + '\n        xmlHttp.onload = function() { '
          + '\n            callback(xmlHttp.responseText);'
          + '\n        }'
          + '\n        xmlHttp.open("GET", url, true); '
          + '\n        xmlHttp.send(null);'
          + '\n    };'
          + '\n    DLSClient.request = function(name, params, callback) {'
          + '\n        DLSClient.getRequest("http://config.darkspore.com/dls/api?method=" + name + (params === undefined ? "" : ("&" + params)), callback); '
          + '\n    };'
          + '\n</script>')
