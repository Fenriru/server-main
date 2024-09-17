#!/bin/bash

PORT=$@

curl -s http://127.0.0.1:$PORT/tests/05-files/index.html