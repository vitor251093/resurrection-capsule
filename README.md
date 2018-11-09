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

(need to add hosts list)

## API
WIP

