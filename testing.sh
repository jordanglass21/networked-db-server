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
# This one runs 100 requests over 5 threads.
# Then prints the stats. Notice the sum of the write and read is 100.
echo "Test 1: Running 100 requests on 5 threads"
(sleep 5; echo stats; echo quit) | ./dbserver 5001 || echo FAILED & sleep 1
./dbtest --port=5001 --count=100 --threads=5 
wait
echo

# Test 2
# This verifies that a value is set to a key by retreaving the value.
echo "Test 2: Setting and Getting a Key Value Pair" 
(sleep 5; echo quit) | ./dbserver 5002 || echo FAILED&
sleep 1
./dbtest --port=5002 --set=KEY VAL
output="$(./dbtest --port=5002 --get=KEY)"
test_result "$output" '="VAL"'
wait
echo

# Test 3
# Writes a key, and then overwrites the exsisting key to a new value.
echo "Test 3: Overwriting an Existing Key"
(sleep 5; echo quit) | ./dbserver 5003 || echo FAILED &
sleep 1
./dbtest --port=5003 --set=KEY VAL
./dbtest --port=5003 --set=KEY NEW_VAL
output="$(./dbtest --port=5003 --get=KEY)"
test_result "$output" '="NEW_VAL"'
wait
echo

# Test 4
# Writes a value, deletes the value, and then reads the value again.
# The read shoudl fail because the value is deleted.
echo "Test 4: Deleting a Key"
(sleep 5; echo quit) | ./dbserver 5004 || echo FAILED &
sleep 1
./dbtest --port=5004 --set=KEY VAL
./dbtest --port=5004 --delete=KEY
output="$(./dbtest --port=5004 --get=KEY)"
test_result "$output" "READ: FAILED (X)"
wait
echo

# Test 5
# The read should fail becuase we are reading a value before we ever
# write to it.
echo "Test 5: Retrieving a Key That has no Value"
(sleep 5; echo quit) | ./dbserver 5005 || echo FAILED &
sleep 1
output="$(./dbtest --port=5005 --get=KEY)"
test_result "$output" "READ: FAILED (X)"
wait
echo

# Test 6
# Concurrently reads and writes a data,
# and only after it well once again read the data.
echo "Test 6: Concurrent Read/Write Operations"
(sleep 5; echo quit) | ./dbserver 5006 || echo FAILED &
sleep 1
./dbtest --port=5006 --set=KEY VAL &
s_pid=$!
./dbtest --port=5006 --get=KEY &
g_pid=$!
wait $s_pid $g_pid
output="$(./dbtest --port=5006 --get=KEY)"
test_result "$output" '="VAL"'
wait
echo

# Test 7
# Run 1000 read and write requests.
# Notice the sum of read and write requests printed by the stats
# function is queal to 1000.
echo "Test 7: Running 1000 Requests"
(sleep 5; echo stats; echo quit) | ./dbserver 5007 || echo FAILED &
sleep 1
./dbtest --port=5007 --count=1000 --threads=10
wait
echo

# Test 8
# Concurrenlty setting mutliple key value pairs.
# Then read the key value pairs to verify.
echo "Test 8: Concurrently Setting Multiple Keys"
(sleep 5; echo quit) | ./dbserver 5008 || echo FAILED &
sleep 1
./dbtest --port=5008 --set=KEY_1 VAL_1 &
./dbtest --port=5008 --set=KEY_2 VAL_2
output1="$(./dbtest --port=5008 --get=KEY_1)"
output2="$(./dbtest --port=5008 --get=KEY_2)"
test_result "$output1" '="VAL_1"'
test_result "$output2" '="VAL_2"'
wait
echo

# Test 9
# First set multiple keys values pairs.
# Then concurrenlty get these values.
echo "Test 9: Concurrently Getting Multiple Values"
(sleep 5; echo quit) | ./dbserver 5009 || echo FAILED &
sleep 1
./dbtest --port=5009 --set=KEY_1 VAL_1
./dbtest --port=5009 --set=KEY_2 VAL_2
output1="$(./dbtest --port=5009 --get=KEY_1)" &
output2="$(./dbtest --port=5009 --get=KEY_2)"
test_result "$output1" '="VAL_1"'
test_result "$output2" '="VAL_2"'
wait
echo

# Test 10
# First, concurrently set multiple key values pairs.
# Then, concurrenlty get multiple values.
echo "Test 10: Concurrent Reading and Writing"
(sleep 5; echo quit) | ./dbserver 5010 || echo FAILED &
sleep 1
./dbtest --port=5010 --set=KEY_1 VAL_1 &
./dbtest --port=5010 --set=KEY_2 VAL_2
output1="$(./dbtest --port=5010 --get=KEY_1)" &
output2="$(./dbtest --port=5010 --get=KEY_2)"
test_result "$output1" '="VAL_1"'
test_result "$output2" '="VAL_2"'
wait
echo

# Test 11
# This on does not work...
echo "Test 11: Empty String for Key and Value Pair"
(sleep 5; echo quit) | ./dbserver 5011 || echo FAILED &
sleep 2
./dbtest --port=5011 --set=""
./dbtest --port=5011 --get=""
output="$(./dbtest --port=5011 --get=)"
test_result "$output" '=""'
wait
echo

# Test 12
# Write a key value pair.
# Then concurrently get that value four times.
echo "Test 12: Many Concurrent Reads"
(sleep 5; echo quit) | ./dbserver 5012|| echo FAILED &
sleep 2
./dbtest --port=5012 --set=KEY VAL
./dbtest --port=5012 --get=KEY &
./dbtest --port=5012 --get=KEY &
./dbtest --port=5012 --get=KEY &
./dbtest --port=5012 --get=KEY &
output1="$(./dbtest --port=5012 --get=KEY)"
output2="$(./dbtest --port=5012 --get=KEY)"
output3="$(./dbtest --port=5012 --get=KEY)"
test_result "$output1" '="VAL"'
test_result "$output2" '="VAL"'
test_result "$output3" '="VAL"'
wait
echo

# Test 13
# First concurrently write a key value pair four times.
# Then, concurrently read those values four times.
echo "Test 13: Many Concurrent Reads and Writes"
(sleep 5; echo quit) | ./dbserver 5013 || echo FAILED &
sleep 2
./dbtest --port=5013 --set=KEY_1 VAL_1 &
./dbtest --port=5013 --set=KEY_2 VAL_2 &
./dbtest --port=5013 --set=KEY_3 VAL_3 &
./dbtest --port=5013 --set=KEY_4 VAL_4
./dbtest --port=5013 --get=KEY_1 &
./dbtest --port=5013 --get=KEY_2 &
./dbtest --port=5013 --get=KEY_3 &
./dbtest --port=5013 --get=KEY_4
output1="$(./dbtest --port=5013 --get=KEY_1)"
output2="$(./dbtest --port=5013 --get=KEY_2)"
output3="$(./dbtest --port=5013 --get=KEY_3)"
output4="$(./dbtest --port=5013 --get=KEY_4)"
test_result "$output1" '="VAL_1"'
test_result "$output2" '="VAL_2"'
test_result "$output3" '="VAL_3"'
test_result "$output4" '="VAL_4"'
wait
echo

# Test 14
# First concurrenlty write a key values pair four times.
# Then concurrenlty get those values.
# Then concurrenlty delete all four values.
echo "Test 14: Many Concurrent Deletes"
(sleep 5; echo quit) | ./dbserver 5014|| echo FAILED &
sleep 2
./dbtest --port=5014 --set=KEY_1 VAL_1 &
./dbtest --port=5014 --set=KEY_2 VAL_2 &
./dbtest --port=5014 --set=KEY_3 VAL_3 &
./dbtest --port=5014 --set=KEY_4 VAL_4
./dbtest --port=5014 --get=KEY_1 &
./dbtest --port=5014 --get=KEY_2 &
./dbtest --port=5014 --get=KEY_3 &
./dbtest --port=5014 --get=KEY_4
./dbtest --port=5014 --delete=KEY_1 &
./dbtest --port=5014 --delete=KEY_2 &
./dbtest --port=5014 --delete=KEY_3 &
./dbtest --port=5014 --delete=KEY_4
output1="$(./dbtest --port=5014 --get=KEY_1)"
output2="$(./dbtest --port=5014 --get=KEY_2)"
output3="$(./dbtest --port=5014 --get=KEY_3)"
output4="$(./dbtest --port=5014 --get=KEY_4)"
test_result "$output1" 'READ: FAILED (X)'
test_result "$output2" 'READ: FAILED (X)'
test_result "$output3" 'READ: FAILED (X)'
test_result "$output3" 'READ: FAILED (X)'
wait
echo

# Test 15
# Used to ensure thr resource busy functionality is wokring properly
# by issuing concurrent set, delete and get requests twenty times.
echo "Test 15: Test Non-deterministic Behavior and Resource Busy"
(sleep 120; echo stats; echo quit) | ./dbserver 5015|| echo FAILED &
sleep 2
for i in {1..20}
do
echo "===$i==="
(
	./dbtest --port=5015 --set=KEY VAL &
	./dbtest --port=5015 --delete=KEY &
	./dbtest --port=5015 --set=KEY VAL2 &
	./dbtest --port=5015 --get=KEY &
	wait
	echo "======"
) &
sleep 5
done
wait
echo
