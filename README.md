# Darkspore-Reloaded
[WIP] A small local server to play Darkspore offline

## Introduction
The focus is creating a "fake server" in order to make Darkspore work again. The game has been dead since 2016 (you can literally buy a new physical copy by 2.99£ in Amazon.com). Since the servers shutdown, the game discs became useless pieces of plastic. This project aims to create a localhost server, which is going to make Darkspore work like if it was the original server, but much faster and private.

Still, be aware: this project will respect every layer of DRM that is above the Darkspore application. If you bought the game in Steam, you will still need Steam to play it. If you bought the game in Origin, you will still need Origin to play it. And if you bought the game disc, you will still need the game disc in your reader to play it, and you will still need a legitimate serial to install it.

The only layer of DRM that _for now_ cannot be kept is the ingame DRM, which checks if you have the game in your Origin account after the game has already started. There are two reasons for that:
- Origin has no public API, so there is no way to check if the user is really logged in, nor there is a way to know if he/she really has the game in the library;
- Even if we managed to do it, it would be very simple for someone to simply fork the project and remove that. I guess the best that can be done is relying in the other DRMs, which will exist independently of the way that you bought the game.

## Architecture
The project has been done by now using Python, Flask and a Docker. The reason for using a Docker is because I'm testing Darkspore from macOS using a Wineskin wrapper, and with a Docker we can do that without messing with the local environment. In the future we can use a different method, but for now that one makes retrieving the request's arguments easy, and is compatible with Linux, macOS and Windows 10. Running without the Docker is also possible, but you will need to install the server requirements in your machine.

## Requirements
In order to start using (or at this point, contributing) the fake server, you will need:
- A computer with Linux, MacOS or Windows 10;
- The Docker application installed;
- Darkspore installed (use Wine, or relatives, if you are in Linux/macOS).

## Progress
- [x] Redirect Darkspore requests to the localhost;
- [ ] Make Darkspore believe that the server is online (Error code 102);
- [ ] ?

## Server redirect
The Darkspore application makes requests to different domains. In order to use the fake server, we need to redirect those requests to the localhost. For now, the method that we are going with is changing the machine `hosts` file to redirect those requests to the local IP. We are using `0.0.0.0` because in that way we don't need a router (and also, the request responses come almost instantly that way), but if you are using a VM to run Darkspore, replace it with `127.0.0.1`.

```
0.0.0.0 config.darkspore.com
0.0.0.0 ea6.com.edgesuite.net
0.0.0.0 beta-sn2.darkspore.ea.com
0.0.0.0 dev-sn2.darkspore.ea.com
0.0.0.0 dev.darkspore.ea.com
```

## Main API
We need to handle each of the `method`s separately, so the API will be split by `method`. 

### api.test.null
__Description:__ ???

### api.status.getBroadcastList
__Description:__ ???

### api.status.getStatus
__Description:__ This method has a callback parameter. It's unknown if that same method may respond to different callbacks, but for now we only know `updateServerStatus(data)`. That callback is present when Darkspore is trying to check if the game servers are online or not. It's unknown which kind of response this callback expects.

### api.account.auth
__Description:__ (Uncertain) Probably the method used to login with your EA games account. Considering that there is no Origin public API, we are probably just going to make it return that the user is authenticated independently of the case, but still using different usernames to differ different players using the same machine.

### api.account.getAccount
__Description:__ (Uncertain) Probably the method used after the prior one to retrieve the user account details. Maybe it's here that it checks if the user has Darkspore in his/her account or not, among with some other info.

### api.account.logout
__Description:__ ???

### api.account.setNewPlayerStats
__Description:__ ???

### api.account.setSettings
__Description:__ ???

### api.account.unlock
__Description:__ ???

### api.config.getConfigs
__Description:__ (Uncertain) First request made by the app. Not sure of what it retrieves.

### api.creature.getCreature
__Description:__ (Uncertain) Probably retrieves the details about said creature.

### api.creature.getCreaturePng
__Description:__ (Uncertain) Probably returns the data of a PNG with the said creature, since this is the way that the SPORE engine handles the saving of creatures.

### api.creature.resetCreature
__Description:__ ???

### api.creature.unlockCreature
__Description:__ ???

### api.creature.updateCreature
__Description:__ ???

### api.deck.updateDecks
__Description:__ ???

### api.game.getGame
__Description:__ ???

### api.game.getRandomGame
__Description:__ ???

### api.game.getReplay
__Description:__ ???

### api.game.exitGame
__Description:__ ???

### api.inventory.getPartList
__Description:__ ???
__Reference pic:__ http://davoonline.com/sporemodder/rob55rod/DI9r_Ref/Inventory.png

### api.inventory.purchasePart
__Description:__ ???

### api.inventory.flairPart
__Description:__ ???

### api.inventory.updatePartStatus
__Description:__ ???

### api.inventory.vendorParts
__Description:__ ???

### api.inventory.vendorPart
__Description:__ ???

### api.survey.getSurveyList
__Description:__ (Uncertain) Probably related with the Demo surveys.


## SPORE Labs APIs
Those APIs may be a legacy part of the code, or maybe not. In any case, we have listed them here. 
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
We can see here the main API again. Those APIs may have been used during the game development, following the same paths of `config.darkspore.com`, so the developers could test and debug the game without using the production server. If that theory is correct, the other 4 APIs may have existed for the official server as well.

- /bugs/choosepath.php
- /survey/takeSurvey.php
- /survey/api
- /game/api
- /bootstrap/api

## Reference images
- http://kaehlerplanet.com/darkspore/

- http://davoonline.com/sporemodder/rob55rod/DI9r_Ref/DarksporeRefs.html