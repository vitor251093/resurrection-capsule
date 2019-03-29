import os
from utils import path as pathUtils

dls_version = "0.1"

darksporeBuild_limitedEditionDvd = "5.3.0.15"
darksporeBuild_onlineInstaller   = "5.3.0.84"  # Released at 27/04/2011
darksporeBuild_steamDemo         = "5.3.0.103" # Released between 23/05/2011 and 14/06/2011
darksporeBuild_latestOfficial    = "5.3.0.127" # Released between 15/11/2011 and 30/11/2012

class DarkSporeServerData(object):
    def __init__(self):
        self.accounts = {}
        self.accountsSequenceNext = 0
        self.activeTheme = "default"

class DarkSporeServer(object):
    def loadServerDataFromFile(self):
        newServerData = pathUtils.loadObjectFromFile(self.config.serverDataFilePath())
        if newServerData != None:
            self.data = newServerData

    def saveServerDataToFile(self):
        pathUtils.saveObjectToFile(self.config.serverDataFilePath(), self.data)

    def getAccount(self, id):
        return self.accounts[str(id)]

    def createAccount(self, email, name):
        id = self.accountsSequenceNext
        if self.accounts[str(id)] != None:
            return -1

        self.accounts[str(id)] = Account(id, email, name)
        self.accountsSequenceNext += 1

        self.saveServerDataToFile()
        return id

    def getActiveTheme(self):
        return self.data.activeTheme

    def setActiveTheme(self, activeTheme):
        self.data.activeTheme = activeTheme
        self.saveServerDataToFile()

    def availableThemes(self):
        themesFolder = self.config.darksporeLauncherThemesPath()
        themesFolder = pathUtils.join(pathUtils.join(self.config.storagePath(), 'www'), themesFolder)
        themesFolderContents = os.listdir(themesFolder)
        themesList = []
        for file in themesFolderContents:
            themeFolder = pathUtils.join(themesFolder,file)
            themeFolderIndex = pathUtils.join(themeFolder,"index.html")
            if os.path.isdir(themeFolder) and os.path.isfile(themeFolderIndex):
                themesList.append(file)
        return themesList

    def __init__(self, config):
        self.config = config
        self.version = dls_version
        self.gameVersion = None
        self.host = "127.0.0.1"
        self.data = DarkSporeServerData()
        self.loadServerDataFromFile()

    def setGameVersion(self, gameVersion):
        if self.gameVersion == None:
            self.gameVersion = gameVersion
        return self.gameVersion == gameVersion
