#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define input_size 1024
#define args_size 64

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
        // status = turtle_execute();

        // clean memory for the next command
        free(input);
        free(args);

    }
}

char* turtle_read() {
    int buffer_size = input_size;
    int index = 0;
    char* buffer = malloc(sizeof(char) * buffer_size);
    int letter;

    // make sure we had a successful malloc
    if (!buffer) {
        fprintf(stderr, "turtle failed to read\n");
        exit(EXIT_FAILURE);
    }

    // read in input from the user until we hit the last character
    while (1) {
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
            buffer_size += input_size;
            buffer = realloc(buffer, buffer_size);

            // make sure we had a successful malloc
            if (!buffer) {
                fprintf(stderr, "turtle failed to read\n");
                exit(EXIT_FAILURE);
            }
        }
    }

}

char** turtle_parse(char* input) {
    int buffer_args = args_size;
    int arg_index = 0;
    char** args = malloc(sizeof(char*) * buffer_args);
    char* cur_arg;

    // determine whether the malloc failed
    if (!args) {
        fprintf(stderr, "turtle failed to parse\n");
        exit(EXIT_FAILURE);
    }

    cur_arg = strtok(input, " \t\r\n\a");
    while (cur_arg != NULL) {
        args[arg_index] = cur_arg;
        arg_index++;

        if (arg_index >= buffer_args) {
            buffer_args += args_size;
            args = realloc(args, buffer_args * sizeof(char*));
            if (!args) {
                fprintf(stderr, "turtle failed to parse\n");
                exit(EXIT_FAILURE);
            }
        }

        cur_arg = strtok(NULL, " \t\r\n\a");
    }

    args[arg_index] = NULL;

    // // ensure proper parsing
    // for (int i = 0; i < arg_index; i++) {
    //     printf("%s\n", args[i]);
    // }

    // return args;
}

int turtle_execute(char** args) {
    return 0;
}