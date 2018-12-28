import os

class DarkSporeServerConfig(object):
    
    def __init__(self):
        self.serverPath = "/darkspore_server"
        self.config = {
            "SHOULD_SKIP_LAUNCHER": False,
            "STORAGE_PATH": "/darkspore_server_save"
        }

        try:
            scriptPath = os.path.abspath(os.path.dirname(__file__))
            serverPath = os.path.join(scriptPath, "..")
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

    def shouldSkipLauncher(self):
        return self.get("SHOULD_SKIP_LAUNCHER") == True

    def storagePath(self):
        storagePath = self.get("STORAGE_PATH")
        while storagePath.endswith('/') or storagePath.endswith('\\'):
            storagePath = storagePath[:-1]
        return storagePath

    def steamDemoVersion(self):
        return "5.3.0.103"
        
    def dvdVersion(self):
        return "5.3.0.15"

