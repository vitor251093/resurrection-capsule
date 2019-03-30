import sys
from cx_Freeze import setup, Executable

import os
from ..darkspore_server.controllers.config import *

version = "1"
config = DarkSporeServerConfig()
versionPath = os.path.join(config.serverPath, "version.txt")
versionFile = open(versionPath,"r")
lines = versionFile.readlines()
for line in lines:
    if len(line.strip()) > 0:
        version = line.strip()

# Dependencies are automatically detected, but it might need fine tuning.
build_exe_options = {"include_msvcr": True}

exe = Executable(
 # what to build
   script = "main.py",
   initScript = None,
   base = None, # if creating a GUI instead of a console app, type "Win32GUI"
   targetName = "DarksporeServer.exe", # this is the name of the executable file
   icon = None # if you want to use an icon file, specify the file name here
)

setup(
    # the actual setup & the definition of other misc. info
    name = "DarksporeServer",
    version = version,
    description = "Darkspore LS server",
    options = {"build_exe": build_exe_options},
    executables = [exe]
)
