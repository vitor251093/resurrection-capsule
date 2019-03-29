#cxfreeze /darkspore_server/hello.py --target-dir /darkspore_server_storage/dist --target-name=DarksporeServer.exe
cd /darkspore_server
if [ -d "/darkspore_server_storage/dist" ]; then rm -Rf /darkspore_server_storage/dist; fi
mkdir /darkspore_server_storage/dist
python buildexe.py build_exe -b /darkspore_server_storage/dist
