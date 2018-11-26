# Darkspore-LS
[WIP] A small local server to play Darkspore offline

![alt Darkspore launcher with LS](https://raw.githubusercontent.com/vitor251093/Darkspore-LS/master/readme-launcher.png)

## Introduction
The focus is creating a local server in order to make Darkspore work again. The game has been dead since 2016 (you can literally buy a new physical copy by 2.99£ in Amazon.com). Since the servers shutdown, the game discs became useless pieces of plastic. This project aims to create a localhost server, which is going to make Darkspore work like if it was the original server, but much faster and private.

Still, be aware: this project will respect every layer of DRM that is above the Darkspore application. If you bought the game in Steam, you will still need Steam to play it. If you bought the game in Origin, you will still need Origin to play it. And if you bought the game disc, you will still need the game disc in your reader to play it, and you will still need a legitimate serial to install it.

The only layer of DRM that _for now_ cannot be kept is the ingame DRM, which checks if you have the game in your Origin account after the game has already started. There are two reasons for that:
- Origin has no public API, so there is no way to check if the user is really logged in, nor there is a way to know if he/she really has the game in the library;
- Even if we managed to do it, it would be very simple for someone to simply fork the project and remove that. I guess the best that can be done is relying in the other DRMs, which will exist independently of the way that you bought the game.

## Architecture
The project has been done by now using Python, Flask and a Docker. The reason for using a Docker is because I'm testing Darkspore from macOS using a Wineskin wrapper, and with a Docker we can do that without messing with the local environment. In the future we can use a different method, but for now that one makes retrieving the request's arguments easy, and is compatible with Linux, macOS and Windows 10. Running without the Docker is also possible, but you will need to install the server requirements in your machine.

## Requirements
In order to start using (or at this point, contributing) the local server, you will need:
- A computer with Linux, MacOS or Windows 10;
- The Docker application installed;
- Darkspore installed (use Wine, or relatives, if you are in Linux/macOS);
- The mod from the `required_mod` folder inside your Darkspore Data folder.

## Progress
- [x] Redirect Darkspore requests to the localhost;
- [x] Make Darkspore believe that the server is online (Error code 102);
- [ ] Make Darkspore open after the Play button has been pressed (Error 3001).
- [ ] ?

## Server redirect
The Darkspore application makes requests to different domains. In order to use the local server, we need to redirect those requests to the localhost. For now, the method that we are going with is changing the machine `hosts` file to redirect those requests to the local IP. We are using `127.0.0.1` because it will make things easier when when we try to support programs like Hamachi.

```
127.0.0.1 api.darkspore.com
127.0.0.1 config.darkspore.com
127.0.0.1 content.darkspore.com
127.0.0.1 beta-sn.darkspore.ea.com
127.0.0.1 beta-sn2.darkspore.ea.com
127.0.0.1 dev.darkspore.ea.com
127.0.0.1 dev-sn.darkspore.ea.com
127.0.0.1 dev-sn2.darkspore.ea.com
127.0.0.1 fail.spore.rws.ad.ea.com
127.0.0.1 ea6.com.edgesuite.net
127.0.0.1 darkspore.alpha.lockbox.ea.com
127.0.0.1 darkspore.com
127.0.0.1 www.darkspore.com
127.0.0.1 www.sporelabs.com
127.0.0.1 splabbetamydb1b.rspc-iad.ea.com
127.0.0.1 321917-prodmydb009.spore.rspc-iad.ea.com
```

## Main API
We need to handle each of the `method`s separately, so the API will be split by `method`.

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
__Progress:__ 0%

__Description:__ (Uncertain) Probably the method used after the prior one to retrieve the user account details. Maybe it's here that it checks if the user has Darkspore in his/her account or not, among with some other info.

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
