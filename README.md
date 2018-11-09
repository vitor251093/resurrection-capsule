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


