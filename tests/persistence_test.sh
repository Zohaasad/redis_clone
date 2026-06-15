
echo "Starting persistence test..."

./minired --port 6399 --data ./test_data &
SERVER_PID=$!
sleep 1

for i in $(seq 1 10000); do
    redis-cli -p 6399 SET "str:$i" "value$i" > /dev/null
done
echo "Inserted 10000 strings"


for i in $(seq 1 10000); do
    redis-cli -p 6399 RPUSH "list:$i" "item$i" > /dev/null
done
echo "Inserted 10000 lists"

for i in $(seq 1 10000); do
    redis-cli -p 6399 HSET "hash:$i" "field" "value$i" > /dev/null
done
echo "Inserted 10000 hashes"


for i in $(seq 1 10000); do
    redis-cli -p 6399 ZADD "zset:$i" $i "member$i" > /dev/null
done
echo "Inserted 10000 zsets"


redis-cli -p 6399 SAVE
echo "Saved!"


kill $SERVER_PID
sleep 1


./minired --port 6399 --data ./test_data &
SERVER_PID=$!
sleep 1


echo "Verifying..."
PASS=0
FAIL=0

for i in 1 100 500 1000 5000 10000; do
    VAL=$(redis-cli -p 6399 GET "str:$i")
    if [ "$VAL" = "value$i" ]; then
        PASS=$((PASS+1))
    else
        FAIL=$((FAIL+1))
        echo "FAIL: str:$i expected value$i got $VAL"
    fi
done


for i in 1 100 500 1000 5000 10000; do
    VAL=$(redis-cli -p 6399 LINDEX "list:$i" 0)
    if [ "$VAL" = "item$i" ]; then
        PASS=$((PASS+1))
    else
        FAIL=$((FAIL+1))
        echo "FAIL: list:$i"
    fi
done

for i in 1 100 500 1000 5000 10000; do
    VAL=$(redis-cli -p 6399 HGET "hash:$i" field)
    if [ "$VAL" = "value$i" ]; then
        PASS=$((PASS+1))
    else
        FAIL=$((FAIL+1))
        echo "FAIL: hash:$i"
    fi
done

for i in 1 100 500 1000 5000 10000; do
    VAL=$(redis-cli -p 6399 ZSCORE "zset:$i" "member$i")
    if [ "$VAL" = "$i" ]; then
        PASS=$((PASS+1))
    else
        FAIL=$((FAIL+1))
        echo "FAIL: zset:$i"
    fi
done

kill $SERVER_PID
rm -rf ./test_data

echo ""
echo "Results: $PASS passed, $FAIL failed"