#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "proj2.h"

/* Functions for the main thread */

/**
 * Prints statistics
 */
void stats() {
	printf("getting stats...\n");
}

/** 
 * Terminates the server
 */
void quit() {
	printf("quitting...\n");
}

/* Variable declarations that are necessary for the worker threads */

// Buffer to store data from request or data read from file.
char buf[4096];

// Defined struct that stores name and status for a file or index.
typedef struct entry {
	char name[31];
	int status;
}dbEntry;

//The DB itself!
dbEntry DB[200];

/* Utility Functions */

/**
 * Reads data from a file 
 * 
 * @param filename char pointer storing name of the file to be read from 
 * 					the file system.
 */
int read_file(char* filename) {
	printf("Reading...\n");
	int fd = open(filename, O_RDONLY);
	int size = read(fd, buf, sizeof(buf));
	printf("size %d\n", size);
	printf("buf %s\n", buf);
	close(fd);
	return size;
}

/** 
 * Writes data to a file
 * 
 * @param filename char pointer storing name of the file to be written to 
 * 					the file system.
 */
void write_file(char* filename) {
	printf("Writing...\n");
	int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0777);
	if(fd < 0)
		perror("can't open"), exit(0);
	write(fd, buf, strlen(buf));
	close(fd);
}

/**
 * Checking to see if there is a dbEntry with the key, if so, return idx
 * Otherwise return -1
 * 
 * @param name Character pointer that stores te name of the entry.
 */
int findIdxByName(char *name) {
	for(int i = 0; i < 200; i++) {
		if(strcmp(DB[i].name, name) == 0) {
			return i;
		}
	}
	return -1;
}

/**
 * Finding an "open" spot on the database that the key can occupy and the
 * value be stored.
 */
int findOpenIdx() {
	for(int i = 0; i < 200; i++) {
		if(DB[i].status == 0) {
			return i;
		}
	}
	return -1;
}

/* Functions for the listener and worker threads */

/**
 * Reads a request from a TCP connection and performs the requested action
 * (write/read/delete).
 * 
 * @param sock_fd Socket file descriptor that we read requests from and
 * 					respond through.
 */
void handle_work(int sock_fd) {
	struct request rq;
	read(sock_fd, &rq, sizeof(rq));
	printf("Operation: %c\n", rq.op_status);
	printf("Name: %s\n", rq.name);
	printf("Length: %s\n", rq.len);
	char filename[32];
	if(rq.op_status == 'W') {
		int dbIdx = findIdxByName(rq.name);
		if(dbIdx == -1) dbIdx = findOpenIdx();
		if(dbIdx == -1) perror("No available space!"), exit(1);
		usleep(random() % 10000);
		// lock here?
		memset(buf, 0, sizeof(buf));
		read(sock_fd, &buf, atoi(rq.len));
		printf("Data: %s\n\n", buf);
		sprintf(filename, "./tmp/data.%d", dbIdx);
		write_file(filename);
		strcpy(DB[dbIdx].name, rq.name);
		DB[dbIdx].status = 1;
		// unlock here?
		rq.op_status = 'K';
		write(sock_fd, &rq, sizeof(rq));
	} else if(rq.op_status == 'R') {
		int idx = findIdxByName(rq.name);
		if(idx == -1) {
			rq.op_status = 'X';
			write(sock_fd, &rq, sizeof(rq));
			close(sock_fd);
			return;
		}
		sprintf(filename, "./tmp/data.%d", idx);
		int sz = read_file(filename);
		rq.op_status = 'K';
		sprintf(rq.len, "%7d", sz);
		write(sock_fd, &rq, sizeof(rq));
		write(sock_fd, &buf, sz);
	} else if (rq.op_status == 'D') {
		// Do Delete
		// Elegantly handle errors
	}
	close(sock_fd);
}

/* Queueing Functions */

/**
 * Allocates a work item and puts it on a queue.
 * 
 * @param sock_fd Socket file descriptor to queue a work item to.
 */
void queue_work(queue_t *queue, void *sock_fd) {
	printf("queueing work...\n");

	// initialize node
	node_t *node = malloc(sizeof(node_t));
	node->data = sock_fd;
	node->next = NULL;

	//NEED MUTEX AND CONDITION VARIABLE

	// if queue is empty
        if (queue->first == NULL) {
                // this node will be the first and last
                queue->first = node;
                queue->last = node;
        } else { // if queue is not empty
                queue->last->next = node; // add this node to the end of the queue
                queue->last = node; // tell the queue that this is now the last node
        }

        // we added a node so the queue grew by one
        queue->size++;
}

/**
 * Removes and returns the element at the front of the queue.
 *
 * @param queue The queue to modify.
 * @return The data removed from the front of the queue, or NULL if the queue is empty.
 */
void *dequeue(queue_t *queue) {

        // if there is nothing in the queue
        if (queue->size == 0 || queue == NULL) {
                return NULL;
        }

        // get the data of the first node (which we will return)
        node_t *first = queue->first;
        void *data = first->data;

        // update the state of queue struct
        queue->first = first->next;
        queue->size --;

        // free allocated mememory of the dequeued node
        free(first);

        return data;
}

/**
 * Initializes the queue.
 *
 * @param queue Pointer to the queue to initialize.
 * @param print_func Function to print queue data.
 */
void initialize_queue(queue_t *queue) {
    queue->first = NULL;
    queue->last = NULL;
    queue->size = 0;
}

/**
 * Gets an item from the queue and frees the work record.
 */
int get_work() {
	printf("getting work item...\n");
	return -1;
}

/**
 * Creates and binds the listening socket.
 * Loops accepting connections and calling handle_work().
 */
void listener() {
	printf("listening...\n");

	// given by part 2 of assignment
	int port = 5000;
	// create a "listening" TCP socket which will listen for incoming connections
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	// the address we'll bind to
	struct sockaddr_in addr = {.sin_family = AF_INET,
					.sin_port = htons(port),
					.sin_addr.s_addr = 0};
	// bind the listening socket to port 5000
	if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
		perror("can't bind"), exit(1);
	// tell OS to start listening on it
    	if (listen(sock, 2) < 0)
		perror("listen"), exit(1);

	// block until we get a new connection
	while (1) {
		int fd = accept(sock, NULL, NULL);
		handle_work(fd);
	}
}

/**
 * Our main function!
 */
int main(void) {
	// deletes all files from previous program run
	system("rm -f ./tmp/data.*");

	// initialize the work queue
	queue_t *queue = malloc(sizeof(queue_t) * sizeof(DB));
	initialize_queue(queue);
	

	for(int i = 0; i < 200; i++) DB[i].status = 0;
	listener();

	free(queue);
	return 0;
}
