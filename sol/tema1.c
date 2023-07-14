#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "genetic_algorithm.h"

int main(int argc, char *argv[]) {
	// array with all the objects that can be placed in the sack
	sack_object *objects = NULL;

	// number of objects
	int object_count = 0;

	// maximum weight that can be carried in the sack
	int sack_capacity = 0;

	// number of generations
	int generations_count = 0;

	// number of cores
	int no_cores = 0;


	if (!read_input(&objects, &object_count, &sack_capacity, &generations_count, &no_cores, argc, argv)) {
		return 0;
	}

	thread_info *arguments;
	pthread_t* threads;
	int r;
	void *status;
	pthread_barrier_t barrier;

	r = pthread_barrier_init(&barrier, NULL, no_cores);
	if (r) {
		printf("Eroare la crearea barierei");
		return 1;
	}


	arguments = (thread_info *) malloc(no_cores * sizeof(thread_info));
	if (arguments == NULL) {
		printf("Eroare la alocarea arguments in tema1.c");
		return 1;		
	}

	threads = (pthread_t*) malloc(no_cores * sizeof(pthread_t));
	if (threads == NULL) {
		printf("Eroare la alocarea threads in tema1.c");
		return 1;
	}
	
	individual *current_generation = (individual*) calloc(object_count, sizeof(individual));
	if (current_generation == NULL) {
		printf("Eroare la alocarea current_generation in tema1.c");
		return 1;
	}

	individual *next_generation = (individual*) calloc(object_count, sizeof(individual));
	if (next_generation == NULL) {
		printf("Eroare la alocarea next_generation in tema1.c");
		return 1;
	}


	for (int i = 0; i < no_cores; i++) {
		arguments[i].id = i;
		arguments[i].no_cores = no_cores;
		arguments[i].sack_capacity = sack_capacity;
		arguments[i].generations_count = generations_count;

		arguments[i].object_count = object_count;
		arguments[i].objects = objects;

		arguments[i].current_generation = &current_generation;
		arguments[i].next_generation = &next_generation;

		arguments[i].barrier = &barrier;

		r = pthread_create(&threads[i], NULL, f, &arguments[i]);

		if (r) {
			printf("Eroare la crearea thread-ului %d\n", i);
			exit(-1);
		}
	}

	for (int i = 0; i < no_cores; i++) {
		r = pthread_join(threads[i], &status);

		if (r) {
			printf("Eroare la asteptarea thread-ului %d\n", i);
			exit(-1);
		}
	}

	free(threads);
	free(arguments);

	free(objects);
	r = pthread_barrier_destroy(&barrier);
	if (r) {
		printf("Eroare la distrugerea barierei");
		return 1;
	}


	return 0;
}
