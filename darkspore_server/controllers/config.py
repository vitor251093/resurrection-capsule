import os
import sys

class DarkSporeServerConfig(object):

    def __init__(self):
        self.serverPath = "/darkspore_server"
        self.config = {
            "SKIP_LAUNCHER": False,
            "VERSION_LOCKED": False,
            "SINGLEPLAYER_ONLY": True,
            "STORAGE_PATH": ("/darkspore_server_storage" if ("docker" in sys.argv) else "../storage"),
            "DARKSPORE_INDEX_PAGE_PATH": "index.html",
            "DARKSPORE_LAUNCHER_NOTES_PATH": "bootstrap/launcher/notes.html",
            "DARKSPORE_LAUNCHER_PATH": "bootstrap/launcher/default/index.html"
        }

        try:
            scriptPath = os.path.abspath(os.path.dirname(__file__))
            serverPath = os.path.dirname(scriptPath)
            self.serverPath = serverPath

            configPath = os.path.join(serverPath, "config.txt")
            configFile = open(configPath,"r")
            lines = configFile.readlines()
            for line in lines:
                if len(line.strip()) > 0 and line.strip().startswith('#') == False:
                    l_comp = line.split('=', 1)
                    if len(l_comp) != 2:
                        raise SyntaxError('Invalid config line: ' + line)
                    key   = l_comp[0].strip()
                    value = l_comp[1].strip()
                    try:
                        value = int(value)
                    except ValueError:
                        if value == "false":
                            value = False
                        if value == "true":
                            value = True
                    self.config[key] = value
        except Exception as e:
            print('')
            print('Error while reading config file: ' + e)
            print('')

        print self.config

    def get(self,key):
        return self.config[key]

    def skipLauncher(self):
        return self.get("SKIP_LAUNCHER") == True

    def singlePlayerOnly(self):
        return self.get("SINGLEPLAYER_ONLY") == True

    def versionLockEnabled(self):
        return self.singlePlayerOnly() == False and self.get("VERSION_LOCKED") == True

    def darksporeIndexPagePath(self):
        return self.get("DARKSPORE_INDEX_PAGE_PATH")

    def darksporeLauncherNotesPath(self):
        return self.get("DARKSPORE_LAUNCHER_NOTES_PATH")

    def darksporeLauncherPath(self):
        return self.get("DARKSPORE_LAUNCHER_PATH")

    def storagePath(self):
        def pathJoin(comp1, comp2):
            if os.name == 'nt':
                while comp2.startswith('..'):
                    comp1 = os.path.dirname(comp1)
                    comp2 = comp2[3:]
                return os.path.join(comp1.replace("/","\\"), comp2.replace("/","\\"))
            return os.path.join(comp1.replace("\\","/"), comp2.replace("\\","/"))

        storagePath = self.get("STORAGE_PATH")
        if storagePath.startswith('.'):
            storagePath = pathJoin(self.serverPath,storagePath)

        while storagePath.endswith('/') or storagePath.endswith('\\'):
            storagePath = storagePath[:-1]
        return storagePath
