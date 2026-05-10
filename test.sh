#!/bin/bash

shell=kitty

make build debug
./bin/debug.out --port 25565 --receiver --file .temp/t | less &
sleep 1
./bin/debug.out --port 25565 --sender --file . | less
