# Darkspore-LS API
The Darkspore-LS API needs to support all the APIs that Darkspore used to rely on. Due to that, there are multiple APIs that need to be supported, and here we gonna cover each one of them.

## Main API
Those requests used to be made to `http://config.darkspore.com/bootstrap/api`. It always receives the GET parameter `method`, which basically defines which kind of information should be returned. We need to handle each of the `method`s separately, so that API will be split by `method`.

### api.test.null
__Progress:__ 0%

__Description:__ ???

### api.status.getBroadcastList
__Progress:__ 0%

__Description:__ ???

### api.status.getStatus
__Progress:__ 100%

__Description:__ This method checks if the game needed servers are online (`blaze`, `gms`, `nucleus` and `game`). It receives a callback parameter, `updateServerStatus(data)`. It's unknown if that same method may respond to different callbacks.

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
__Progress:__ 0%

__Description:__ (Uncertain) First request made by the app. Not sure of what it retrieves, but it seems to contain (among other things) the server host (`config.darkspore.com`), which seems kinda redundant.

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

### api.survey.getSurveyList
__Progress:__ 0%

__Description:__ (Uncertain) Probably related with the Demo surveys.


## SPORE Labs APIs
Those APIs may be a legacy part of the code, or maybe not. In any case, those are the paths of its requests. The host is still unknown, but most likely in the hosts list, so it's irrelevant.
- /web/sporelabsgame/creatureprofile
- /web/sporelabsgame/wiki
- /web/sporelabsgame/leaderboards
- /web/sporelabsgame/profile
- /web/sporelabsgame/lobby
- /web/sporelabsgame/friends
- /web/sporelabsgame/store
- /web/sporelabsgame/inventory
- /web/sporelabsgame/squad
- /web/sporelabs/home
- /web/sporelabs/stats
- /web/sporelabs/alerts
- /web/sporelabs/resetpassword
- /web/sporelabsgame/finish
- /web/sporelabsgame/persona
- /web/sporelabsgame/register

## Other APIs
The use of those APIs is still unknown, although they can be found in the game EXE strings. Their hosts is also unknown.
- /bugs/choosepath.php
- /survey/takeSurvey.php
- /survey/api
- /game/api
- /game/service/png
