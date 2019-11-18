# Darkspore-LS API (outdated)
The Darkspore-LS API needs to support all the APIs that Darkspore used to rely on. Due to that, there are multiple APIs that need to be supported, and here we gonna cover each one of them.

## Main API
Those requests used to be made to `http://config.darkspore.com/bootstrap/api`. It always receives the GET parameter `method`, which basically defines which kind of information should be returned. We need to handle each of the `method`s separately, so that API will be split by `method`.

### api.test.null
__Progress:__ 0%

__Description:__ ???

### api.status.getBroadcastList
__Progress:__ 0%

__Description:__ ???

### api.account.auth
__Progress:__ 0%

__Description:__ (Uncertain) Probably the method used to login with your EA games account. Considering that there is no Origin public API, we are probably just going to make it return that the user is authenticated independently of the case, but still using different usernames to differ different players using the same machine.

### api.account.getAccount
__Progress:__ 3.5%

__Description:__ Method used to retrieve the user account details (DNA, XP, level, name, avatar, etc). It can also return the user feed, decks and creatures.

### api.account.logout
__Progress:__ 0%

__Description:__ ???

### api.account.setNewPlayerStats
__Progress:__ 0%

__Description:__ ???

### api.account.setSettings
__Progress:__ 0%

__Description:__ ???

### api.account.unlock
__Progress:__ 0%

__Description:__ ???

### api.config.getConfigs
__Progress:__ 100%

__Description:__ Configures how to the game should handle requests. It can also be used to completely modify the launcher.

### api.creature.getCreature
__Progress:__ 0%

__Description:__ (Uncertain) Probably retrieves the details about said creature.

### api.creature.getCreaturePng
__Progress:__ 0%

__Description:__ (Uncertain) Probably returns the data of a PNG with the said creature, since this is the way that the SPORE engine handles the saving of creatures.

### api.creature.resetCreature
__Progress:__ 0%

__Description:__ ???

### api.creature.unlockCreature
__Progress:__ 0%

__Description:__ ???

### api.creature.updateCreature
__Progress:__ 0%

__Description:__ ???

### api.deck.updateDecks
__Progress:__ 0%

__Description:__ ???

### api.game.getGame
__Progress:__ 0%

__Description:__ ???

### api.game.getRandomGame
__Progress:__ 0%

__Description:__ ???

### api.game.getReplay
__Progress:__ 0%

__Description:__ ???

### api.game.exitGame
__Progress:__ 0%

__Description:__ ???

### api.inventory.getPartList
__Progress:__ 0%

__Description:__ ???

__Reference pic:__ http://davoonline.com/sporemodder/rob55rod/DI9r_Ref/Inventory.png

### api.inventory.purchasePart
__Progress:__ 0%

__Description:__ ???

### api.inventory.flairPart
__Progress:__ 0%

__Description:__ ???

### api.inventory.updatePartStatus
__Progress:__ 0%

__Description:__ ???

### api.inventory.vendorParts
__Progress:__ 0%

__Description:__ ???

### api.inventory.vendorPart
__Progress:__ 0%

__Description:__ ???


## Surveys API
It's probably direct related with the Steam demo version, and can be found at `/survey/api`. It always receives the GET parameter `method`, which basically defines which kind of information should be returned. We need to handle each of the `method`s separately, so that API will be split by `method`.

### api.survey.getSurveyList
__Progress:__ 0%

__Description:__ (Uncertain) Probably list the Demo version surveys.


## Status API
That name is provisory, and is due to the fact that the only use found for that API until now is updating the server status in the Darkspore launcher. That API path is `/api` and `/game/api`. Just like the API above, it receives the GET parameter `method`, which in the case above defines which kind of information should be returned. Considering that, and the possibility that we can find other utilities for that API in the future, we gonna handle each of the `method`s separately, so that API will be split by `method` as well.

### api.status.getStatus
__Progress:__ 100%

__Description:__ This method checks if the game needed servers are online (`api` (added later), `blaze`, `gms`, `nucleus` and `game`). In the older versions it also received a callback parameter, with the value `updateServerStatus(data)`, but that's no longer the case.


## SPORE Labs APIs
Those APIs may be a legacy part of the code, or maybe not. In any case, those are the paths of its requests. The host is still unknown, but most likely in the hosts list, so it's irrelevant.
- `/web/sporelabsgame/announceen`
- `/web/sporelabsgame/creatureprofile`
- `/web/sporelabsgame/finish`
- `/web/sporelabsgame/friends`
- `/web/sporelabsgame/inventory`
- `/web/sporelabsgame/leaderboards`
- `/web/sporelabsgame/lobby`
- `/web/sporelabsgame/persona`
- `/web/sporelabsgame/profile`
- `/web/sporelabsgame/register`
- `/web/sporelabsgame/squad`
- `/web/sporelabsgame/store`
- `/web/sporelabsgame/wiki`
- `/web/sporelabs/alerts`
- `/web/sporelabs/home`
- `/web/sporelabs/resetpassword`
- `/web/sporelabs/stats`


## Other APIs
The use of those APIs is still unknown, although they can be found in the game EXE strings. Their hosts are also unknown.
- `/bugs/choosepath.php`
- `/survey/takeSurvey.php`
- `/game/service/png`
