#!/bin/bash
cd darkspore_launcher
docker build -t darkspore_server .
cd ..
docker run -it -v $(pwd)/storage:/darkspore_server_storage --name darkspore_server -td darkspore_server
docker exec -it darkspore_server bash /darkspore_launcher/build-launcher-docker.sh
