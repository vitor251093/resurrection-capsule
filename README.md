![alt ReCap logo and title](https://raw.githubusercontent.com/vitor251093/resurrection-capsule/master/readme_head.png)

[WIP] ReCap for short. A small local server to play Darkspore offline

Join our [Discord](https://discord.gg/btfTw62) to keep in touch with the latest updates and/or to help with the project.

Just want to test it in your computer? Check [HOW_I_RUN_IT.md](HOW_I_RUN_IT.md).

## Introduction
The focus is creating a local server in order to make Darkspore work again. The game has been dead since 2016 (you can literally buy a new physical copy by 2.99£ in Amazon.com). Since the servers shutdown, the game discs became useless pieces of plastic. This project aims to create a localhost server, which is going to make Darkspore work like if it was the original server, but much faster and private.

## Actual state
We are only focusing in making it work with the latest version of the game (5.3.0.127) and the Steam Demo version (5.3.0.103). The DVD version (5.3.0.15) is known not to work properly yet, though. Once we develop a proper patching system, ReCap should be capable of updating any Darkspore version to 5.3.0.127, like it was officially done by EA back when the game servers were online.

At this moment (06/01/25): the game launches, and the hero editor can be used, with support to parts and details.

## Modules
- Resurrection Capsule Hub
- [Resurrection Capsule Launcher](https://github.com/vitor251093/recap_launcher)
- [Resurrection Capsule Server](https://github.com/vitor251093/recap_server)
- [Resurrection Capsule Web Panel](https://github.com/vitor251093/recap_panel)

## FAQ

### With that, will Darkspore work exactly like it did before?
No. There are two factors that most likely will change; a positive one and a negative one. The positive one is that there will be no server instability (for obvious reason); still, if someone uses that to create an open private server, it may experience similar instabilities, unless we fix them.

The negative one is that, at this moment, there is no sign of packet logs from the original server. That means that, most likely, the game procedures won't be exactly like the original Darkspore ones. That includes damage calculation, drop chance, spawn frequency, experience calculation, among other things.

### How long it will take to be finished?
I have no idea. The biggest issue at this moment is reaching the login screen fields, so any help is welcome.

### Which programs do I need to modify the project?
That depends on which module are we talking about. Check the page of the module you want to build.

## Reference images
- http://kaehlerplanet.com/darkspore/

- http://davoonline.com/sporemodder/rob55rod/DI9r_Ref/DarksporeRefs.html

## Projects used as references
- [buchacho/BF4BlazeEmulator](https://github.com/buchacho/BF4BlazeEmulator)
- [pedromartins1/BlazeServer](https://github.com/pedromartins1/BlazeServer)
- [Cloudef/xiloader-wine](https://github.com/Cloudef/xiloader-wine)

## Credits
 * dalkon, for migrating the project from Python to C++, and developing the majority of the ReCap server code from 2019 to 2022
 * Splitwirez, for being our main consultant regarding the way Darkspore used to work, our designer, and the developer of the Resurrection Capsule Hub
 * r0ptr, for fixing the CMake build process, allowing us to continue the project after a huge hiatus, and for helping with coding in general
 * All the members from our Discord server that helped testing and developing the server!

## Special Thanks
- Emd (Discord)
- haradons (Discord)
- nonchip (Github)
- Otomachi Una (Discord)
- Snek (Discord)

## How can I help?
We can't accept any form of income in the project, but if want to help Darkspore to be released one day without an internet requirement, endorse the Darkspore wish in GOG's wishlist. That's the best we can do:
- https://www.gog.com/wishlist/games/darkspore
