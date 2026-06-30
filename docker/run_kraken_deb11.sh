#!/bin/bash

python3.10 monitor_kraken & LD_PRELOAD=/libkeepalive/libkeepalive.so /usr/bin/kraken
