#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <sys/time.h>

#define NUM_BUCKETS 5     // Buckets in hash table
#define NUM_KEYS 100000   // Number of keys inserted per thread
int num_threads = 1;      // Number of threads (configurable)
int keys[NUM_KEYS];

typedef struct _bucket_entry {
    int key;
    int val;
    struct _bucket_entry *next;
} bucket_entry;

bucket_entry *table[NUM_BUCKETS];
pthread_mutex_t lock[NUM_BUCKETS];          // Lock for each bucket to manage both reads and writes

void panic(char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

double now() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// Inserts a key-value pair into the table
void insert(int key, int val) {
    int i = key % NUM_BUCKETS;
    bucket_entry *e = (bucket_entry *) malloc(sizeof(bucket_entry));
    if (!e) panic("No memory to allocate bucket!");

    pthread_mutex_lock(&lock[i]); // Lock the bucket for exclusive access
    e->next = table[i];
    e->key = key;
    e->val = val;
    table[i] = e;
    pthread_mutex_unlock(&lock[i]); // Unlock the bucket
}

// Retrieves an entry from the hash table by key
// Returns NULL if the key isn't found in the table
bucket_entry *retrieve(int key) {
    int i = key % NUM_BUCKETS;

    // Lock the bucket for exclusive access (no parallelization for readers)
    pthread_mutex_lock(&lock[i]);

    // Search for the key in the bucket
    bucket_entry *b;
    for (b = table[i]; b != NULL; b = b->next) {
        if (b->key == key) {
            pthread_mutex_unlock(&lock[i]); // Unlock the bucket after finding the key
            return b;
        }
    }

    // Unlock the bucket if key was not found
    pthread_mutex_unlock(&lock[i]);
    return NULL;
}

void *put_phase(void *arg) {
    long tid = (long) arg;
    for (int key = tid; key < NUM_KEYS; key += num_threads) {
        insert(keys[key], tid);
    }
    pthread_exit(NULL);
}

void *get_phase(void *arg) {
    long tid = (long) arg;
    long lost = 0;

    for (int key = tid; key < NUM_KEYS; key += num_threads) {
        if (retrieve(keys[key]) == NULL) lost++;
    }
    printf("[thread %ld] %ld keys lost!\n", tid, lost);

    pthread_exit((void *)lost);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        panic("usage: ./parallel_mutex <num_threads>");
    }
    if ((num_threads = atoi(argv[1])) <= 0) {
        panic("must enter a valid number of threads to run");
    }

    // Initialize the mutexes and generate random keys
    for (int i = 0; i < NUM_BUCKETS; i++) {
        pthread_mutex_init(&lock[i], NULL);
    }

    srandom(time(NULL));
    for (int i = 0; i < NUM_KEYS; i++) {
        keys[i] = random();
    }

    pthread_t *threads = (pthread_t *) malloc(sizeof(pthread_t) * num_threads);
    if (!threads) {
        panic("Failed to allocate memory for thread handles");
    }

    // Parallel insertion phase
    double start = now();
    for (long i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, put_phase, (void *)i);
    }
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    double end = now();
    printf("[main] Inserted %d keys in %f seconds\n", NUM_KEYS, end - start);

    // Reset thread array for retrieval phase
    memset(threads, 0, sizeof(pthread_t) * num_threads);

    // Parallel retrieval phase
    start = now();
    for (long i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, get_phase, (void *)i);
    }

    long total_lost = 0;
    long *lost_keys = (long *) malloc(sizeof(long) * num_threads);
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], (void **)&lost_keys[i]);
        total_lost += lost_keys[i];
    }
    end = now();

    printf("[main] Retrieved %ld/%d keys in %f seconds\n", NUM_KEYS - total_lost, NUM_KEYS, end - start);

    // Clean up the mutexes
    for (int i = 0; i < NUM_BUCKETS; i++) {
        pthread_mutex_destroy(&lock[i]);
    }

    free(threads);
    free(lost_keys);

    return 0;
}
