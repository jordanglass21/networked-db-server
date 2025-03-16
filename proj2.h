/*
 * file:        proj2.h
 * description: request/response header for project 2
 */
#ifndef __PROJ2_H__
#define __PROJ2_H__

struct request {
    char op_status;             /* R/W/D, K/X */
    char name[31];              /* null-padded, max strlen = 30 */
    char len[8];                /* text, decimal, null-padded */
};

/**
 * Structure representing a node in the linked queue.
 */
typedef struct node_t {
        void *data; // file descriptor -  need to malloc this
        struct node_t *next;
} node_t;

/**
 * Structure representing a FIFO queue.
 */
typedef struct queue_t {
        node_t *first;
        node_t *last;
        int size;
} queue_t;

/**
 * Initializes the queue.
 *
 * @param queue Pointer to the queue to initialize.
 */
void initialize_queue(queue_t *queue);


/**
 * Adds an element to the end of a queue.
 *
 * @param queue The queue to modify.
 * @param element The element to add.
 */
void queue_work(queue_t *queue, void *element);

/**
 * Removes and returns the element at the front of the queue.
 *
 * @param queue The queue to modify.
 * @return The element removed from the front.
 */
void *dequeue_work(queue_t *queue);

/**
 * Structure to store the statistics of the program;
 */
typedef struct stats_t {
	int table_count;
	int read_count;
	int write_count;
	int delete_count;
	int requests_queued;
	int failed_count;
} stats_t;

#endif
