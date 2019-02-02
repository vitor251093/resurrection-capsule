
class DarkSporeServerApi(object):
    def __init__(self, serverConfig, server):
        self.serverConfig = serverConfig
        self.server = server

    def getStatus_javascript(self, callback):
        javascript = ("var data = {status: {blaze: {health: 1}, gms: {health: 1}, nucleus: {health: 1}, game: {health: 1}}}; " +
                        "setTimeout(function(){" +
                            "oncriticalerror = false; " +
                            "setPlayButtonActive(); " +
                            "updateBottomleftProgressComment('DARKSPORE-LS ENABLED');" +
                            "updateProgressBar(1);" +
                            "document.getElementById('Patch_Content_Frame').style.display = 'block'; " +
                            "document.getElementById('ERROR_MESSAGE').style.height = '0px'; " +
                        "},200); " +
                        callback + ";")

        if self.serverConfig.shouldSkipLauncher():
            javascript = ("var data = {status: {blaze: {health: 1}, gms: {health: 1}, nucleus: {health: 1}, game: {health: 1}}}; "
                            "clickPlayButton();" +
                            "var runNow = function(){" +
                                "oncriticalerror = false; " +
                                "setPlayButtonActive(); " +
                                "updateBottomleftProgressComment('DARKSPORE-LS ENABLED');" +
                                "updateProgressBar(1);" +
                                "document.getElementById('Patch_Content_Frame').style.display = 'block'; " +
                                "document.getElementById('ERROR_MESSAGE').style.height = '0px'; " +
                                "clickPlayButton();" +
                                "setTimeout(runNow,1); " +
                            "}; " +
                            "runNow(); " +
                            callback + ";")
        return javascript

    def getAccount_object(self, id):
        print "Retrieving user info..."
        account = self.server.getAccount(id)
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
