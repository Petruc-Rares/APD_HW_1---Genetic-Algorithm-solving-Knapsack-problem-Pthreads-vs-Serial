#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "genetic_algorithm.h"

int read_input(sack_object **objects, int *object_count, int *sack_capacity, int *generations_count, int *no_cores,
				int argc, char *argv[])
{
	FILE *fp;

	if (argc < 4) {
		fprintf(stderr, "Usage:\n\t./tema1 in_file generations_count no_cores \n");
		return 0;
	}

	fp = fopen(argv[1], "r");
	if (fp == NULL) {
		return 0;
	}

	if (fscanf(fp, "%d %d", object_count, sack_capacity) < 2) {
		fclose(fp);
		return 0;
	}

	if (*object_count % 10) {
		fclose(fp);
		return 0;
	}

	sack_object *tmp_objects = (sack_object *) calloc(*object_count, sizeof(sack_object));

	for (int i = 0; i < *object_count; ++i) {
		if (fscanf(fp, "%d %d", &tmp_objects[i].profit, &tmp_objects[i].weight) < 2) {
			free(objects);
			fclose(fp);
			return 0;
		}
	}

	fclose(fp);

	*generations_count = (int) strtol(argv[2], NULL, 10);
	*no_cores = (int) strtol(argv[3], NULL, 10);
	
	if ((*generations_count == 0) || (*no_cores == 0)) {
		free(tmp_objects);

		return 0;
	}

	*objects = tmp_objects;

	return 1;
}

void print_objects(const sack_object *objects, int object_count)
{
	for (int i = 0; i < object_count; ++i) {
		printf("%d %d\n", objects[i].weight, objects[i].profit);
	}
}

void print_generation(const individual *generation, int limit)
{
	for (int i = 0; i < limit; ++i) {
		for (int j = 0; j < generation[i].chromosome_length; ++j) {
			printf("%d ", generation[i].chromosomes[j]);
		}

		printf("\n%d - %d\n", i, generation[i].fitness);
	}
}

void print_best_fitness(const individual *generation)
{
	printf("%d\n", generation[0].fitness);
}

void compute_fitness_function(const sack_object *objects, individual *generation, int start, int end, int sack_capacity)
{
	int weight;
	int profit;

	for (int i = start; i < end; ++i) {
		weight = 0;
		profit = 0;

		for (int j = 0; j < generation[i].chromosome_length; ++j) {
			if (generation[i].chromosomes[j]) {
				weight += objects[j].weight;
				profit += objects[j].profit;
			}
		}

		generation[i].fitness = (weight <= sack_capacity) ? profit : 0;
	}
}

int cmpfunc(const void *a, const void *b)
{
	individual *first = (individual *) a;
	individual *second = (individual *) b;

	int res = second->fitness - first->fitness; // decreasing by fitness
	if (res == 0) {
		res = first->chromosome_count - second->chromosome_count; // increasing by number of objects in the sack
		if (res == 0) {
			return second->index - first->index;
		}
	}

	return res;
}

void mutate_bit_string_1(const individual *ind, int generation_index)
{
	int i, mutation_size;
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	if (ind->index % 2 == 0) {
		// for even-indexed individuals, mutate the first 40% chromosomes by a given step
		mutation_size = ind->chromosome_length * 4 / 10;
		for (i = 0; i < mutation_size; i += step) {
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	} else {
		// for even-indexed individuals, mutate the last 80% chromosomes by a given step
		mutation_size = ind->chromosome_length * 8 / 10;
		for (i = ind->chromosome_length - mutation_size; i < ind->chromosome_length; i += step) {
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	}
}

void mutate_bit_string_2(const individual *ind, int generation_index)
{
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	// mutate all chromosomes by a given step
	for (int i = 0; i < ind->chromosome_length; i += step) {
		ind->chromosomes[i] = 1 - ind->chromosomes[i];
	}
}

void crossover(individual *parent1, individual *child1, int generation_index)
{
	individual *parent2 = parent1 + 1;
	individual *child2 = child1 + 1;
	int count = 1 + generation_index % parent1->chromosome_length;

	memcpy(child1->chromosomes, parent1->chromosomes, count * sizeof(int));
	memcpy(child1->chromosomes + count, parent2->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));

	memcpy(child2->chromosomes, parent2->chromosomes, count * sizeof(int));
	memcpy(child2->chromosomes + count, parent1->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));
}

void copy_individual(const individual *from, const individual *to)
{
	memcpy(to->chromosomes, from->chromosomes, from->chromosome_length * sizeof(int));
}

void free_generation(individual *generation)
{
	int i;

	for (i = 0; i < generation->chromosome_length; ++i) {
		free(generation[i].chromosomes);
		generation[i].chromosomes = NULL;
		generation[i].fitness = 0;
	}
}

int min(int a, int b) {
	return (a < b)? a:b;
}

void *f(void *arg)
{
	individual *tmp = NULL;
	thread_info *th_info = (thread_info *) arg;

	int id = th_info->id;

	int object_count = th_info->object_count;
	int no_cores = th_info->no_cores;
	int generations_count = th_info->generations_count;
	int sack_capacity = th_info->sack_capacity;

	const sack_object *objects = th_info->objects;

	pthread_barrier_t *barrier = th_info->barrier;

	int start = id * (double) object_count / no_cores;
	int end = min((id + 1) * (double) object_count/ no_cores, object_count);

	int count = 0;
	int cursor = 0;

	individual *current_generation =  *th_info->current_generation;
	individual *next_generation =  *th_info->next_generation;

	for (int i = start; i < end; i++) {
		current_generation[i].fitness = 0;
		current_generation[i].chromosomes = (int*) calloc(object_count, sizeof(int));
		current_generation[i].chromosomes[i] = 1;
		current_generation[i].index = i;
		current_generation[i].chromosome_length = object_count;

		next_generation[i].fitness = 0;
		next_generation[i].chromosomes = (int*) calloc(object_count, sizeof(int));
		next_generation[i].index = i;
		next_generation[i].chromosome_length = object_count;
	}

	pthread_barrier_wait(barrier);

	for (int k = 0; k < generations_count; ++k) {
		cursor = 0;

		// compute fitness
		compute_fitness_function(objects, current_generation,
						 start, end, sack_capacity);

		// compute chromosomes count for each individual
		for (int i = start; i < end; i++) {
			current_generation[i].chromosome_count = 0;
			for (int j = 0; j < object_count; j++) {
				current_generation[i].chromosome_count += current_generation[i].chromosomes[j];
			}
		}
		
		// wait for all threads to compute chromosome_count and fitness cost
		pthread_barrier_wait(barrier);

		// sort by fitness
		if (id == 0) {
			qsort(current_generation, object_count, sizeof(individual), cmpfunc);
		}

		// wait for all threads to have the array of current_generation sorted
		pthread_barrier_wait(barrier);

		// keep first 30% children (elite children selection)		
		count = object_count * 3 / 10;
		int start_count = id * (double)count / no_cores;
		int end_count = min((id + 1) * count/ no_cores, count);


		for (int i = start_count; i < end_count; ++i) {
			copy_individual(current_generation + i, next_generation + i);
		}
		cursor = count;

		// mutate first 20% children with the first version of bit string mutation
		count = object_count * 2 / 10;
		
		start_count = id * (double)count / no_cores;
		end_count = min((id + 1) * count/ no_cores, count);

		for (int i = start_count; i < end_count; ++i) {
			copy_individual(current_generation + i, next_generation + cursor + i);
			mutate_bit_string_1(next_generation + cursor + i, k);
		}
		cursor += count;

		// mutate next 20% children with the second version of bit string mutation
		count = object_count * 2 / 10;
		start_count = id * (double)count / no_cores;
		end_count = min((id + 1) * count/ no_cores, count);

		for (int i = start_count; i < end_count; ++i) {
			copy_individual(current_generation + i + count, next_generation + cursor + i);
			mutate_bit_string_2(next_generation + cursor + i, k);
		}
		cursor += count;


		count = object_count * 3 / 10;

		if (count % 2 == 1) {
			copy_individual(current_generation + object_count - 1,
						 	next_generation + cursor + count - 1);
			count--;
		}

		start_count = id * (double)count / no_cores;
		end_count = min((id + 1) * count/ no_cores, count);

		start_count = (start_count % 2) ? start_count + 1 : start_count;
		end_count = (end_count % 2) ? end_count + 1:end_count;

		for (int i = start_count; i < end_count; i += 2) {
			crossover(current_generation + i, next_generation + cursor + i, k);
		}

		pthread_barrier_wait(barrier);

		tmp =  current_generation;
		current_generation =  next_generation;
		next_generation = tmp;

		for (int i = start_count; i < end_count; ++i) {
			current_generation[i].index = i;
		}

		if ((k % 5 == 0) &&  (id == 0)) {
			print_best_fitness(current_generation);
		}

		// wait the threads so one can not compute fitness cost
		// before printing best fitness cost from thread with id 0
		pthread_barrier_wait(barrier);
	}

	// last time compute fitness cost and chromosome count
	compute_fitness_function(objects, current_generation,
	 					start, end, sack_capacity);

	for (int i = start; i < end; i++) {
		current_generation[i].chromosome_count = 0;
		for (int j = 0; j < object_count; j++) {
			current_generation[i].chromosome_count += current_generation[i].chromosomes[j];
		}
	}

	pthread_barrier_wait(barrier);

	if (id == 0) {
		// final sort and final print of the best fitness from the generation
		qsort(current_generation, object_count, sizeof(individual), cmpfunc);
		print_best_fitness(current_generation);


		// and free resources
		free_generation(current_generation);
		free_generation(next_generation);


		free(current_generation);
		free(next_generation);
	}

	return NULL;
}