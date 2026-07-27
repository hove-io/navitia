#!/bin/bash

python3.11 monitor_kraken & LD_PRELOAD=/libkeepalive/libkeepalive.so /usr/bin/kraken
