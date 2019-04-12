# Removing files from previous builds
if [ ! -d "/darkspore_server_storage/dist" ]; then mkdir /darkspore_server_storage/dist; fi

# Building EXE
cd /darkspore_launcher
i686-w64-mingw32-g++ main.cpp -o /darkspore_server_storage/dist/DarksporeLauncher.exe
