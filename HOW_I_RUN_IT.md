# How I run it?
That just depends if you want to test or help with coding.

## Installation

### I just want to test
In order to make tests with the local server, you will need:

- A computer with Windows 7 (or above);
- Darkspore installed;
- Your `hosts` file modified according to the specifications in **Server redirect** area in the README file (it may take a few minutes to take effect; rebooting the computer is faster sometimes) (that step won't be needed in the future).

Copy the Darkspore.exe file to its same folder (don't confuse it with the Darkspore.exe.exe file, that may exist; Darkspore.exe size is over 15Mb, while the Darkspore.exe.exe size is around 66Kb). Open the Darkspore.exe copy with a hex editor, and then replace `80 BF 2C 01 00 00 00` with `80 BF 2C 01 00 00 01`. If you find more than one occurence of `80 BF 2C 01 00 00 00`, replace only the last one.

Download the latest version of DLS from the Releases tab here in Github. 

### I want to help coding
In order to start contributing with the local server, you will need:

- A computer with Windows 10;
- Install the DLS dependencies;
   - Install Visual Studio 2019 ([Download](https://visualstudio.microsoft.com/thank-you-downloading-visual-studio/?sku=Community&rel=16));
- Darkspore installed;
- Your `hosts` file modified according to the specifications in **Server redirect** area in the README file (it may take a few minutes to take effect; rebooting the computer is faster sometimes) (that step won't be needed in the future).

Copy the Darkspore.exe file to its same folder (don't confuse it with the Darkspore.exe.exe file, that may exist; Darkspore.exe size is over 15Mb, while the Darkspore.exe.exe size is around 66Kb). Open the Darkspore.exe copy with a hex editor, and then replace `80 BF 2C 01 00 00 00` with `80 BF 2C 01 00 00 01`. If you find more than one occurence of `80 BF 2C 01 00 00 00`, replace only the last one.

To use the server, build it in **Release x64** (VERY IMPORTANT).

## Start DLS
Start DLS. While it is running, launch Darkspore with the exe copy, and keep DLS running while using it.

Press Play in the Darkspore launcher and wait for the login screen. Using the Register button in the Darkspore login screen, create a user account (it will only exist in your computer, and you won't need internet to do that). After creating the user, open the following link in your browser:

- http://config.darkspore.com/panel/

It should show a template page (sorry, still didn't have time to clean it up). Press the Users button in the top, and then you should see a list with all the users in your local server (which at this point should be only the user that you just created). Select your user mail, and then press **"Start testing"**.

After that, go back to Darkspore and login. You should be able to use the hero editor now, which is as far as we could go until this day (05/09/2019).