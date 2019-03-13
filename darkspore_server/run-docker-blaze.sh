#!/bin/bash

python /darkspore_server/blaze/Init.py
cat /darkspore_server_storage/logs/$(ls /darkspore_server_storage/logs | grep "dls-" | tail -1)
