#!/bin/bash

FLASK_APP=/darkspore_server/main.py flask run --host=0.0.0.0
cat /darkspore_server_storage/$(ls /darkspore_server_storage | grep "dls-" | tail -1)