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
	print("queueing work...\n");
}


int main(void) {
	stats();
	quit();
	return 0;
}
