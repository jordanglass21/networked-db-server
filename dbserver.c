#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>

#include "proj2.h"


/* Variable declarations that are necessary for the worker threads */

// global memory region to store the port that the server is running on
int *PORT;

//The DB itself!
dbEntry DB[200];

// struct to keep track of stats
stats_t *STATS;

// global memory region to store the socket fd for easier closing upon quit.
int *SOCK_FD;

// The work queue that worker grab requests from.
queue_t *queue;

// The POSIX synchronization, locks and condtional variables.
pthread_mutex_t q_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t d_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t s_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t q_cond = PTHREAD_COND_INITIALIZER;

/* Functions for the main thread */

/**
 * Prints statistics
 */
void stats() {
		pthread_mutex_lock(&s_lock);
        printf("getting stats...\n");
		printf("Port: %d\n", *PORT);
        printf("Number of objects in table: %d\n", STATS->table_count);
        printf("Number of read requests: %d\n", STATS->read_count);
        printf("Number of write requests: %d\n", STATS->write_count);
        printf("Number of delete requests: %d\n", STATS->delete_count);
        printf("Number of requests queued waiting for worker threads: %d\n", STATS->requests_queued);
        printf("Number of failed requests: %d\n", STATS->failed_count);
		pthread_mutex_unlock(&s_lock);
}


/** 
 * Terminates the server
 */
void quit(int *sock_fd) {
	close(*sock_fd);
	free(sock_fd);
	exit(0);
}

/* Utility Functions */

/**
 * Reads data from a file 
 * 
 * @param filename char pointer storing name of the file to be read from 
 * 					the file system.
 */
int read_file(char* filename, char* buf, int sz) {
	int fd = open(filename, O_RDONLY);
	int size = read(fd, buf, sz);
	close(fd);
	return size;
}

/** 
 * Writes data to a file
 * 
 * @param filename char pointer storing name of the file to be written to 
 * 					the file system.
 */
void write_file(char* filename, char* buf) {
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
 * value be stored. returns -1 if no open space found.
 */
int findOpenIdx() {
	for(int i = 0; i < 200; i++) {
		if(DB[i].status == 0) {
			return i;
		}
	}
	return -1;
}

/**
 * Computes occupied entry space
 */
int findObjInTable() {
	int tally = 0;
	for(int i = 0; i < 200; i++) {
		if(DB[i].status == 1) {
			tally++;
		}
	}
	return tally;
}

/* Functions for the listener and worker threads */

/**
 * Function that handles the write operation for a request
 * 
 * @param rq 		Request struct that stores information about a request.
 * @param sock_fd 	Socket file descriptor to read data from/write data to.
 * @param filename	Filename to write data from request to for DB impl.
 */
void handle_write(struct request rq, int sock_fd, char* filename) {
	// Buffer to store the data to write to database
	char buf[4096];
	// Flush the buffer
	memset(buf, 0, sizeof(buf));
	// Read the data from the socket into the buffer
	read(sock_fd, buf, atoi(rq.len));
	// Acquire the lock
	pthread_mutex_lock(&d_lock);
	// Try to find the entry by name
	int idx = findIdxByName(rq.name);
	// Did not find entry by name, trying to find open spot
	if(idx == -1) idx = findOpenIdx();
	// No open spot, write response and return
	if(idx == -1) {
		perror("WRITE: INSUFFICIENT SPACE FOR WRITE");
		rq.op_status = 'X';
		write(sock_fd, &rq, sizeof(rq));
		// Acquire lock for stats object
		pthread_mutex_lock(&s_lock);
		// update stats
		STATS->write_count++;
		STATS->failed_count++;
		// unlock stats and database lock
		pthread_mutex_unlock(&s_lock);
		pthread_mutex_unlock(&d_lock);
		return;
	}
	// Spot found by name, resource busy. write response and return
	if(DB[idx].status == 2) {
		perror("WRITE: Resource Busy");
		rq.op_status = 'X';
		write(sock_fd, &rq, sizeof(rq));
		// Acquire stats lock
		pthread_mutex_lock(&s_lock);
		//update stats
		STATS->write_count++;
		STATS->failed_count++;
		// release stats and database lock
		pthread_mutex_unlock(&s_lock);
		pthread_mutex_unlock(&d_lock);
		return;
	}
	// set status to busy
	DB[idx].status = 2;
	// build filename
	sprintf(filename, "./tmp/data.%d", idx);
	// write key to database
	strcpy(DB[idx].name, rq.name);
	// release lock for possible resource busy condtion
	pthread_mutex_unlock(&d_lock);
	
	// Do a sleep!
	usleep(random() % 10000);

	// Acquire the lock again.
	pthread_mutex_lock(&d_lock);
	// write data from buffer to file
	write_file(filename, buf);
	// update entry status
	DB[idx].status = 1;

	// Lock and update stats then release lock
	pthread_mutex_lock(&s_lock);
	STATS->write_count++;
	STATS->table_count = findObjInTable();
	pthread_mutex_unlock(&s_lock);
	
	// Set response status in object and write to socket
	rq.op_status = 'K';
	write(sock_fd, &rq, sizeof(rq));
	
	// release database lock
	pthread_mutex_unlock(&d_lock);
}

/**
 * Function that handles the read operation for a request
 * 
 * @param rq 		Request struct that stores information about a request.
 * @param sock_fd 	Socket file descriptor to read data from/write data to.
 * @param filename	Filename to write data from request to for DB impl.
 */

 void handle_read(struct request rq, int sock_fd, char* filename) {
	// buffer
	char buf[4096];
	// Acquire database lock
	pthread_mutex_lock(&d_lock);
	// Try to find entry by name, -1 if not found
	int idx = findIdxByName(rq.name);
	// Not found, set status and write to socket, update stats with locks
	if(idx == -1) {
		perror("READ: NO KEY FOUND WITH SPECIFIED VALUE");
		rq.op_status = 'X';
		write(sock_fd, &rq, sizeof(rq));
		pthread_mutex_lock(&s_lock);
		STATS->read_count++;
		STATS->failed_count++;
		pthread_mutex_unlock(&s_lock);
		pthread_mutex_unlock(&d_lock);
		return;
	}
	// Resource busy, set status and write to socket, update stats with locks
	if(DB[idx].status == 2) {
		perror("READ: Resource Busy");
		rq.op_status = 'X';
		write(sock_fd, &rq, sizeof(rq));
		pthread_mutex_lock(&s_lock);
		STATS->read_count++;
		STATS->failed_count++;
		pthread_mutex_unlock(&s_lock);
		pthread_mutex_unlock(&d_lock);
		return;
	}
	// build filename to look at
	sprintf(filename, "./tmp/data.%d", idx);
	// read data from file.
	int sz = read_file(filename, buf, sizeof(buf));
	
	//update stats with locks
	pthread_mutex_lock(&s_lock);
	STATS->read_count++;
	pthread_mutex_unlock(&s_lock);
	
	// set status and len of data, write to socket
	rq.op_status = 'K';
	sprintf(rq.len, "%7d", sz);
	write(sock_fd, &rq, sizeof(rq));
	// write data to socket
	write(sock_fd, &buf, sz);
	
	// release lock
	pthread_mutex_unlock(&d_lock);
 }

 /**
 * Function that handles the delete operation for a request
 * 
 * @param rq 		Request struct that stores information about a request.
 * @param sock_fd 	Socket file descriptor to read data from/write data to.
 * @param filename	Filename to write data from request to for DB impl.
 */
void handle_delete(struct request rq, int sock_fd, char* filename) {
	// Acquire lock and search for entry with name in database
	pthread_mutex_lock(&d_lock);
	int idx = findIdxByName(rq.name);
	if(idx == -1) {
		// no entry found with name, returning failure
		perror("DELETE: NO KEY FOUND WITH SPECIFIED VALUE");
		// set status, write to socket
		rq.op_status = 'X';
		write(sock_fd, &rq, sizeof(rq));
		// lock, update, unlock for stats object
		pthread_mutex_lock(&s_lock);
		STATS->delete_count++;
		STATS->failed_count++;
		pthread_mutex_unlock(&s_lock);
		// release database lock
		pthread_mutex_unlock(&d_lock);
		return;
	}
	if(DB[idx].status == 2) {
		// resource busy
		perror("DELETE: Resource Busy");
		// set status, write to socket
		rq.op_status = 'X';
		write(sock_fd, &rq, sizeof(rq));
		// lock, update, unlock for stats object
		pthread_mutex_lock(&s_lock);
		STATS->delete_count++;
		STATS->failed_count++;
		pthread_mutex_unlock(&s_lock);
		// release database lock
		pthread_mutex_unlock(&d_lock);
		return;
	}
	// set busy status code
	DB[idx].status = 2;
	// flush database entry's name
	memset(DB[idx].name, 0, 31);
	// set status to open for new entry
	DB[idx].status = 0;
	
	// build filename and delete file.
	sprintf(filename, "./tmp/data.%d", idx);
	unlink(filename);
	
	//update stats
	pthread_mutex_lock(&s_lock);
	STATS->delete_count++;
	STATS->table_count = findObjInTable();
	pthread_mutex_unlock(&s_lock);
	
	// stat status and write to socket.
	rq.op_status = 'K';
	write(sock_fd, &rq, sizeof(rq));
	
	// release database lock
	pthread_mutex_unlock(&d_lock);
}

/**
 * Reads a request from a TCP connection and performs the requested action
 * (write/read/delete).
 * 
 * @param sock_fd Socket file descriptor that we read requests from and
 * 					respond through.
 */
void handle_work(int sock_fd) {
	struct request rq;
	// read request from socket
	read(sock_fd, &rq, sizeof(rq));
	// buffer for file name
	char filename[32];
	// based on 'op', decide what code to run
	if(rq.op_status == 'W') {
		// write to database
		handle_write(rq, sock_fd, filename);
	} else if(rq.op_status == 'R') {
		// read from database
		handle_read(rq, sock_fd, filename);
	} else if (rq.op_status == 'D') {
		// delete from database
		handle_delete(rq, sock_fd, filename);
	}
	// close the request socket
	close(sock_fd);
}

/* Queueing Functions */

/**
 * Allocates a work item and puts it on a queue.
 * 
 * @param sock_fd Socket file descriptor to queue a work item to.
 */
void queue_work(queue_t *queue, int sock_fd) {
	// initialize node
	node_t *node = malloc(sizeof(node_t));
	node->fd = sock_fd;
	node->next = NULL;

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
	pthread_mutex_lock(&s_lock);
	STATS->requests_queued = queue->size;
	pthread_mutex_unlock(&s_lock);
}

/**
 * Gets an item from the queue and frees the work record.
 *
 * @param queue The queue to modify.
 * @return The data removed from the front of the queue, or NULL if the queue is empty.
 */
int get_work() {
	// empty queue? Return -1
	if (queue->size == 0 || queue == NULL) {
			return -1;
	}

	// get the data of the first node (which we will return)
	node_t *first = queue->first;
	int fd = first->fd;
	
	// update the state of queue struct
	queue->first = first->next;
	queue->size --;

	// free allocated mememory of the dequeued node
	free(first);

	//printf("fd: %d\n", fd);
	pthread_mutex_lock(&s_lock);
	STATS->requests_queued = queue->size;
	pthread_mutex_unlock(&s_lock);
	return fd;
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
 * Creates and binds the listening socket.
 * Loops accepting connections and calling handle_work().
 */
void listener() {
	// given by part 2 of assignment
	int port = *PORT;
	// create a "listening" TCP socket which will listen for incoming connections
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	*SOCK_FD = sock;
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
		// accept new connection
		int fd = accept(sock, NULL, NULL);
		// acquire lock for queue as we are adding to it
		pthread_mutex_lock(&q_lock);
		// push request to queue
		queue_work(queue, fd);
		// release queue lock
		pthread_mutex_unlock(&q_lock);
		// signal to workers that there is work to do
		pthread_cond_signal(&q_cond);
	}
}

/**
 * Function that defines what the worker thread has to do
 */
void worker(void *arg) {
	// infinite loop
	while(1) {
		// Acquire lock for queue as we are popping work from the queue
		pthread_mutex_lock(&q_lock);
		/* If there is no work to do, do a conditional wait
		*	will be woken up by the listener thread that queues if 
		*	there is work to do
		*/
		if(queue->size == 0) {
			pthread_cond_wait(&q_cond, &q_lock);
		}
		// grab the work from the queue
		int work_fd = get_work(queue);
		// Release lock for queue
		pthread_mutex_unlock(&q_lock);
		// go process this work request
		handle_work(work_fd);
	}
}

/**
 * Setup function that take care of necessary allocations and data structure
 * setting up
 * 
 * @param argc Argument count
 * @param argv Actual arugments
 */
void setup(int argc, char **argv) {
	// deletes all files from previous program run
	system("rm -f ./tmp/data.*");

	// initialize the work queue
	queue = malloc(sizeof(queue_t) * sizeof(DB));
	initialize_queue(queue);

	// initialize the stats struct
	STATS = calloc(1, sizeof(stats_t));

	// initialize the socket fd storage memory
	SOCK_FD = calloc(1, sizeof(int));

	// initialize port storage memory
	PORT = calloc(1, sizeof(int));
	
	// little decision mechanism to determine which port to run on.
	int port = 5000;
	if (argc == 2) {
		port = atoi(argv[1]);
	}
	*PORT = port;

	// zero out the db
	for(int i = 0; i < 200; i++) DB[i].status = 0;
}

/**
 * Controller function that is like the CPU to jump start 
 * 	this entire application
 * 
 * @param argc Argument count
 * @param argv Actual arugments
 */
void controller(int argc, char **argv) {
    // call setup for global memory allocation and data structure setting up
	setup(argc, argv);

	// listener thread variable
	pthread_t listener_t;
	// four worker thread variable in array
	pthread_t w_threads[4];
	
	// create listener thread for accepting connections
	pthread_create(&listener_t, NULL, (void *)listener, NULL);
	// create four worker threads to handle the work in the queue if any
	for(int i = 0; i < 4; i++) {
		pthread_create(&w_threads[i], NULL, (void *)worker, NULL);
	}
	// Line buf to interactive main thread
	char line[128];
	// While fgets is not null
	while (fgets(line, sizeof(line), stdin) != NULL) {
		// buf variable for token (word)
		char word[8];
		// extract word from line
		sscanf(line, "%7s", word);
		// if word is 'quit'
		if (strcmp(word, "quit") == 0) {
			// Acquire locks for queue and stats
			pthread_mutex_lock(&q_lock);
			pthread_mutex_lock(&s_lock);
			// Free the queue and stats structs
			free(STATS);
			free(queue);
			// Release the locks
			pthread_mutex_unlock(&q_lock);
			pthread_mutex_unlock(&s_lock);
			// free the port
			free(PORT);
			// call quit function
			quit(SOCK_FD);
		} else if (strcmp(word, "stats") == 0) {
			// call stats if the word is 'stats'
			stats();
		} else {
			// command not recognized, the default
			printf("Command not recognized\n");
		}
	}
}

/**
 * Our main function!
 */
int main(int argc, char **argv) {
	// jump start the controller
	controller(argc, argv);	
	return 0;
}
