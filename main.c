#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "commands.h"

#define input_size 1024
#define args_size 64

void turtle_run();
char* turtle_read();
char** turtle_parse(char* input);
int turtle_execute(char** args);
int turtle_fork(char** args);

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
        printf("turtle $ ");
        input = turtle_read();
        // Parse the string into its command and arguments
        args = turtle_parse(input);
        // Execute the command
        status = turtle_execute(args);

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
        if (letter == EOF) {
            buffer[index] = '\0'; // null terminate strings
            return buffer;
        }
        // handle special newline case
        else if (letter == '\n') {
            // be able to handle multiple lines of input
            if (index == 0 || buffer[index-1] != '\\') {
                return buffer;
            } else {
                index--;
                printf("> ");
            }
        }
        // read input as normally
        else {
            buffer[index] = letter;
            index++;
        }

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

    return args;
}

int turtle_execute(char** args) {
    // no command was given
    if (args[0] == NULL) {
        return 1; // TODO: exit success?
    }

    // built-ins
    if (strcmp(args[0], "cd") == 0) {
        // TODO
        return 1;
    } else if (strcmp(args[0], "help") == 0) {
        // TODO
        return 1;
    } else if (strcmp(args[0], "exit") == 0) {
        // TODO
        return 1;
    } else if (strcmp(args[0], "q") == 0) {
        exit(EXIT_SUCCESS);
    } else if (strcmp(args[0], "turtlesay") == 0) {
        turtlesay(args);
        return 1;
    } else {
        // launch a new process to handle this command
        return turtle_fork(args);
    }
}

int turtle_fork(char** args) {
    int pid;
    int wait_val;
    int status;

    pid = fork();
    // error when forking
    if (pid < 0) {
        perror("turtle");
    }
    // child process
    else if (pid == 0) {
        if (execvp(args[0], args) == -1) {
            perror("turtle");
        }
        exit(EXIT_FAILURE); // shouldn't execute if execvp was successful
    }
    // parent process
    else {
        do {
            wait_val = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1; // TODO: exit success?
}