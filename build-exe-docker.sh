#!/bin/bash
cd build_exe
docker build -t darkspore_server .
cd ..
docker run -it -p 80:80/tcp -p 42127:42127/tcp -v $(pwd)/storage:/darkspore_server_storage --name darkspore_server -td darkspore_server
docker exec -it darkspore_server C:\darkspore_server\build-exe-docker.bat
