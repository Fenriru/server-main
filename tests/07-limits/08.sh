#!/bin/bash
# echo limits and post limits

PORT=$@

EOL=$'\r\n'

FIRST=$'this should not be changed'
REQUEST=$'POST /write HTTP/1.1'${EOL}$'Content-Length: '${#FIRST}${EOL}${EOL}${FIRST}
printf "$REQUEST" | nc -N 127.0.0.1 $PORT

DATA=`printf "%1300s" | tr " " "N"`
EREQUEST=$'GET /echo HTTP/1.1'${EOL}$'Header1: '${DATA}${EOL}${EOL}
PREQUEST=$'POST /write HTTP/1.1'${EOL}$'Content-Length: '${#DATA}${EOL}${EOL}${DATA}
RREQUEST=$'GET /read HTTP/1.1'${EOL}${EOL}

for _ in $(seq 1 2); do
    printf "$EREQUEST" | nc -N 127.0.0.1 $PORT
    printf "\n"
    printf "$PREQUEST" | nc -N 127.0.0.1 $PORT
    printf "\n"
    printf "$RREQUEST" | nc -N 127.0.0.1 $PORT
    printf "\n"
done
