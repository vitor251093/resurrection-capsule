# How I run it?
That just depends if you want to test or help with coding.

## I just want to test
In order to make tests with the local server, you will need:

- A computer with Linux, MacOS or Windows 7 (or above);
- Darkspore installed (use Wine, or relatives, if you are in Linux/macOS);
- Your `hosts` file modified according to the specifications in **Server redirect** area in the README file (will change in the future).

Download the latest version of DLS from the Releases tab here in Github, and just run it. While it is running, launch Darkspore, and keep DLS running while using it.

## I want to help coding
In order to start contributing with the local server, you will need:

### Docker version
- A computer with Linux, MacOS or Windows 10;
- The Docker application installed (you can download it from one of the links below);
   - Windows: https://hub.docker.com/editions/community/docker-ce-desktop-windows
   - macOS: https://hub.docker.com/editions/community/docker-ce-desktop-mac
   - Linux: https://docs.docker.com/install/linux/docker-ce/ubuntu/#install-docker-ce
- Darkspore installed (use Wine, or relatives, if you are in Linux/macOS);
- Your `hosts` file modified according to the specifications in **Server redirect** area in the README file (will change in the future).

To start the server:
- Windows: Open the Command Prompt (cmd) and run the build-docker.bat file with it
- Linux/macOS: Open the Terminal and run the build-docker.sh file with it

### Non-Docker version
- A computer with Linux, MacOS or Windows XP (or above);
- Install the DLS dependencies;
   - Install Python 2.7 and PIP;
   - Use PIP to install `setuptools`, `flask`, `python-magic`, `twisted`, `pyopenssl` and `service_identity`;
   - (Windows) Use PIP to install `python-libmagic` and `python-magic-bin==0.4.14`;
   - (Windows) Install VCforPython from Microsoft;
- Darkspore installed (use Wine, or relatives, if you are in Linux/macOS);
- Your `hosts` file modified according to the specifications in **Server redirect** area in the README file (will change in the future).

To start the server:
- Windows: Open the Command Prompt (cmd) and run the darkspore_server_python/run-nodocker.bat file with it
- Linux/macOS: Open the Terminal and run the darkspore_server_python/run-nodocker.sh file with it

## Common errors

### twisted.internet.error.CannotListenError (Error 10048)
DLS needs to listen to the 80 and 42127 ports in order to work. If that happens, use that command to find out the process ID of the process that is doing that:

```
netstat -ano | findstr PORT_NUMBER_HERE
```

And then use the command below to kill it:

```
taskkill /PID PROCESS_ID_HERE /F
```
