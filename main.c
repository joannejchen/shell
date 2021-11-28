#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include "commands.h"
#include "util.h"

#define input_size 1024
#define args_size 64

int turtle_execute(char** args);
int turtle_fork(char** args);
void turtle_init();
char** turtle_parse(char* input);
char* turtle_read();
void turtle_run();
void turtle_welcome();

int main(int argc, char* argv[], char** envp) {
    // initialize
    turtle_init();
    turtle_welcome();
    environ = envp;
    setenv("shell", getcwd(cur_directory, 4096), 1);

    // run command loop.
    turtle_run();

    // perform shutdown/cleanup

    return EXIT_SUCCESS;
}

int turtle_execute(char** args) {
    // no command was given
    if (args[0] == NULL) {
        return 1; // TODO: exit success?
    }

    // built-ins
    if (strcmp(args[0], "cd") == 0) {
        return turtle_cd(args);
    } else if (strcmp(args[0], "help") == 0) {
        return turtle_help();
    } else if (strcmp(args[0], "exit") == 0) {
        return turtle_exit();
    } else if (strcmp(args[0], "q") == 0) {
        return turtle_exit();
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

// make sure the shell is running interactively as the foreground job
// this is needed in order to allow our shell to also be able to run job control
void turtle_init() {
    // check if we are running interactively, which is when STDIN is the terminal
    turtle_terminal = STDIN_FILENO;
    turtle_is_interactive = isatty(turtle_terminal);
    
    if (turtle_is_interactive) {
        // loop until we are in the foreground
        while (tcgetpgrp(turtle_terminal) != (turtle_pgid = getpgrp())) {
            kill(-turtle_pgid, SIGTTIN);
        }

        // ignore interactive and job-control signals
        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);
        signal(SIGCHLD, SIG_IGN);

        // put ourselves in our own process group with turtle as the leader
        turtle_pgid = getpid();
        if (setpgid(turtle_pgid, turtle_pgid) < 0) {
            perror("couldn't put turtle in its own process group\n");
            exit(1);
        }

        // take control of the terminal
        tcsetpgrp(turtle_terminal, turtle_pgid);
        
        // save default terminal attributes for the shell
        tcgetattr(turtle_terminal, &turtle_tmodes);

        // get the current directory
        cur_directory = (char*)calloc(256, sizeof(char));
    } else {
        perror("couldn't make the turtle interactive\n");
        exit(1);
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
                buffer[index] = '\0';
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

void turtle_run() {
    char* input;
    char** args;
    int status = 1;
    
    while (status) {
        // print the prompt in the form of user@turtle/cwd $
        printf("%s@turtle %s $ ", getenv("LOGNAME"), getcwd(cur_directory, 4096));
        
        // read the user's command(s) from standard input
        input = turtle_read();
        
        // parse the string into its command and arguments
        args = turtle_parse(input);

        //execute the command
        status = turtle_execute(args);

        // clean the memory for the next command
        free(input);
        free(args);
    }
}

void turtle_welcome() {
    printf("------------------------------------------------------\n");
    printf("        you have stumbled on the turtle!\n\n");
    printf("\t                    __\n");
    printf("\t         .,-;-;-,. /'_\\\n");
    printf("\t       _/_/_/_|_\\_\\) /\n");
    printf("\t     '-<_><_><_><_>=/\\\n");
    printf("\t       `/_/====/_/-'\\_\\\n");
    printf("\t        \"\"     \"\"    \"\"\n");
    printf("\n    have fun interacting with the turtle!\n");
    printf("------------------------------------------------------\n");
}