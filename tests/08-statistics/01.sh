#!/bin/bash

PORT=$@

REQUEST=$'GET /stats HTTP/1.1\r\n\r\n'
PING=$'GET /ping HTTP/1.1\r\n\r\n'

echo -n "$REQUEST" | nc 127.0.0.1 $PORT #0
echo ""
echo -n "$REQUEST" | nc 127.0.0.1 $PORT #1
echo ""
echo -n "$REQUEST" | nc 127.0.0.1 $PORT #2
echo ""
echo -n "$REQUEST" | nc 127.0.0.1 $PORT #3
echo ""
echo -n "$REQUEST" | nc 127.0.0.1 $PORT #4
echo ""
echo -n "$PING" | nc 127.0.0.1 $PORT &> /dev/null #5
echo -n "$REQUEST" | nc 127.0.0.1 $PORT #6
echo ""
curl -s --user-agent "curl/XXX" -H "HOST: localhost" http://127.0.0.1:$PORT/echo &> /dev/null #7
echo -n "$REQUEST" | nc 127.0.0.1 $PORT #8
echo ""
echo -n "$PING" | nc 127.0.0.1 $PORT &> /dev/null #9
echo -n "$REQUEST" | nc 127.0.0.1 $PORT #10
echo ""
curl -s --user-agent "curl/XXX" -H "HOST: localhost" http://127.0.0.1:$PORT/echo &> /dev/null #11
echo -n "$REQUEST" | nc 127.0.0.1 $PORT #12
echo ""
curl -s --user-agent "curl/XXX" -H "HOST: localhost" http://127.0.0.1:$PORT/echo &> /dev/null #13
echo -n "$REQUEST" | nc 127.0.0.1 $PORT #14
echo ""
curl -s --user-agent "curl/XXX" -H "HOST: localhost" http://127.0.0.1:$PORT/read &> /dev/null #15
echo -n "$REQUEST" | nc 127.0.0.1 $PORT #16
echo ""
curl -s --user-agent "curl/XXX" -H "HOST: localhost" http://127.0.0.1:$PORT/echo &> /dev/null #17
echo -n "$REQUEST" | nc 127.0.0.1 $PORT #18
echo ""
curl -s --user-agent "curl/XXX" -H "HOST: localhost" http://127.0.0.1:$PORT/tests/05-files/index.html &> /dev/null #19
echo -n "$REQUEST" | nc 127.0.0.1 $PORT #20
echo ""
curl -s --user-agent "curl/XXX" -H "HOST: localhost" http://127.0.0.1:$PORT/tests/06-files/index.html &> /dev/null #21
echo ""
echo -n "$REQUEST" | nc 127.0.0.1 $PORT #22
echo ""
curl -s --user-agent "curl/XXX" -H "HOST: localhost" http://127.0.0.1:$PORT/tests/07-error/index.html &> /dev/null #23
echo -n "$REQUEST" | nc 127.0.0.1 $PORT
echo ""
curl -s --user-agent "curl/XXX" -H "HOST: localhost" http://127.0.0.1:$PORT/tests/07-error/index.html &> /dev/null
echo -n "$REQUEST" | nc 127.0.0.1 $PORT
echo ""
