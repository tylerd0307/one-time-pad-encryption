#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Function to generate a random character (A-Z or space)
char generate_random_char() {
    int rand_value = rand() % 27;
    return (rand_value == 26) ? ' ' : 'A' + rand_value;
}

int main(int argc, char *argv[]) {
    // Ensure the program is called with the correct number of arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s keylength\n", argv[0]);
        return 1;
    }

    // Parse the key length and validate it
    int key_length = atoi(argv[1]);
    if (key_length <= 0) {
        fprintf(stderr, "Error: keylength must be a positive integer\n");
        return 1;
    }

    // Seed the random number generator
    srand(time(NULL));

    // Generate and output the random key
    for (int i = 0; i < key_length; i++) {
        putchar(generate_random_char());
    }

    // Output a newline character and ensure all output is flushed
    putchar('\n');
    fflush(stdout); // Explicitly flush stdout to avoid buffering issues

    return 0;
}
