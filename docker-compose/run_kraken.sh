#!/bin/bash

python /monitor_kraken & LD_PRELOAD=/libkeepalive/libkeepalive.so /usr/bin/kraken
