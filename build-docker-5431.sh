#!/bin/bash

docker build -t darkspore_server .
docker run -it -p 5431:5431/tcp -v $(pwd)/storage:/darkspore_server_storage --name darkspore_server_5431 -td darkspore_server
docker exec -it darkspore_server_5431 bash /darkspore_server/run-docker-5431.sh
