#
# Name: Lukáš Drahník
# Project: ISA: Měření ztrátovosti a RTT (Matěj Grégr)
# Date: 30.6.2020
# Email: <xdrahn00@stud.fit.vutbr.cz>, <ldrahnik@gmail.com>
#

PORT=10015
OUTPUT_FILE=./tests/tmp/output
RUNNING_TIME_IN_SECONDS=2

# waitAndKillLastProcess 2
function waitAndKillLastProcess {
    PID=$!
    sleep $1
    sudo kill -INT $PID 2>/dev/null
    sudo wait $PID 2>/dev/null
}

# 1 IPv4
sudo ./testovac localhost -v > $OUTPUT_FILE 2>&1 &
waitAndKillLastProcess 2

if [ -s $OUTPUT_FILE ]; then
    echo "*******TEST 1 PASSED - IPv4";
else
    echo "TEST 1 FAILED - IPv4";
fi

rm -f $OUTPUT_FILE

# 2 IPv6
sudo ./testovac ::1 -v > $OUTPUT_FILE 2>&1 &
waitAndKillLastProcess 2

if [ -s $OUTPUT_FILE ]; then
    echo "*******TEST 2 PASSED - IPv6";
else
    echo "TEST 2 FAILED - IPv6";
fi

rm -f $OUTPUT_FILE

# 3 UDP server
sudo ./testovac localhost -u -p $PORT -l $PORT -v > $OUTPUT_FILE 2>&1 &
waitAndKillLastProcess 2

if [ -s $OUTPUT_FILE ]; then
    echo "*******TEST 3 PASSED - UDP server";
else
    echo "TEST 3 FAILED - UDP server";
fi

rm -f $OUTPUT_FILE
