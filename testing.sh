#!/bin/bash

# Test Scripting File
# Before running the tests first run command:
# $ chmod +x testing.sh
# Then to run the tests:
# $ ./testing.sh

# Verifies that two inputs are equal.
# Used to validate the results of the tests.
test_result () {
	if [ "$1" = "$2" ]; then
		echo "Test Passed"
	else
		echo "Test Failed"
	fi
	echo "Expected: $2"
	echo "Result: $1"
}

# Test 1
# This one is more of a proof of concept. It runs 100 requests over 5 threads.
# Then prints the stats. Notice the sum of the write and read is 100, but there are also deletes...
# Not sure if that is correct. Also would be cool to somehow automate the result checking.
echo "Test 1: Running 100 requests on 5 threads"
(sleep 5; echo stats; echo quit) | ./dbserver 5001 || echo FAILED & sleep 1
./dbtest --port=5001 --count=100 --threads=5
echo "Test 1 Complete"
wait

# Test 2
# This verifies that a value is set to a key and then got. Would be cool if we could find a way
# to not have to format the value when passing it to the test_result function.
echo "Test 2: Setting and Getting a Key Value Pair" 
(sleep 5; echo quit) | ./dbserver 5002 || echo FAILED&
sleep 1
./dbtest --port=5002 --set=KEY VAL
output="$(./dbtest --port=5002 --get=KEY)"
test_result "$output" '="VAL"'
wait

# Test 3
echo "Test 3: Overwriting an Existing Key"
(sleep 5; echo quit) | ./dbserver 5003 || echo FAILED &
sleep 1
./dbtest --port=5003 --set=KEY VAL
./dbtest --port=5003 --set=KEY NEWVAL
output="$(./dbtest --port=5003 --get=KEY)"
test_result "$output" "NEWVAL"
wait

# Test 4
echo "Test 4: Deleting a Key"
(sleep 5; echo quit) | ./dbserver 5004 || echo FAILED &
sleep 1
./dbtest --port=5004 --set=KEY VAL
./dbtest --port=5004 --delete=KEY
output="$(./dbtest --port=5004 --get=KEY)"
test_result "$output" "NOT_FOUND"
wait

# Test 5
echo "Test 5: Retrieving a Nonexistent Key"
(sleep 5; echo quit) | ./dbserver 5005 || echo FAILED &
sleep 1
output="$(./dbtest --port=5005 --get=KEY)"
test_result "$output" "NOT_FOUND"
wait

# Test 6
echo "Test 6: Concurrent Read/Write Operations"
(sleep 5; echo quit) | ./dbserver 5006 || echo FAILED &
sleep 1
./dbtest --port=5006 --set=KEY VAL &
./dbtest --port=5006 --get=KEY &
wait
output="$(./dbtest --port=5006 --get=KEY)"
test_result "$output" "VAL"
wait

# Test 7
echo "Test 7: Stress Testing with 1000 Requests"
(sleep 5; echo quit) | ./dbserver 5007 || echo FAILED &
sleep 1
./dbtest --port=5007 --count=1000 --threads=10
echo "Test 7 Complete"
wait