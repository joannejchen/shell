#include <stdio.h>
#include <stdlib.h>

#define block_size 1024

void turtle_run();
char* turtle_read();
char** turtle_parse(char* input);
int turtle_execute(char** args);

int main() {
    // TODO: load config files

    // Run command loop.
    turtle_run();

    // Perform shutdown/cleanup

    return EXIT_SUCCESS;
}

void turtle_run() {
    char* input;
    char** args;
    int status = 1;

    while (status) {
        // Read the command from standard input
        printf("turtle ~ ");
        input = turtle_read();
        // Parse the string into its command and arguments
        args = turtle_parse(input);
        // Execute the command
        status = turtle_execute();

        // clean memory for the next command
        free(input);
        free(args);

    }
}

char* turtle_read() {
    int buffer_size = block_size;
    int index = 0;
    char* buffer = malloc(sizeof(char) * buffer_size);
    int letter;

    // make sure we had a successful malloc
    if (!buffer) {
        fprintf(stderr, "turtle failed to read");
        exit(EXIT_FAILURE);
    }

    // read in input from the user until we hit the last character
    while (true) {
        letter = getchar();

        // check if this char is the last character
        if (letter == EOF || letter == '\n') {
            buffer[index] = '\0'; // null terminate strings
            return buffer;
        }

        buffer[index] = letter;
        index++;

        // determine whether we need to allocate more space
        if (index >= buffer_size) {
            buffer_size += block_size;
            buffer = realloc(buffer, buffer_size);

            // make sure we had a successful malloc
            if (!buffer) {
                fprintf(stderr, "turtle failed to read");
                exit(EXIT_FAILURE);
            }
        }
    }

}

char** turtle_parse(char* input) {
    
}

int turtle_execute(char** args) {
    
}