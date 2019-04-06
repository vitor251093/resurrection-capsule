#!/bin/bash

python /darkspore_server/main.py docker debug travis
cat /darkspore_server_storage/logs/$(ls /darkspore_server_storage/logs | grep "dls-" | tail -1)
