#!/bin/bash

celery worker -A tyr.tasks -O fair -c $TYR_WORKER_N_PROC
