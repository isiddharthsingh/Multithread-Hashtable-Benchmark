#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_TRIALS 10
#define PROGRAMS 3
#define THREAD_COUNTS 10

// Programs to test
const char *programs[PROGRAMS] = {"./parallel_hashtable", "./parallel_mutex", "./parallel_spin"};
const char *program_names[PROGRAMS] = {"parallel_hashtable", "parallel_mutex", "parallel_spin"};

// Thread counts to test
const int thread_counts[THREAD_COUNTS] = {1, 2, 3, 4, 5, 6, 7, 8, 12, 16};

// Function to execute a program with a specified number of threads
double run_program(const char *program, int threads) {
    char command[256];
    snprintf(command, sizeof(command), "%s %d", program, threads);

    // Open a pipe to read program output
    FILE *fp = popen(command, "r");
    if (!fp) {
        fprintf(stderr, "Error: Could not run %s with %d threads\n", program, threads);
        exit(1);
    }

    // Read the output and parse execution time
    char line[256];
    double insert_time = 0, retrieve_time = 0;
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (sscanf(line, "[main] Inserted %*d keys in %lf seconds", &insert_time) == 1) {
            // Found insertion time
        } else if (sscanf(line, "[main] Retrieved %*d/%*d keys in %lf seconds", &retrieve_time) == 1) {
            // Found retrieval time
        }
    }

    // Close the pipe
    pclose(fp);

    // Return the total execution time (sum of insert and retrieve times)
    return insert_time + retrieve_time;
}

int main() {
    double results[PROGRAMS][THREAD_COUNTS] = {0};

    printf("Program,Threads,Execution_Time\n");

    // Run each program with each thread count and record execution times
    for (int i = 0; i < PROGRAMS; i++) {
        for (int j = 0; j < THREAD_COUNTS; j++) {
            int threads = thread_counts[j];
            double exec_time = run_program(programs[i], threads);
            results[i][j] = exec_time;
            printf("%s,%d,%f\n", program_names[i], threads, exec_time);
        }
    }

    // Optionally save results to a CSV file
    FILE *file = fopen("execution_times.csv", "w");
    if (file) {
        fprintf(file, "Program,Threads,Execution_Time\n");
        for (int i = 0; i < PROGRAMS; i++) {
            for (int j = 0; j < THREAD_COUNTS; j++) {
                fprintf(file, "%s,%d,%f\n", program_names[i], thread_counts[j], results[i][j]);
            }
        }
        fclose(file);
    } else {
        fprintf(stderr, "Error: Could not write to execution_times.csv\n");
    }

    return 0;
}
