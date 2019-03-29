import sys
from cx_Freeze import setup, Executable

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
    version = "0.1",
    description = "Darkspore LS server",
    options = {"build_exe": build_exe_options},
    executables = [exe]
)
