
darksporeBuild_limitedEditionDvd = "5.3.0.15"
darksporeBuild_onlineInstaller   = "5.3.0.84"  # Released at 27/04/2011
darksporeBuild_steamDemo         = "5.3.0.103" # Released between 23/05/2011 and 14/06/2011
darksporeBuild_latestOfficial    = "5.3.0.127" # Released between 15/11/2011 and 30/11/2012

class DarkSporeServer(object):
    def __init__(self):
        self.accounts = {}
        self.accountsSequenceNext = 0
        self.gameVersion = None
        self.version = "0.1"

    def setGameVersion(self, gameVersion):
        if self.gameVersion == None:
            self.gameVersion = gameVersion
        else:
            if self.gameVersion != gameVersion:
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
