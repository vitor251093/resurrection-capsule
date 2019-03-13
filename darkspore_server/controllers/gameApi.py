import json
import time

class DarkSporeServerApi(object):
    def __init__(self, server):
        self.server = server
        self.exectimeInt = 0

    def timestamp(self):
        return long(time.time())

    def exectime(self):
        self.exectimeInt += 1
        return self.exectimeInt

    def objectByAddingResponseCommonKeys(self, obj):
        obj["stat"] = 'ok'
        obj["version"] = self.server.gameVersion
        obj["timestamp"] = self.timestamp()
        obj["exectime"] = self.exectime()
        return obj

    def bootstrapApi_error_object(self):
        obj = self.objectByAddingResponseCommonKeys({})
        obj["stat"] = 'error'
        return obj

    # Not used by the latest version of the game
    # TODO: Check if this can be removed
    def api_getStatus_javascript(self, callback):
        data = json.dumps(self.gameApi_getStatus_object(True))
        javascript = ("var data = " + data + "; " +
                      "oncriticalerror = false; " +
                      "setPlayButtonActive(); " +
                      "updateProgressBar(1);" +
                      callback + ";")
        return javascript

    def gameApi_getStatus_object(self, include_broadcasts):
        obj = {
                "status": {
                    "api": {
                        "health": 1,
                        "revision": 1,
                        "version": 1
                    },
                    "blaze":   {"health": 1},
                    "gms":     {"health": 1},
                    "nucleus": {"health": 1},
                    "game": {
                        "health": 1,
                        "countdown": 0,
                        "open": 1,
                        "throttle": 0,
                        "vip": 0
                    }
                }
              }

        if include_broadcasts:
            obj["broadcasts"] = []

        return self.objectByAddingResponseCommonKeys(obj)

    def bootstrapApi_getAccount_object(self, id, include_feed, include_decks, include_creatures):
        account = self.server.getAccount(id)
        obj = {
            "account": {
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

                # catalysts are things you collect in game and make you stronger, and max amount of them is 9
                "unlock_catalysts": '0', # TODO
                "unlock_diagonal_catalysts": '0', # TODO

                # Editor flair is probably amount of "detail"  slots you can put on creature.
                "unlock_editor_flair_slots": '3', # TODO

                # Fuel might be: higher threat level unlock or amount of levels you can repeat after each one
                "unlock_fuel_tanks": '2', # TODO

                # Inventory is probably the amount of parts player can store.
                "unlock_inventory": '180', # TODO
                "unlock_inventory_identify": '0', # TODO

                "unlock_pve_decks": '1', # Not sure of what is that
                "unlock_pvp_decks": '0', # Not sure of what is that
                "unlock_stats": '0', # Not sure of what is that
                "xp": '201' # TODO
            }
        }

        # TODO: Still missing creatures, decks and feed
        if include_feed:
            print "include feed"

        if include_decks:
            print "include decks"

        if include_creatures:
            print "include creatures"

        return self.objectByAddingResponseCommonKeys(obj)

    def bootstrapApi_searchAccounts_object(self, count, terms):
        obj = {
            "configs": [{
                # TODO:
            }]
        }

        return self.objectByAddingResponseCommonKeys(obj)

    def bootstrapApi_getConfigs_object(self, build, include_settings, include_patches):
        obj = {
            "configs": [{
                "config": {
                    "blaze_service_name": 'darkspore',
                    "blaze_secure": 'N',
                    "blaze_env": 'production',
                    "sporenet_cdn_host": 'darkspore.com',
                    "sporenet_db_host": 'darkspore.com',
                    "sporenet_db_port": '80',
                    "sporenet_db_name": 'darkspore',
                    "sporenet_host": 'darkspore.com',
                    "sporenet_port": '80',
                    'http_secure': 'N',
                    "liferay_host": 'darkspore.com',
                    "liferay_port": '80',
                    "launcher_action": '2',
                    "launcher_url": ('http://darkspore.com/bootstrap/launcher/?version=' + build)
                }
            }],
            "to_image": "",
            "from_image": ""
        }

        if include_settings:
            obj["settings"] = {
                "open": 'false',
                "telemetry-rate": '256',
                "telemetry-setting": '0'
            }

        if include_patches:
            obj["patches"] = []
            # target, date, from_version, to_version, id, description, application_instructions,
            # locale, shipping, file_url, archive_size, uncompressed_size,
            # hashes (attributes, Version, Hash, Size, BlockSize)

        obj = self.objectByAddingResponseCommonKeys(obj)
        obj["version"] = build
        return obj

    def bootstrapApi_getCreature_object(self, creature_id, include_parts, include_abilities):
        obj = {
            "configs": [{
                # TODO:
            }]
        }

        if include_parts:
            print "include_parts"

        if include_abilities:
            print "include_abilities"

        return self.objectByAddingResponseCommonKeys(obj)

    def bootstrapApi_getCreatureTemplate_object(self, creature_id, include_abilities):
        obj = {
            "configs": [{
                # TODO:
            }]
        }

        if include_abilities:
            print "include_abilities"

        return self.objectByAddingResponseCommonKeys(obj)

    def bootstrapApi_getFriendsList_object(self, start, sort, list):
        obj = {
            "configs": [{
                # TODO:
            }]
        }

        return self.objectByAddingResponseCommonKeys(obj)

    def bootstrapApi_followFriend_object(self, name):
        obj = {
            "configs": [{
                # TODO:
            }]
        }

        return self.objectByAddingResponseCommonKeys(obj)

    def bootstrapApi_unfollowFriend_object(self, name):
        obj = {
            "configs": [{
                # TODO:
            }]
        }

        return self.objectByAddingResponseCommonKeys(obj)

    def bootstrapApi_blockFriend_object(self, name):
        obj = {
            "configs": [{
                # TODO:
            }]
        }

        return self.objectByAddingResponseCommonKeys(obj)

    def bootstrapApi_unblockFriend_object(self, name):
        obj = {
            "configs": [{
                # TODO:
            }]
        }

        return self.objectByAddingResponseCommonKeys(obj)

    def bootstrapApi_getLeaderboard_object(self, name, varient, count, start):
        obj = {
            "leaderboard": [{
                # TODO:
            }]
        }

        return self.objectByAddingResponseCommonKeys(obj)

    def surveyApi_getSurveyList_object(self):
        obj = {
            "surveys": [{
                "survey": {
                    # TODO:
                    "id": str(1),
                    "trigger1": "",
                    "trigger2": ""
                }
            }]
        }
        return self.objectByAddingResponseCommonKeys(obj)
