# Removing files from previous builds
if exist "C:\darkspore_server_storage\build\" rmdir C:\darkspore_server_storage\build
mkdir C:\darkspore_server_storage\build
if exist "C:\darkspore_server_storage\dist\" rmdir C:\darkspore_server_storage\dist
mkdir C:\darkspore_server_storage\dist

# Building EXE
cd C:\darkspore_server
python buildexe.py build_exe -b C:\darkspore_server_storage\build
cp config-exe.txt C:\darkspore_server_storage\build\config-exe.txt
cp config.txt C:\darkspore_server_storage\build\config.txt
cp version.txt C:\darkspore_server_storage\build\version.txt
mkdir C:\darkspore_server_storage\build\storage
cp C:\darkspore_server_storage\server.data C:\darkspore_server_storage\build\storage\server.data
cp -r C:\darkspore_server_storage\www C:\darkspore_server_storage\build\storage\www

# Turning EXE and dependencies into a single EXE file
cd C:\darkspore_server_storage\build
7z a C:\darkspore_server_storage\dist\DarksporeServer.7z C:\darkspore_server_storage\build\*
cd C:\darkspore_server_storage\dist\

mkdir 7z920_extra
cd 7z920_extra
wget "https://downloads.sourceforge.net/project/sevenzip/7-Zip/9.20/7z920_extra.7z" -O 7z920_extra.7z
7z x 7z920_extra.7z
cd ..
mv 7z920_extra\7zS.sfx 7zS.sfx
rm -r 7z920_extra

cp ..\build\config-exe.txt config.txt
copy /b 7zS.sfx + config.txt + DarksporeServer.7z DarksporeServer.exe
rm 7zS.sfx
rm config.txt
rm DarksporeServer.7z

# References
# https://superuser.com/a/42792
# https://superuser.com/a/923281
