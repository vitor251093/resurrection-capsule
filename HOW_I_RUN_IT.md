# How I run it?
That just depends if you want to test or help with coding.

## Installation

### I just want to test
In order to make tests with the local server, you will need:

- A computer with Windows 7 (or above);
- Darkspore installed;
- Your `hosts` file (C:\Windows\system32\Drivers\etc\hosts) modified according to the specifications in **Server redirect** area in the README file (it may take a few minutes to take effect; rebooting the computer is faster sometimes) (that step won't be needed in the future).

For the next step, you are going to need a hex editor. There are many free options
available out there. One that has been used and approved by our users is HxD:

- https://mh-nexus.de/en/hxd/

Copy the Darkspore.exe file to its same folder (don't confuse it with the Darkspore.exe.exe file, that may exist; Darkspore.exe size is over 15Mb, while the Darkspore.exe.exe size is around 66Kb). Open the Darkspore.exe copy with a hex editor, and then replace `80 BF 2C 01 00 00 00` with `80 BF 2C 01 00 00 01`. If you find more than one occurence of `80 BF 2C 01 00 00 00`, replace only the last one.

Download the latest version of ReCap from the Releases tab here in Github. 

### I want to help coding
In order to start contributing with the local server, you will need:

- A computer with Windows 10;
- Install the ReCap dependencies;
   - Install Visual Studio 2019 ([Download](https://visualstudio.microsoft.com/thank-you-downloading-visual-studio/?sku=Community&rel=16));
- Darkspore installed;
- Your `hosts` file (C:\Windows\system32\Drivers\etc\hosts) modified according to the specifications in **Server redirect** area in the README file (it may take a few minutes to take effect; rebooting the computer is faster sometimes) (that step won't be needed in the future).

For the next step, you are going to need a hex editor. There are many free options
available out there. One that has been used and approved by our users is HxD:

- https://mh-nexus.de/en/hxd/

Copy the Darkspore.exe file to its same folder (don't confuse it with the Darkspore.exe.exe file, that may exist; Darkspore.exe size is over 15Mb, while the Darkspore.exe.exe size is around 66Kb). Open the Darkspore.exe copy with a hex editor, and then replace `80 BF 2C 01 00 00 00` with `80 BF 2C 01 00 00 01`. If you find more than one occurence of `80 BF 2C 01 00 00 00`, replace only the last one.

To use the server, build it in **Release x64** (VERY IMPORTANT).

## Start ReCap
Start ReCap. While it is running, launch Darkspore with the exe copy, and keep ReCap running while using it.

Press Play in the Darkspore launcher and wait for the login screen. Using the Register button in the Darkspore login screen, create a user account (it will only exist in your computer, and you won't need internet to do that). After creating the user, login with it. You should be able to use the hero editor, which is as far as we could go until this day (17/02/2020).
