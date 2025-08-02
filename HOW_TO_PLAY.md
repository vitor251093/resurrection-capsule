# How I run it?
In order to play the game locally you will need:

- A computer with Windows 7 (or above);
- Darkspore installed.

Download the zip with the latest version of ReCap from the Releases tab here in Github ([Windows](https://github.com/vitor251093/resurrection-capsule/releases/latest/download/resurrection-capsule-windows-x64.zip) / [Ubuntu](https://github.com/vitor251093/resurrection-capsule/releases/latest/download/resurrection-capsule-ubuntu-x86_64.zip)).

After that, you will need to find the DarksporeBin folder. Depending of your Darkspore version, its folder will be different:
- **DVD/Latest version:** C:\Program Files (x86)\Electronic Arts\Darkspore\DarksporeBin
- **Steam version:** C:\Program Files (x86)\Steam\steamapps\common\Darkspore\DarksporeBin

Extract the contents of the zip of latest version of ReCap on the DarksporeBin folder. With that, you will have a file called `RecapLauncher.exe` inside the DarksporeBin folder. 

Run the `RecapLauncher.exe` file. It will launch the server and the game at the same time. Keep both running while playing.

On your first launch, the server will start extracting some files from the Darkspore Data folder. That's expected, and may take a little while. You will know that it finished when you see `Darkspore Data extraction completed successfully.` on the logs.

Press Singleplayer in the Darkspore launcher and wait for the login screen. Press the Register button in the Darkspore login screen, create a user account (it will only exist in your computer, and you won't need internet to do that), and then login with your new user.

## IMPORTANT WARNING NOTE
Darkspore's communication protocols are old and outdated, so anything you do inside of it can be sniffed.

In other words: when creating a user to play, use a password like `a` or `password`, but **DON'T** use passwords that you are used to use online. The same applies to name and e-mail. **DON'T** use your real e-mail. There is no email validator at the moment, so you can use anything, like `a` . In summary: **DON'T** use any real private info within the game.

We gonna try to update the SSL protocols within the Darkspore game EXE on the long term, but that's a complicated thing to do since we don't have the original game source code. Until then, as long as you don't share any real private info with the game, you can play Darkspore with safety.
