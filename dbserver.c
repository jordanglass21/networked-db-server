#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "proj2.h"

/* Functions for the main thread */

/* Prints statistics */
void stats() {
	printf("getting stats...\n");
}

/* Terminates the server */
void quit() {
	printf("quitting...\n");
}

/* Functions for the listener and worker threads */

/* Creates and binds the listening socket.
 * Loops accepting connections and calling handle_work().
 */
void listener() {
	printf("listening...\n");

	// given by part 2 of assignment
	int port = 5000;
	// create a "listening" TCP socket which will listen for incoming connections
	int sock = socket(AF-INET, SOCK_STREAM, 0);
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
	}
	
	
}

/* Reads a request from a TCP connection and performs the requested action
 * (write/read/delete).
 */
void handle_work(int sock_fd) {
	printf("handling work...\n");
}

/* Allocates a work item and puts it on a queue, and int sock_id = get_work(),
 *  which gets an item from the queue and frees the work record.
 */
void queue_work(int sock_fd) {
	printf("queueing work...\n");
}


int main(void) {
	stats();
	quit();
	return 0;
}
