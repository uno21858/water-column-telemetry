#!/usr/bin/env bash
cd "$(dirname "$0")" || exit 1
pio run -t upload && pio device monitor
