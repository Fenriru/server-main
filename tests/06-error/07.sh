#! /bin/bash
# if post request is malformed -> 404

PORT=$@

EOL=$'\r\n'
DATA=$'This post is malformed!'
REQUEST1=$'POST /write HTTP/1.1\r
Content-Length: '${#DATA}${EOL}${DATA}
REQUEST2=$'GET /read HTTP/1.1'${EOL}${EOL}

for i in $(seq 1 3); do
    printf "$REQUEST1" | nc 127.0.0.1 $PORT
    printf "\n"
    printf "$REQUEST2" | nc 127.0.0.1 $PORT
done
