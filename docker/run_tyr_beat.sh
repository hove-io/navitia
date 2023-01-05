#!/bin/bash

exec celery beat -A tyr.tasks
