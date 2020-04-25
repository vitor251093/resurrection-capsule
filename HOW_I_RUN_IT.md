# How I run it?
That just depends if you want to test or help with coding.

## Installation

### I just want to test
In order to make tests with the local server, you will need:

- A computer with Windows 7 (or above);
- Darkspore installed;
- The ReCap dependency (https://aka.ms/vs/16/release/vc_redist.x64.exe);

Download the latest version of ReCap from the Releases tab here in Github. 

After that, you will need to find the Darkspore.exe. Depending of your Darkspore version, its folder will be different:
- DVD/Latest version: C:\Program Files (x86)\Electronic Arts\Darkspore\DarksporeBin
- Steam version: C:\Program Files (x86)\Steam\steamapps\common\Darkspore\DarksporeBin

Copy the `patch_darkspore_exe.exe` file from the patcher folder (from the ReCap release) to the DarksporeBin folder. Run the copied `patch_darkspore_exe.exe` file, and a new window should pop up; you can close after the success message appears. A new file should appear in the DarksporeBin folder after that, the Darkspore_local.exe. Use it to launch Darkspore.

### I want to help coding
Same as above, but you will also need to install Visual Studio 2019 ([Download](https://visualstudio.microsoft.com/thank-you-downloading-visual-studio/?sku=Community&rel=16)).

To build the server properly, use the **Release x64** setting (VERY IMPORTANT).

## Start Darkspore
Start ReCap. While it is running, launch Darkspore with the exe copy, and keep ReCap running while using it.

Press Play in the Darkspore launcher and wait for the login screen. Using the Register button in the Darkspore login screen, create a user account (it will only exist in your computer, and you won't need internet to do that). After creating the user, login with it. You should be able to use the hero editor, which is as far as we could go until this day (25/04/2020).
