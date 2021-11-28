#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include "commands.h"
#include "main.h"
#include "util.h"

uint32_t first_color = 0;
uint32_t second_color = 0;
uint32_t third_color = 0;

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

void close_pipes(int (*pipes)[2], int pipe_count) {
    for (int i = 0; i < pipe_count; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
}

void free_commands(struct Commands* commands) {
    for (int i = 0; i < commands->cmd_count; i++) {
        free(commands->commands[i]);
    }
    free(commands);
}

// execute multiple commands (piping them lsif necessary)
int turtle_execute(struct Commands* commands) {
    int exec_ret;

    // if this is just a single command, then we can run it as normal
    if (commands->cmd_count == 1) {
        commands->commands[0]->fds[0] = STDIN_FILENO;
        commands->commands[0]->fds[1] = STDOUT_FILENO;
        exec_ret = turtle_execute_single(commands, commands->commands[0], NULL);
        wait(NULL);
    }
    // have to handle the commands through piping
    else {
        int pipe_count = commands->cmd_count - 1;
        int (*pipes)[2] = calloc(pipe_count * sizeof(int[2]), 1);

        if (pipes == NULL) {
            fprintf(stderr, "turtle failed to allocate memory\n");
            return 0;
        }

        // create all the pipes and set the file descriptors for each command
        // first command just reads from stdin
        commands->commands[0]->fds[0] = STDIN_FILENO;
        for (int i = 1; i < commands->cmd_count; i++) {
            pipe(pipes[i-1]);
            // previous command writes to the pipe
            commands->commands[i-1]->fds[1] = pipes[i-1][1];
            // the next command reads from the pipe
            commands->commands[i]->fds[0] = pipes[i-1][0];
        }
        // last command just writes to stdout
        commands->commands[pipe_count]->fds[1] = STDOUT_FILENO;

        // execute the commands
        for (int i = 0; i < commands->cmd_count; i++) {
            exec_ret = turtle_execute_single(commands, commands->commands[i], pipes);
        }

        close_pipes(pipes, pipe_count);

        // make sure all the processes/comamnds have been executed
        for (int i = 0; i < commands->cmd_count; i++) {
            wait(NULL);
        }

        free(pipes);
    }

    return exec_ret;
}

int turtle_execute_single(struct Commands* commands, struct Command* command, int (*pipes)[2]) {
    // check if the command is any of the built-ins
    if (strcmp(command->cmd_name, "cd") == 0) {
        return turtle_cd(command->argv);
    } else if (strcmp(command->cmd_name, "exit") == 0 || strcmp(command->cmd_name, "q") == 0) {
        return turtle_exit();
    } else if (strcmp(command->cmd_name, "help") == 0) {
        return turtle_help();
    } else if (strcmp(command->cmd_name, "turtlesay") == 0) {
        return turtlesay(command->argv);
    } else if(strcmp(command->cmd_name, "theme") == 0) {
        turtle_theme(command->argv);
        return 1;
    }
    // otherwise, fork the process and launch
    else {
        int pid = fork();

        // fork failed
        if (pid == -1) {
            fprintf(stderr, "turtle failed to fork process");
        }
        // in child process
        else if (pid == 0) {
            int input_fd = command->fds[0];
            int output_fd = command->fds[1];

            // change input file descriptor if it isn't std
            if (input_fd != -1 && input_fd != STDIN_FILENO) {
                dup2(input_fd, STDIN_FILENO);
            }
            // change output file descriptor if it isn't std
            if (output_fd != -1 && output_fd != STDOUT_FILENO) {
                dup2(output_fd, STDOUT_FILENO);
            }
            // check if this command requires piping, and if so, we
            // can now close the pipes because we have already changed
            // the input/output fd
            if (pipes != NULL) {
                int pipe_count = commands->cmd_count - 1;
                close_pipes(pipes, pipe_count);
            }

            // execute the command
            execvp(command->cmd_name, command->argv);
            fprintf(stderr, "turtle could not find the command\n"); // execvp should not return here

            // clean up memory if we failed to identify a command
            free(pipes);
            free_commands(commands);

            _exit(EXIT_FAILURE);
        }

        return pid; // exit success?
    }

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

        // TODO: ignore interactive and job-control signals
        // signal(SIGINT, SIG_IGN);
        // signal(SIGQUIT, SIG_IGN);
        // signal(SIGTSTP, SIG_IGN);
        // signal(SIGTTIN, SIG_IGN);
        // signal(SIGTTOU, SIG_IGN);
        // signal(SIGCHLD, SIG_IGN);

        // put ourselves in our own process group with turtle as the leader
        turtle_pgid = getpid();
        if (setpgid(turtle_pgid, turtle_pgid) < 0) {
            fprintf(stderr, "couldn't put turtle in its own process group\n");
            exit(1);
        }

        // take control of the terminal
        tcsetpgrp(turtle_terminal, turtle_pgid);
        
        // save default terminal attributes for the shell
        tcgetattr(turtle_terminal, &turtle_tmodes);

        // get the current directory
        cur_directory = (char*)calloc(256, sizeof(char));
    } else {
        fprintf(stderr, "couldn't make the turtle interactive\n");
        exit(1);
    }
}

struct Commands* turtle_parse(char* input) {
    int num_commands = 0;
    int input_index = 0;

    // iterate through the input to find how many commands there are
    while (input[input_index] != '\0') {
        if (input[input_index] == '|') {
            num_commands++;
        }
        input_index++;
    }
    num_commands++;

    struct Commands* list_commands = calloc(sizeof(struct Commands) + num_commands * sizeof(struct Command*), 1);
    if (list_commands == NULL) {
        fprintf(stderr, "turtle failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }
    list_commands->cmd_count = num_commands;
    list_commands->commands = calloc(num_commands * sizeof(struct Command*), 1);
    
    char* save;
    char* cur_command = strtok_r(input, "|", &save);
    int i = 0;
    while (cur_command != NULL && i < num_commands) {
        list_commands->commands[i] = turtle_parse_single(cur_command);
        cur_command = strtok_r(NULL, "|", &save);
        i++;
    }

    return list_commands;
}

struct Command* turtle_parse_single(char* command) {
    int num_args = 0;
    int command_index = 0;
    
    // iterate through the command to find how many args there are
    while (command[command_index] != '\0') {
        if (command[command_index] == ' ') {
            num_args++;
        }
        command_index++;
    }
    num_args++;

    struct Command* single_command = calloc(sizeof(struct Command) + num_args * sizeof(char*), 1);
    if (single_command == NULL) {
        fprintf(stderr, "turtle failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }
    single_command->argc = num_args;
    single_command->argv = calloc(sizeof(struct Command) * num_args, 1);

    // get token by splitting on whitespace
    char* cur_arg = strtok(command, " \t\r\n\a");
    int i = 0;
    while (cur_arg != NULL && i < num_args) {
        single_command->argv[i] = cur_arg;
        cur_arg = strtok(NULL, " \t\r\n\a");
        i++;
    }
    single_command->cmd_name = single_command->argv[0];

    return single_command;
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
    struct Commands* list_commands;
    int status = 1;
    
    while (status) {
        // print the prompt in the form of user@turtle/cwd $
        set_text(first_color);
        printf("%s@turtle %s ", getenv("LOGNAME"), getcwd(cur_directory, 4096));
        
        set_text(second_color);
        printf("$ ");

        set_text(third_color);

        // read the user's command(s) from standard input
        input = turtle_read();
        
        // parse the string into its command and arguments
        list_commands = turtle_parse(input);

        //execute the command
        status = turtle_execute(list_commands);

        // clean the memory for the next command
        free(input);
        free_commands(list_commands);
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