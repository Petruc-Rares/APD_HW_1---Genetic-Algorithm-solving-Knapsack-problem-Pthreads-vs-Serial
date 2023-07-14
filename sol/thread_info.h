#ifndef THREAD_INFO_H
#define THREAD_INFO_H

// structure for an object to be placed in the sack, with its weight and profit
typedef struct _thread_info
{
    int id;
	int object_count;
	int no_cores;
    const sack_object *objects;
    int generations_count;
    int sack_capacity;
    individual **current_generation;
    individual **next_generation;
    pthread_barrier_t *barrier;
} thread_info;

#endif