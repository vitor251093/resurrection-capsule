#!/bin/bash

docker build -t darkspore_server .
docker run -it -p 42127:42127/tcp -v $(pwd)/storage:/darkspore_server_storage --name darkspore_server_42127 -td darkspore_server
docker exec -it darkspore_server_42127 bash /darkspore_server/run-docker-42127.sh
