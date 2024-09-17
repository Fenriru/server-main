#!/bin/bash

# usage: ./runtests.sh [test_dir_regex] (test_file_regex)
# where test_file_regex is optional
# example: ./runtests.sh 01-connect     // runs all tests in tests/01-connect/
#          ./runtests.sh 01-connect 01  // runs tests/01-connect/01.sh
#          ./runtests.sh 01 01          // runs tests/01-connect/01.sh

EXEC=hw4
DUMMY=test
OUTPUT=output
TEST_DIR=tests
DIFF_FILE=d

ALL_PASS=1

RED='\033[41;37m'
GREEN='\033[42m'
BLUE='\033[44m'
RESET='\033[0m'

function choose_single_test
{
    if [ $# -eq 1 ]
    then
        grep -E "$1"
    else
        :
    fi
}

export ASAN_OPTIONS="detect_leaks=false"

rm output -f
make clean && make all

ulimit -n 100

START=$(date +%s%N | cut -b1-13 | sed s/N/000/g)
for T in $(ls $TEST_DIR | grep -E "$1" | sort)
do
    for F in $(ls $TEST_DIR/$T | grep ".sh$" | choose_single_test "$2".sh | sort)
    do
        SHORT_NAME=$(echo $F | cut -d "." -f1)
        NAME="${T}/${SHORT_NAME}"
        echo -e "\n\n${BLUE} Running Test ${RESET} ${NAME}"

        TESTFILE="$TEST_DIR/$T/$F"
        EXPECTED=$(sed 's/.sh$/.expected/g' <<<"$TESTFILE")

        killall -9 $EXEC &> /dev/null

        # Start server in background
        bash -c "echo \$\$ > pid ; exec ./$EXEC" &

        T1=$(date +%s%N | cut -b1-13 | sed s/N/000/g)
        TRIES=0

        # Wait until server accepts requests on port
        until nc -z 127.0.0.1 `cat port.txt` ;
        do
            echo "$TRIES"
            ((TRIES++))
            sleep 0.1
            if [[ "$TRIES" -gt 10 ]]; then
                killall -9 $EXEC &> /dev/null
                exit 0
            fi
        done

        PID=`cat pid`

        T1=$(date +%s%N | cut -b1-13 | sed s/N/000/g)
        ./$TESTFILE `cat port.txt` >$OUTPUT 2>&1
        RET=$?
        T2=$(date +%s%N | cut -b1-13 | sed s/N/000/g)
        TT=$((T2-T1))

        # Kill server
        kill -9 $PID &> /dev/null
        wait $PID &> /dev/null

        if [ $RET -eq 0 ]
        then
            echo -e "${BLUE} TEST ${NAME} RUN OK in ${TT}ms ${RESET}"
            if [ -f "$EXPECTED" ]; then
                diff -y $EXPECTED $OUTPUT &> $DIFF_FILE
                DIFF=$?
                rm $DUMMY $OUTPUT &> /dev/null

                if [ $DIFF -eq 0 ]
                then
                    echo -e " ${GREEN} DIFF OK ${RESET}\t"
                else
                    echo -e " ${RED} DIFF FAIL ${RESET}"
                    cat $DIFF_FILE
                fi
            else
                DIFF=0
            fi
        else
            echo -e "${RED} TEST ${NAME} FAILED in ${TT}ms ${RESET}"
            cat $OUTPUT
        fi

        if [ $RET -ne 0 ] || [ $DIFF -ne 0 ]
        then
            ALL_PASS=0
            echo -e "${RED}                   TEST ${NAME} FAILED                 ${RESET}"
        else
            echo -e "${GREEN}                   TEST ${NAME} PASSED              ${RESET}"
        fi
    done
done
END=$(date +%s%N | cut -b1-13 | sed s/N/000/g)
echo "Finished in $((END-START))ms"

killall -9 $EXEC &> /dev/null

rm -f d pid actual expected output read file actual1 actual6 file2

if [ $ALL_PASS -eq 1 ]
then
    echo -e "${GREEN}All tests passed${RESET}"
else
    echo -e "${RED}Some tests failed${RESET}"
fi
