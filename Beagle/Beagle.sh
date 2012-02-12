#!/bin/sh
port=1234
./app_DEBUG.Beagle | nc -l -p $port
