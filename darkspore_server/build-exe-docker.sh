#cxfreeze /darkspore_server/hello.py --target-dir /darkspore_server_storage/dist --target-name=DarksporeServer.exe
cd /darkspore_server
rm -r /darkspore_server_storage/dist/*
python buildexe.py build_exe -b /darkspore_server_storage/dist
