#!/bin/bash

docker build -t darkspore_server .
docker run -it -p 80:80/tcp -v $(pwd)/storage:/darkspore_server_storage --name darkspore_server_80 -td darkspore_server
docker exec -it darkspore_server_80 bash /darkspore_server/run-docker-80.sh
