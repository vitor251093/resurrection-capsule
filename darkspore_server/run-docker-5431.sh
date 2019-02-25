#!/bin/bash

python /darkspore_server/main-5431.py
cat /darkspore_server_storage/$(ls /darkspore_server_storage | grep "dls-5431-" | tail -1)
