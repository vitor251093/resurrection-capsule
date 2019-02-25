#!/bin/bash

python /darkspore_server/main-80.py
cat /darkspore_server_storage/$(ls /darkspore_server_storage | grep "dls-" | tail -1)
