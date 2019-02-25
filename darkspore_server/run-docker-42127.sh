#!/bin/bash

python /darkspore_server/main-42127.py
cat /darkspore_server_storage/$(ls /darkspore_server_storage | grep "dls-42127-" | tail -1)
