**How to Run**

Simply type make into the terminal to run compile:
`$ make`

Once made, type the following command to start the server:
`$ ./dbserver`

In a seperate terminal, you can test the functionality of this program by typing:
`$ ./dbtest`

To run a series of automated tests, type the following command:
`$ ./testing.sh`


**Summary**

The proj.2 files was modified to create this project.
The modification includes the definition of the following structs:
- dbEntry - a database entry
- stats_t - used to keep track of the various statistics of the sever operations
- queue_t - used to keep track of various operations requested to be performed
- node_t - used to implement the queue_t

The dbserver.c and testing.sh files were created to implement this project.
- dbserver.c - the implementation of the server
- testing.sh - automated testing to verify the server implementation works properly
