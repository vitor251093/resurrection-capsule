## How I run it on Windows?

1. Be sure that you have Darkspore installed (either the latest Darkspore version, or the Steam Demo version).

2. Update your Microsoft Visual C++ Redistributable (C and C++ Runtime libraries) to the latest version https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170

3. Download the zip with the latest version of ReCap from the Releases tab here in Github ([Windows](https://github.com/vitor251093/resurrection-capsule/releases/latest/download/resurrection-capsule-windows-x64.zip)).

4. After that, you will need to find the DarksporeBin folder. Depending of your Darkspore version, its folder will be different:
\- **DVD/Latest version:** C:\Program Files (x86)\Electronic Arts\Darkspore\DarksporeBin
\- **Steam version:** C:\Program Files (x86)\Steam\steamapps\common\Darkspore\DarksporeBin

5. Extract the contents of the zip of latest version of ReCap on the DarksporeBin folder.

6. Run the `RecapLauncher.exe` file, which is now inside the DarksporeBin folder. 

7. The launcher will launch the server and the game at the same time. Keep both running while playing.

8. On your first launch, the server will start extracting some files from the Darkspore Data folder. That's expected, and may take a little while. Just wait until the Singleplayer button is enabled on the Darkspore launcher.

9. Once it gets enabled, press Singleplayer in the Darkspore launcher and wait for the login screen.

10. Press the Register button in the Darkspore login screen, create a user account (it will only exist in your computer, and you won't need internet to do that), and then login with your new user.

## How I run it on Ubuntu?

1. Be sure that you have Darkspore installed under Wine/Proton (either the latest Darkspore version, or the Steam Demo version).
**Note:** We currently don't have an specific recommended Wine/Proton version

2. Download the zip with the latest version of ReCap from the Releases tab here in Github ([Ubuntu](https://github.com/vitor251093/resurrection-capsule/releases/latest/download/resurrection-capsule-ubuntu-x86_64.zip)).

3. After that, you will need to find the DarksporeBin folder. Depending of your Darkspore version, its folder will be different (replace C: with the drive of the Wine prefix where Darkspore was installed):
\- **DVD/Latest version:** C:\Program Files (x86)\Electronic Arts\Darkspore\DarksporeBin
\- **Steam version:** C:\Program Files (x86)\Steam\steamapps\common\Darkspore\DarksporeBin

4. Extract the contents of the zip of latest version of ReCap on the DarksporeBin folder. With that, you will have a file called `RecapLauncher.exe` inside the DarksporeBin folder. 

5. Run the binary called `recap_server` that is inside DarksporeBin/Server. Keep it running while playing the game.

6. Run the `RecapLauncher.exe` file using Wine/Proton. It will launch the game.

7. On your first launch, the server will start extracting some files from the Darkspore Data folder. That's expected, and may take a little while. Just wait until the Singleplayer button is enabled on the Darkspore launcher.

8. Once it gets enabled, press Singleplayer in the Darkspore launcher and wait for the login screen.

9. Press the Register button in the Darkspore login screen, create a user account (it will only exist in your computer, and you won't need internet to do that), and then login with your new user.

## IMPORTANT WARNING NOTE
Darkspore's communication protocols are old and outdated, so anything you do inside of it can be sniffed.

In other words: when creating a user to play, use a password like `a` or `password`, but **DON'T** use passwords that you are used to use online. The same applies to name and e-mail. **DON'T** use your real e-mail. There is no email validator at the moment, so you can use anything, like `a` . In summary: **DON'T** use any real private info within the game.

We gonna try to update the SSL protocols within the Darkspore game EXE on the long term, but that's a complicated thing to do since we don't have the original game source code. Until then, as long as you don't share any real private info with the game, you can play Darkspore with safety.
