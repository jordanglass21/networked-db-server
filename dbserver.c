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

// Buffer to store data from request or data read from file.
char buf[4096];

int *PORT;

// Defined struct that stores name and status for a file or index.
typedef struct entry {
        char name[31];
        int status;
}dbEntry;

//The DB itself!
dbEntry DB[200];

// struct to keep track of stats
stats_t *STATS;

int *SOCK_FD;

queue_t *queue;

pthread_mutex_t q_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t d_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t q_cond = PTHREAD_COND_INITIALIZER;

/* Functions for the main thread */

/**
 * Prints statistics
 */
void stats() {
        printf("getting stats...\n");
	printf("Port: %d\n", *PORT);
        printf("Number of objects in table: %d\n", STATS->table_count);
        printf("Number of read requests: %d\n", STATS->read_count);
        printf("Number of write requests: %d\n", STATS->write_count);
        printf("Number of delete requests: %d\n", STATS->delete_count);
        printf("Number of requests queued waiting for worker threads: %d\n", STATS->requests_queued);
        printf("Number of failed requests: %d\n", STATS->failed_count);
}


/** 
 * Terminates the server
 */
void quit(int *sock_fd) {
	printf("quitting...\n");
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
 * Function that handles the write operation for a request
 * 
 * @param rq 		Request struct that stores information about a request.
 * @param sock_fd 	Socket file descriptor to read data from/write data to.
 * @param filename	Filename to write data from request to for DB impl.
 */
void handle_write(struct request rq, int sock_fd, char* filename) {
	pthread_mutex_lock(&d_lock);
	int dbIdx = findIdxByName(rq.name);
	if(dbIdx == -1) dbIdx = findOpenIdx();
	if(dbIdx == -1) {
		perror("WRITE: INSUFFICIENT SPACE FOR WRITE");
		rq.op_status = 'X';
		write(sock_fd, &rq, sizeof(rq));
		STATS->failed_count++;
		return;
	}
	if(DB[dbIdx].status == 2) {
		perror("WRITE: Resource Busy");
		rq.op_status = 'X';
		write(sock_fd, &rq, sizeof(rq));
		STATS->failed_count++;
		return;
	}
	DB[dbIdx].status = 2;
	pthread_mutex_unlock(&d_lock);

	usleep(random() % 10000);
	memset(buf, 0, sizeof(buf));
	read(sock_fd, &buf, atoi(rq.len));
	printf("Data: %s\n\n", buf);
	sprintf(filename, "./tmp/data.%d", dbIdx);
	
	pthread_mutex_lock(&d_lock);
	write_file(filename);
	strcpy(DB[dbIdx].name, rq.name);
	DB[dbIdx].status = 1;
	pthread_mutex_unlock(&d_lock);

	rq.op_status = 'K';
	write(sock_fd, &rq, sizeof(rq));
	
	//update stats
	STATS->write_count++;
	STATS->table_count++;
}

/**
 * Function that handles the read operation for a request
 * 
 * @param rq 		Request struct that stores information about a request.
 * @param sock_fd 	Socket file descriptor to read data from/write data to.
 * @param filename	Filename to write data from request to for DB impl.
 */

 void handle_read(struct request rq, int sock_fd, char* filename) {
	int idx = findIdxByName(rq.name);
	if(idx == -1) {
		perror("READ: NO KEY FOUND WITH SPECIFIED VALUE");
		rq.op_status = 'X';
		write(sock_fd, &rq, sizeof(rq));
		STATS->failed_count++;
		return;
	}
	sprintf(filename, "./tmp/data.%d", idx);
	int sz = read_file(filename);
	rq.op_status = 'K';
	sprintf(rq.len, "%7d", sz);
	write(sock_fd, &rq, sizeof(rq));
	write(sock_fd, &buf, sz);

	//update stats
	STATS->read_count++;
 }

 /**
 * Function that handles the delete operation for a request
 * 
 * @param rq 		Request struct that stores information about a request.
 * @param sock_fd 	Socket file descriptor to read data from/write data to.
 * @param filename	Filename to write data from request to for DB impl.
 */
void handle_delete(struct request rq, int sock_fd, char* filename) {
	int idx = findIdxByName(rq.name);
	if(idx == -1) {
		// no entry found with name, returning failure
		perror("DELETE: NO KEY FOUND WITH SPECIFIED VALUE");
		rq.op_status = 'X';
		write(sock_fd, &rq, sizeof(rq));
		STATS->failed_count++;
		return;
	}
	DB[idx].status = 0;
	memset(DB[idx].name, 0, 31);
	rq.op_status = 'K';
	write(sock_fd, &rq, sizeof(rq));

	//update stats
	STATS->delete_count++;
	STATS->table_count--;
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
	read(sock_fd, &rq, sizeof(rq));
	printf("Operation: %c\n", rq.op_status);
	printf("Name: %s\n", rq.name);
	printf("Length: %s\n", rq.len);
	char filename[32];
	if(rq.op_status == 'W') {
		handle_write(rq, sock_fd, filename);
	} else if(rq.op_status == 'R') {
		pthread_mutex_lock(&d_lock);
		handle_read(rq, sock_fd, filename);
		pthread_mutex_unlock(&d_lock);
	} else if (rq.op_status == 'D') {
		pthread_mutex_lock(&d_lock);
		handle_delete(rq, sock_fd, filename);
		pthread_mutex_unlock(&d_lock);
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
		STATS->requests_queued = queue->size;
}

/**
 * Gets an item from the queue and frees the work record.
 *
 * @param queue The queue to modify.
 * @return The data removed from the front of the queue, or NULL if the queue is empty.
 */
int *get_work() {

	printf("getting work item...\n");
        // if there is nothing in the queue
        if (queue->size == 0 || queue == NULL) {
                return NULL;
        }

        // get the data of the first node (which we will return)
        node_t *first = queue->first;
        int *data = (int *) first->data;
        
	// update the state of queue struct
        queue->first = first->next;
        queue->size --;

        // free allocated mememory of the dequeued node
        free(first);

		printf("data: %d\n",*data);
		STATS->requests_queued = queue->size;
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
 * Creates and binds the listening socket.
 * Loops accepting connections and calling handle_work().
 */
void listener() {
	printf("listening...\n");

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
		int fd = accept(sock, NULL, NULL);
		pthread_mutex_lock(&q_lock);
		queue_work(queue, &fd);
		pthread_mutex_unlock(&q_lock);
		pthread_cond_signal(&q_cond);
	}
}
void worker(void *arg) {
	while(1) {
		pthread_mutex_lock(&q_lock);
		if(queue->size == 0) {
			pthread_cond_wait(&q_cond, &q_lock);
		}
		int work_fd = *(get_work(queue));
		printf("work_fd: %d\n", work_fd);
		pthread_mutex_unlock(&q_lock);
		handle_work(work_fd);
	}
}
/**
 * Our main function!
 */
int main(int argc, char **argv) {
	// deletes all files from previous program run
	system("rm -f ./tmp/data.*");

	// initialize the work queue
	queue = malloc(sizeof(queue_t) * sizeof(DB));
	initialize_queue(queue);

	//initialize the stats struct
	STATS = calloc(1, sizeof(stats_t));
	
	SOCK_FD = calloc(1, sizeof(int));

	PORT = calloc(1, sizeof(int));
	int port = 5000;
	if (argc == 2) {
		port = atoi(argv[1]);
	}
	*PORT = port;


	for(int i = 0; i < 200; i++) DB[i].status = 0;
	pthread_t listener_t;
	pthread_t w1;
	int w1_id = 1;
	pthread_create(&listener_t, NULL, (void *)listener, NULL);
	pthread_create(&w1, NULL, (void *)worker, (void *)&w1_id);
	char line[128];
	while (fgets(line, sizeof(line), stdin) != NULL) {
		char word[8];
		sscanf(line, "%7s", word);
		if (strcmp(word, "quit") == 0) {
			free(STATS);
			free(queue);
			free(PORT);
			quit(SOCK_FD);
		} else if (strcmp(word, "stats") == 0) {
			stats();
		} else {
			printf("Command not recognized\n");
		}
	}
	return 0;
}
