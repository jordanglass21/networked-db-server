# Test Scripting File
# Before running the tests first run command:
# $ chmod +x testing.sh
# Then to run the tests:
# ./testing.sh

# Test 0
# send quit command after 5 seconds, output FAILED if dbserver
# exits with exit(1). [exit(0) is OK, exit(1) is failure]
#
(sleep 5; echo quit) | ./dbserver 5000 || echo FAILED&
# give it a second to get up and running. A usleep command would be nice...
sleep 1
./dbtest --port=5000 --count=100 --threads=1
wait

# Test 1
# send quit command after 5 seconds, output FAILED if dbserver
# exits with exit(1). [exit(0) is OK, exit(1) is failure]
#
##(sleep 5; echo quit) | ./dbserver 5001 || echo FAILED&
# give it a second to get up and running. A usleep command would be nice...
##sleep 1
##./dbtest --port=5001 --count=100 --threads=5
##wait

