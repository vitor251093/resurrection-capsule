#!/bin/bash

python /darkspore_server/main.py docker debug
cat /darkspore_server_storage/logs/$(ls /darkspore_server_storage/logs | grep "dls-" | tail -1)
