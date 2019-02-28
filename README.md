# Darkspore-LS
[WIP] A small local server to play Darkspore offline

![alt Darkspore launcher with LS](https://raw.githubusercontent.com/vitor251093/Darkspore-LS/master/readme-launcher.png)

Join our [Discord](https://discord.gg/sDxuRNv) to keep in touch with the latest updates and/or to help with the project.

## Introduction
The focus is creating a local server in order to make Darkspore work again. The game has been dead since 2016 (you can literally buy a new physical copy by 2.99£ in Amazon.com). Since the servers shutdown, the game discs became useless pieces of plastic. This project aims to create a localhost server, which is going to make Darkspore work like if it was the original server, but much faster and private.

Still, be aware: this project will respect every layer of DRM that is above the Darkspore application. If you bought the game in Steam, you will still need Steam to play it. If you bought the game in Origin, you will still need Origin to play it. And if you bought the game disc, you will still need the game disc in your reader to play it, and you will still need a legitimate serial to install it.

The only layer of DRM that _for now_ cannot be kept is the ingame DRM, which checks if you have the game in your Origin account after the game has already started. There are two reasons for that:
- Origin has no public API, so there is no way to check if the user is really logged in, nor there is a way to know if he/she really has the game in the library;
- Even if we managed to do it, it would be very simple for someone to simply fork the project and remove that. I guess the best that can be done is relying in the other DRMs, which will exist independently of the way that you bought the game.

## FAQ

### With that, will Darkspore work exactly like it did before?
No. There are two factors that most likely will change; a positive one and a negative one. The positive one is that there will be no server instability (for obvious reason); still, if someone uses that to create an open private server, it may experience similar instabilities, unless we fix them.

The negative one is that, at this moment, there is no sign of packet logs from the original server. That means that, most likely, the game procedures won't be exactly like the original Darkspore ones. That includes damage calculation, drop chance, spawn frequency, experience calculation, among other things.

### How long it will take to be finished?
I have no idea. The biggest issue at this moment is reaching the login screen fields, so any help is welcome.

## Actual state
This is the progress so far with each version of the game.

### Limited Edition DVD version (5.3.0.15)
- **Without mod:** game launches, but login screen gives lost network connecting error.
- **With mod:** game launches, but login screen gives lost network connection error.

### Online installer version (5.3.0.84)
- **Without mod:** no change;
- **With mod:** game launches, but login screen gives lost network connection error.

### Steam Beta version (5.3.0.103)
- **Without mod:** no change;
- **With mod:** game launches, but login screen gives lost network connection error.

## Architecture
The project has been done by now using Python, Flask and a Docker. The reason for using a Docker is because I'm testing Darkspore from macOS using a Wineskin wrapper, and with a Docker we can do that without messing with the local environment. In the future we can use a different method, but for now that one makes retrieving the request's arguments easy, and is compatible with Linux, macOS and Windows 10. Running without the Docker is also possible, but you will need to install the server requirements in your machine.

## Requirements
In order to start using (or at this point, contributing) the local server, you will need:

### Docker version
- A computer with Linux, MacOS or Windows 10;
- The Docker application installed;
- Darkspore installed (use Wine, or relatives, if you are in Linux/macOS);
- The mod from the `required_mod` folder inside your Darkspore Data folder (non-DVD versions only).

### Non-Docker version
- A computer with Linux, MacOS or Windows XP (or above);
- Python and Flask installed;
- Darkspore installed (use Wine, or relatives, if you are in Linux/macOS);
- The mod from the `required_mod` folder inside your Darkspore Data folder (non-DVD versions only).

## Progress
- [x] Redirect Darkspore requests to the localhost;
- [x] Make Darkspore believe that the server is online (Error code 102);
- [x] Make Darkspore open after the Play button has been pressed (Error 3001).
- [ ] Make the login screen appear properly (Network connection was lost / Error 73000).
- [ ] ?

## Server redirect
The Darkspore application makes requests to different domains. In order to use the local server, we need to redirect those requests to the localhost. For now, the method that we are going with is changing the machine `hosts` file to redirect those requests to the local IP. We are using `127.0.0.1` because it will make things easier when we try to support programs like Hamachi.

```
127.0.0.1 api.darkspore.com
127.0.0.1 config.darkspore.com
127.0.0.1 content.darkspore.com
127.0.0.1 beta.darkspore.ea.com
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
127.0.0.1 telemetry.maxis.com
127.0.0.1 gosredirector.ea.com
127.0.0.1 undefined.Home
```

## Reference images
- http://kaehlerplanet.com/darkspore/

- http://davoonline.com/sporemodder/rob55rod/DI9r_Ref/DarksporeRefs.html

## Future plans
- Creating a small EXE which can be used to use Darkspore.exe redirecting its requests to a specific host, in order to avoid modifying the hosts file and to allow the use of different Darkspore servers. It may be based in [tinyproxy](https://github.com/tinyproxy/tinyproxy) or [proxifier](http://proxifier.sourceforge.net).

## Special Thanks
- rob55rod (Discord)
- nonchip (Github)
- haradons (Discord)
