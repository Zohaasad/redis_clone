#!/bin/bash

PORT=6380
HOST=127.0.0.1
OUTPUT="benchmark/benchmark_results.txt"

echo "========================================"
echo "  minired Benchmark Suite"
echo "  $(date)"
echo "========================================"
echo ""

# check server is running
PING_RESULT=$(redis-cli -p $PORT PING 2>&1)
if [ "$PING_RESULT" != "PONG" ]; then
    echo "ERROR: Server not running on port $PORT"
    echo "Start it with: ./minired --port $PORT --data ./dump.rdb"
    exit 1
fi

echo "Server is running on port $PORT"
echo ""

mkdir -p benchmark
> $OUTPUT

echo "========================================" | tee -a $OUTPUT
echo "  minired Benchmark Results"             | tee -a $OUTPUT
echo "  $(date)"                               | tee -a $OUTPUT
echo "========================================" | tee -a $OUTPUT
echo "" | tee -a $OUTPUT

echo "--- PING ---" | tee -a $OUTPUT
redis-benchmark -h $HOST -p $PORT -t ping -n 100000 -c 50 2>&1 | tee -a $OUTPUT
echo "" | tee -a $OUTPUT

echo "--- SET/GET ---" | tee -a $OUTPUT
redis-benchmark -h $HOST -p $PORT -t set,get -n 100000 -c 50 -r 10000 2>&1 | tee -a $OUTPUT
echo "" | tee -a $OUTPUT

echo "--- LPUSH/LPOP ---" | tee -a $OUTPUT
redis-benchmark -h $HOST -p $PORT -t lpush,lpop -n 50000 -c 50 2>&1 | tee -a $OUTPUT
echo "" | tee -a $OUTPUT

echo "--- HSET ---" | tee -a $OUTPUT
redis-benchmark -h $HOST -p $PORT -t hset -n 50000 -c 50 2>&1 | tee -a $OUTPUT
echo "" | tee -a $OUTPUT

echo "--- ZADD ---" | tee -a $OUTPUT
redis-benchmark -h $HOST -p $PORT -t zadd -n 50000 -c 50 2>&1 | tee -a $OUTPUT
echo "" | tee -a $OUTPUT

echo "--- INCR ---" | tee -a $OUTPUT
redis-benchmark -h $HOST -p $PORT -t incr -n 50000 -c 50 2>&1 | tee -a $OUTPUT
echo "" | tee -a $OUTPUT

echo "========================================" | tee -a $OUTPUT
echo "  Benchmark Complete"                     | tee -a $OUTPUT
echo "  Results saved to: $OUTPUT"              | tee -a $OUTPUT
echo "========================================" | tee -a $OUTPUT

echo ""
echo "Summary (ops/sec):"
grep "requests per second" $OUTPUT | while read line; do
    echo "  $line"
done
