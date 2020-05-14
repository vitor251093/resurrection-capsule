![alt ReCap logo and title](https://raw.githubusercontent.com/vitor251093/resurrection-capsule/master/readme_head.png)

[WIP] ReCap for short. A small local server to play Darkspore offline

[![Build Status](https://travis-ci.com/vitor251093/resurrection-capsule.svg?branch=master)](https://travis-ci.com/vitor251093/resurrection-capsule)

Join our [Discord](https://discord.gg/btfTw62) to keep in touch with the latest updates and/or to help with the project.

Just want to test it in your computer? Check [HOW_I_RUN_IT.md](HOW_I_RUN_IT.md).

## Overall progress
- [ ] Making the game playable offline;
- [ ] Making the game playable on LAN;
- [ ] Creating a server-client mechanic so servers can be hosted;
- [ ] Fix the original game issues, including connectivity issues.

## _Making the game playable offline_ progress
- [x] Redirect Darkspore requests to the localhost;
- [x] Make Darkspore believe that the server is online (Error code 102);
- [x] Make Darkspore open after the Play button has been pressed (Error 3001).
- [x] Make the login screen appear properly (Network connection was lost / Error 73000).
- [-] Make it possible to unlock any of the creatures in the hangar.
- [-] Make it possible to unlock any of the parts for creatures.
- [ ] ?

## Introduction
The focus is creating a local server in order to make Darkspore work again. The game has been dead since 2016 (you can literally buy a new physical copy by 2.99£ in Amazon.com). Since the servers shutdown, the game discs became useless pieces of plastic. This project aims to create a localhost server, which is going to make Darkspore work like if it was the original server, but much faster and private.

Still, be aware: this project will respect every layer of DRM that is above the Darkspore application. If you bought the game in Steam, you will still need Steam to play it. If you bought the game in Origin, you will still need Origin to play it. And if you bought the game disc, you will still need your serial to install it.

The only layer of DRM that _for now_ cannot be kept is the ingame DRM, which checks if you have the game in your Origin account after the game has already started. There are two reasons for that:
- Origin has no public API, so there is no way to check if the user is really logged in, nor there is a way to know if he/she really has the game in the library;
- Even if we managed to do it, it would be very simple for someone to simply fork the project and remove that. I guess the best that can be done is relying in the serial DRM, which will exist independently of the way that you bought the game.

## FAQ

### With that, will Darkspore work exactly like it did before?
No. There are two factors that most likely will change; a positive one and a negative one. The positive one is that there will be no server instability (for obvious reason); still, if someone uses that to create an open private server, it may experience similar instabilities, unless we fix them.

The negative one is that, at this moment, there is no sign of packet logs from the original server. That means that, most likely, the game procedures won't be exactly like the original Darkspore ones. That includes damage calculation, drop chance, spawn frequency, experience calculation, among other things.

### How long it will take to be finished?
I have no idea. The biggest issue at this moment is reaching the login screen fields, so any help is welcome.

### Which programs do I need to modify the project?
- Visual Studio 2019 for the server and the launcher

## Actual state
We are only focusing in making it work with the latest version of the game (5.3.0.127), although it may work with older versions of it, like the DVD version (5.3.0.15) and the Steam Beta (5.3.0.103). Once we have worked through the launcher patching system, ReCap should be capable of updating any Darkspore version to 5.3.0.127, like it was officially done by EA back when the game servers were online.

At this moment (25/04/20): the game launches, and the hero editor can be used, with support to parts and details.

## Architecture
The project has been done by now using C++ with Visual Studio 2019 (originally in Python, Flask and a Docker). The reason for this is focusing in the best performance for the server. 

## Reference images
- http://kaehlerplanet.com/darkspore/

- http://davoonline.com/sporemodder/rob55rod/DI9r_Ref/DarksporeRefs.html

## Projects used has references
- [buchacho/BF4BlazeEmulator](https://github.com/buchacho/BF4BlazeEmulator)
- [pedromartins1/BlazeServer](https://github.com/pedromartins1/BlazeServer)
- [Cloudef/xiloader-wine](https://github.com/Cloudef/xiloader-wine)

## Special Thanks
- dalkon (Discord)
- Emd (Discord)
- haradons (Discord)
- nonchip (Github)
- Otomachi Una (Discord)
- rob55rod (Discord)
- Snek (Discord)
