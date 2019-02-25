#!/bin/bash

python /darkspore_server/main-42127.py
#FLASK_APP=/darkspore_server/main-42127.py flask run --host=0.0.0.0 --ssl_context='adhoc'
cat /darkspore_server_storage/$(ls /darkspore_server_storage | grep "dls-42127-" | tail -1)
