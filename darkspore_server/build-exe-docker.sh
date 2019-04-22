# Removing files from previous builds
if [ -d "/darkspore_server_storage/build" ]; then rm -Rf /darkspore_server_storage/build; fi
mkdir /darkspore_server_storage/build
if [ -d "/darkspore_server_storage/dist" ]; then rm -Rf /darkspore_server_storage/dist; fi
mkdir /darkspore_server_storage/dist

# Building EXE
cd /darkspore_server
cp /darkspore_server_storage/www/static/favicon.ico favicon.ico
cp pyinstaller/info.txt info.txt
wine ~/.wine/drive_c/Python27/Scripts/pyinstaller.exe --onefile --icon=favicon.ico --version-file=info.txt main.py
cd /darkspore_server_storage
mv /darkspore_server/dist/main.exe build/DarksporeServer.exe
cp /darkspore_server/config.txt build/config.txt
cp /darkspore_server/version.txt build/version.txt
mkdir build/storage
cp -r /darkspore_server_storage/www build/storage/www

# Turning EXE and dependencies into a single EXE file
cd /darkspore_server_storage/build
7z a /darkspore_server_storage/dist/DarksporeServer.7z /darkspore_server_storage/build/*
cd /darkspore_server_storage/dist/

mkdir 7z920_extra
cd 7z920_extra
wget "https://downloads.sourceforge.net/project/sevenzip/7-Zip/9.20/7z920_extra.7z" -O 7z920_extra.7z
7z x 7z920_extra.7z
cd ..
mv 7z920_extra/7zS.sfx 7zS.sfx
rm -r 7z920_extra

echo ';!@Install@!UTF-8!' > config.txt
echo 'Title="DarksporeServer"' >> config.txt
echo 'RunProgram="DarksporeServer.exe"' >> config.txt
echo ';!@InstallEnd@!' >> config.txt
cat 7zS.sfx config.txt DarksporeServer.7z > DarksporeServer.exe
rm 7zS.sfx
rm config.txt
rm DarksporeServer.7z

# References
# https://superuser.com/a/42792
# https://superuser.com/a/923281
