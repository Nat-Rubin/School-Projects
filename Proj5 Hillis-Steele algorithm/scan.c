#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>

typedef struct args_t {
	int size;
	int num_threads;
	int i;
	int j;
	int neighbors;
	int* x;
	int* y;
	pthread_t* threads;

} args_t;

args_t *args;

int threads_done = 0;

pthread_mutex_t mutex;
pthread_cond_t cond;

void* calculate_sums(void* arg) {


	pthread_mutex_lock(&mutex);
	int index = *(int*)arg;
	pthread_mutex_unlock(&mutex);

	

	pthread_mutex_lock(&mutex);

	int neighbors;
	int size = args->size;
	int* x = args->x;
	int* y = args->y;
	int num_threads = args->num_threads;

	pthread_mutex_unlock(&mutex);

	for (int i = 0; i < logb(size); i++) {

		pthread_mutex_lock(&mutex);
		neighbors = (int)pow(2, i);
		pthread_mutex_unlock(&mutex);
		
		// Split up work between threads
		for (int j = 0; j < size/num_threads; j++) {

			if (j+index == neighbors || j+index > neighbors) {
				pthread_mutex_lock(&mutex);
				y[j+index] = x[j+index] + x[j+index - neighbors];
				pthread_mutex_unlock(&mutex);

			} else {
				// no neighbors, copy down
				pthread_mutex_lock(&mutex);
				y[j+index] = x[j+index];
				pthread_mutex_unlock(&mutex);

			}


		}

		pthread_mutex_lock(&mutex);
		threads_done++;
		//printf("TD: %d\n", threads_done);
		pthread_mutex_unlock(&mutex);

		if (threads_done == num_threads) {

			pthread_mutex_lock(&mutex);
			for (int d = 0; d < size; d++) {
				x[d] = y[d];
			}
			threads_done = 0;
			pthread_mutex_unlock(&mutex);

			pthread_mutex_lock(&mutex);
			pthread_cond_broadcast(&cond);
			pthread_mutex_unlock(&mutex);

		} else {

			pthread_mutex_lock(&mutex);
			//while (threads_done != num_threads) {
				pthread_cond_wait(&cond, &mutex);
			//}
			pthread_mutex_unlock(&mutex);
		}		

	}

	pthread_cond_destroy(&cond);
	pthread_mutex_destroy(&mutex);
	
	return NULL;
}

int main(int argc, char** argv) {

	char* fname = argv[1];
	int size = atoi(argv[2]);
	int num_threads = atoi(argv[3]);
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);

	int *x = (int*) malloc(sizeof(int)*size);
	//int current[size];
	int *y = (int*) malloc(sizeof(int)*size);
	FILE *fp;
	char* line = NULL;
	size_t len = 1;
	int ln;
	fp = fopen(fname, "r");

		int c = 0;
		while ((ln = getline(&line, &len, fp)) != -1) {
        	x[c] = atoi(line);
        	c++;

	}

	pthread_t threads[num_threads];
	
	args = malloc(sizeof(args_t));

	args->size = size;
	args->x = x;
	args->y = y;
	args->size = size;
	args->num_threads = num_threads;
	args->threads = threads;

	int i;
	for (i = 0; i < num_threads; i++) {

		int* a = malloc(sizeof(int));
		*a = i * (size / num_threads);

		pthread_create(threads + i, NULL, calculate_sums, a);

	}

	for (int k = 0; k < num_threads; k++) {
		pthread_join(threads[k], NULL);
		
	}

	for(int i = 0; i < size; i++) {
		printf("%d\n", y[i]);
	}

	
	return 0;

}