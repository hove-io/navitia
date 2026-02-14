#!/bin/bash

celery -A tyr.tasks worker -O fair -c $TYR_WORKER_N_PROC
