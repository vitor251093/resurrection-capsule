# Darkspore-Fakeserver
[WIP] A small local server to play Darkspore offline

## Introduction
The focus is creating a "fake server" in order to make Darkspore work again. The game has been dead for 2 years now (you can literally buy a new physical copy by 2.99£ in Amazon.com). Since the servers shutdown, the game disc became a useless piece of plastic. This project aims to create a localhost server, which is going to make Darkspore work like if it was the original server.

## Architecture
The project has been done by now using Python, Flask and a Docker. The reason for using a Docker is because I'm testing Darkspore from macOS using a Wineskin wrspper, and with a Docker we can do that without messing with the local environment. In the future we can use a different method, but for now that one makes retrieving the request's arguments easy, and is compatible with Linux, macOS and Windows 10. Running without the Docker is also possible, but you will need to install the server requirements in your machine.

## Requirements
In order to start using (or at this point, contributing) the fake server, you will need:
- A computer with Linux, MacOS or Windows 10;
- The Docker application installed;
- Darkspore installed (use Wine, or relatives, if you are in Linux/macOS).

## Progress
- [x] Redirect Darkspore requests to the localhost;
- [ ] Make Darkspore believe that the server is online;
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

## API (config.darkspore.com)
There is one single API that seems to be responsible by many of the game interactions with the server: `/bootstrap/api`. We need to handle each of those interactions separately, so the API will be split by method. The methods below were find in the game .EXE by opening it with Hex Fiend and searching for `api.`.

### api.test.null
__Description:__ ???

### api.survey.getSurveyList
__Description:__ ???

### api.config.getConfigs
__Description:__ (Uncertain) First request made by the app. Not sure of what it retrieves.

### api.account.auth
__Description:__ (Uncertain) Probably the method used to login with your EA games account. If so, we gonna need to use some Origin API to make sure that the user exists, and that it's using a valid license of the game, manually.

### api.account.getAccount
__Description:__ (Uncertain) Probably the method used after the prior one to retrieve the user account details. Maybe it's here that it checks if the user has Darkspore in his/her account or not.

### api.creature.getCreature
__Description:__ (Uncertain) Probably retrieves the details about said creature.

### api.creature.getCreaturePng
__Description:__ (Uncertain) Probably returns the data of a PNG with the said creature, since this is the way that the SPORE engine handles the saving of creatures.

### api.game.getGame
__Description:__ ???

### api.game.getRandomGame
__Description:__ ???

### api.inventory.getPartList
__Description:__ ???

### api.game.getReplay
__Description:__ ???

### api.inventory.purchasePart
__Description:__ ???

### api.account.setNewPlayerStats
__Description:__ ???

### api.account.setSettings
__Description:__ ???

### api.creature.unlockCreature
__Description:__ ???

### api.creature.updateCreature
__Description:__ ???

### api.deck.updateDecks
__Description:__ ???

### api.inventory.flairPart
__Description:__ ???

### api.inventory.updatePartStatus
__Description:__ ???

### api.account.unlock
__Description:__ ???

### api.inventory.vendorPart
__Description:__ ???


## APIs (dev.darkspore.ea.com)
Those others paths were found in the game EXE. While we can (almost) safely assume that those are other APIs, used by the SPORE Labs, which is a removed feature according to the game Wiki.
http://darksporegame.wikia.com/wiki/Removed_Elements

Considering that even the APIs are there, is it possible that the SPORE Labs were already partially implemented in-game? Maybe we can find out.
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

## APIs (dev-sn2.darkspore.ea.com)
We can see here the main API again. Those API may have been used during the game development, following the same paths of `config.darkspore.com`, so the developers could test and debug the game without using the production server. If that theory is correct, the other 4 APIs may have existed for the official server as well.

- /bugs/choosepath.php
- /survey/takeSurvey.php
- /survey/api
- /game/api
- /bootstrap/api
