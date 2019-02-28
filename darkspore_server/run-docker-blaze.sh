#!/bin/bash

python /darkspore_server/blaze/Init.py
cat /darkspore_server_storage/$(ls /darkspore_server_storage | grep "dls-" | tail -1)
