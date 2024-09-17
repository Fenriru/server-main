#!/bin/bash

PORT=$@

telnet 127.0.0.1 $PORT </dev/null
echo -n ""
