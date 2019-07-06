# How I run it?
That just depends if you want to test or help with coding.

## I just want to test
In order to make tests with the local server, you will need:

- A computer with Windows 7 (or above);
- Darkspore installed;
- Copy the Darkspore.exe file to its same folder;
- Open the Darkspore.exe copy with a hex editor, and then replace `80 BF 2C 01 00 00 00` with `80 BF 2C 01 00 00 01`.
   - If you find more than one occurence of `80 BF 2C 01 00 00 00`, replace only the last one.
- Your `hosts` file modified according to the specifications in **Server redirect** area in the README file (it may take a few minutes to take effect; rebooting the computer is faster sometimes) (that step won't be needed in the future).

Download the latest version of DLS from the Releases tab here in Github, and just run it. While it is running, launch Darkspore with the exe copy, and keep DLS running while using it.

## I want to help coding
In order to start contributing with the local server, you will need:

- A computer with Windows 10;
- Install the DLS dependencies;
   - Install Visual Studio 2019 ([Download](https://visualstudio.microsoft.com/thank-you-downloading-visual-studio/?sku=Community&rel=16));
- Darkspore installed;
- Copy the Darkspore.exe file to its same folder;
- Open the Darkspore.exe copy with a hex editor, and then replace `80 BF 2C 01 00 00 00` with `80 BF 2C 01 00 00 01`.
   - If you find more than one occurence of `80 BF 2C 01 00 00 00`, replace only the last one.
- Your `hosts` file modified according to the specifications in **Server redirect** area in the README file (it may take a few minutes to take effect; rebooting the computer is faster sometimes) (that step won't be needed in the future).

To start the server:
- Build it in **Release x64** (VERY IMPORTANT) and run it. Launch Darkspore with the exe copy, and keep DLS running while using it.
