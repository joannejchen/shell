#include "commands.h"
#include "main.h"

int first_color = 0;
int second_color = 0;
int third_color = 0;

int main(int argc, char** argv) {
    // initialize
    turtle_init();
    turtle_welcome();

    // run command loop
    turtle_run();

    // perform shutdown
    return EXIT_SUCCESS;
}

// make sure the shell is running interactively as the foreground job
// this is needed in order to allow our shell to also be able to run job control
void turtle_init() {
    // check if we are running interactively (i.e. when STDIN is the terminal)
    turtle_terminal = STDIN_FILENO;
    turtle_is_interactive = isatty(turtle_terminal);

    if (turtle_is_interactive) {
        // loop until we are in the foreground
        while (tcgetpgrp(turtle_terminal) != (turtle_pgid = getpgrp())) {
            kill (-turtle_pgid, SIGTTIN);
        }

        // set signals for the default shell window
        act_int.sa_handler = sigint_handler;
        sigaction(SIGINT, &act_int, 0);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);

        // put turtle in a process group and take control of terminal
        pid_t pid = getpid();
        setpgid(pid, pid);
        tcsetpgrp(0, pid);

        // set up information for the shell
        shell = calloc(sizeof(struct shell_info), 1);
        getlogin_r(shell->user, sizeof(shell->user));
        struct passwd *temp_pw = getpwuid(getuid());
        strcpy(shell->pw_dir, temp_pw->pw_dir);
        getcwd(shell->dir, sizeof(shell->dir));
        for (int i = 0; i < MAX_NUM_JOBS; i++) {
            shell->jobs[i] = NULL;
        }
    }
}

// default handler when trying to ctrl-c in the terminal
void sigint_handler(int signal) {
    printf("\n");
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

void turtle_run() {
    char* input;
    struct Job* job;
    int status;

    while (1) {
        set_text(first_color);
        printf("%s@turtle ", getenv("LOGNAME"));

        set_text(second_color);
        printf("%s $ ", getcwd(shell->dir, 4096));

        set_text(third_color);
        input = turtle_read();

        job = turtle_parse(input);

        status = turtle_execute(job);
    }
}

char* turtle_read() {
    int buffer_size = INPUT_SIZE;
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

            // copy this string into the history
            struct History* new_command = calloc(sizeof(struct History*), 1);
            new_command->history_command = calloc(sizeof(char) * strlen(buffer), 1);
            strcpy(new_command->history_command, buffer);
            if (turtle_head != NULL) {
                new_command->turtle_next = turtle_head;
            }
            turtle_head = new_command;

            return buffer;
        }
        // handle special newline case
        else if (letter == '\n') {
            // be able to handle multiple lines of input
            if (index == 0 || buffer[index-1] != '\\') {
                buffer[index] = '\0';

                // copy this string into the history
                struct History* new_command = calloc(sizeof(struct History*), 1);
                new_command->history_command = calloc(sizeof(char) * strlen(buffer), 1);
                strcpy(new_command->history_command, buffer);
                if (turtle_head != NULL) {
                    new_command->turtle_next = turtle_head;
                }
                turtle_head = new_command;
                
                return buffer;
            } else {
                index-=2;
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
            buffer_size += INPUT_SIZE;
            buffer = realloc(buffer, buffer_size);

            // make sure we had a successful malloc
            if (!buffer) {
                fprintf(stderr, "turtle failed to read\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

struct Job* turtle_parse(char* input) {
    struct Command *root_cmd = NULL;
    struct Command *cmd = NULL;
    enum mode mode_type = FOREGROUND;

    if (input[strlen(input) - 1] == '&') {
        mode_type = BACKGROUND;
        input[strlen(input) - 1] = '\0';
    }

    int num_commands = 0;
    int input_index = 0;

    // find how many comands there are
    while (input[input_index] != '\0') {
        if (input[input_index] == '|') {
            num_commands++;
        }
        input_index++;
    }
    num_commands++;

    // parse through each individual command
    char* save;
    char* cur_cmd = strtok_r(input, "|", &save);
    int i = 0;
    while (cur_cmd != NULL && i < num_commands) {
        struct Command* new_cmd = turtle_parse_single(cur_cmd);
        if (!root_cmd) {
            root_cmd = new_cmd;
            cmd = root_cmd;
        } else {
            cmd->next = new_cmd;
            cmd = new_cmd;
        }
        cur_cmd = strtok_r(NULL, "|", &save);
        i++;
    }

    // create a new job associated with this command
    struct Job* new_job = calloc(sizeof(struct Job), 1);
    new_job->root = root_cmd;
    new_job->pgid = -1;
    new_job->mode_type = mode_type;
    return new_job;
}

struct Command* turtle_parse_single(char* command) {
    int buf_size = BUFFER_SIZE;
    int position = 0;
    char* arg;
    char** args = calloc(buf_size * sizeof(char*), 1);

    if (!args) {
        fprintf(stderr, "turtle failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }

    arg = strtok(command, " \t\r\n\a");
    while (arg != NULL) {
        glob_t glob_buffer;
        int glob_count = 0;
        if (strchr(arg, '*') != NULL || strchr(arg, '?') != NULL) {
            glob(arg, 0, NULL, &glob_buffer);
            glob_count = glob_buffer.gl_pathc;
        }

        if (position + glob_count >= buf_size) {
            buf_size += BUFFER_SIZE;
            buf_size += glob_count;
            args = realloc(args, buf_size * sizeof(char*));
            if (!args) {
                fprintf(stderr, "turtle failed to allocate memory\n");
                exit(EXIT_FAILURE);
            }
        }

        if (glob_count > 0) {
            for (int i = 0; i < glob_count; i++) {
                args[position++] = strdup(glob_buffer.gl_pathv[i]);
            }
            globfree(&glob_buffer);
        } else {
            if (arg[0] == '$') {
                args[position] = getenv(&(arg[1]));
            } else {
                args[position] = arg;
            }
            position++;
        }
        arg = strtok(NULL, " \t\r\n\a");
    }
    // check if there was io redirection
    // and to see how long the actual command is
    int i = 0, argc = 0;
    char* input_path = NULL;
    char* output_path = NULL;
    while (i < position) {
        if (args[i][0] == '<' || args[i][0] == '>') {
            break;
        }
        i++;
    }
    argc = i;

    // copy input/output paths
    for(int j = i; j < position; j++) {
        if (args[j][0] == '<') {
            // input_path is the next arg
            if (strlen(args[j]) == 1) {
                input_path = calloc(strlen((args[j+1]) + 1) * sizeof(char), 1);
                strcpy(input_path, args[j+1]);
                j++;
            }
            // input path is part of this arg
            else {
                input_path = calloc(strlen(args[j]) * sizeof(char), 1);
                strcpy(input_path, args[j] + 1);
            }
        } else if (args[i][0] == '>') {
            // output path is the next arg
            if (strlen(args[j]) == 1) {
                output_path = calloc((strlen(args[j+1]+1)) * sizeof(char), 1);
                strcpy(output_path, args[j+1]);
            }
            // output path is part of this arg
            else {
                output_path = calloc(strlen(args[j]) * sizeof(char), 1);
                strcpy(output_path, args[j]+1);
            }
        } else {
            break;
        }
    }
    // null terminate the command to ignore io redirection
    for (int j = argc; j <= position; j++) {
        args[j] = NULL;
    }

    struct Command* new_cmd = calloc(sizeof(struct Command), 1);
    new_cmd->argv = args;
    new_cmd->argc = argc;
    new_cmd->input_path = input_path;
    new_cmd->output_path = output_path;
    new_cmd->pid = -1;
    new_cmd->cmd_type = turtle_get_cmd_type(args[0]);
    new_cmd->next = NULL;
    return new_cmd;
}

enum command_type turtle_get_cmd_type(char* cmd_name) {
    if (strcmp(cmd_name, "exit") == 0) {
        return EXIT;
    } else if (strcmp(cmd_name, "cd") == 0) {
        return CD;
    } else if (strcmp(cmd_name, "jobs") == 0) {
        return JOBS;
    } else if (strcmp(cmd_name, "fg") == 0) {
        return FG;
    } else if (strcmp(cmd_name, "bg") == 0) {
        return BG;
    } else if (strcmp(cmd_name, "kill") == 0) {
        return KILL;
    } else if (strcmp(cmd_name, "export") == 0) {
        return EXPORT;
    } else if (strcmp(cmd_name, "unset") == 0) {
        return UNSET;
    } else if (strcmp(cmd_name, "history") == 0) {
        return HISTORY;
    } else if (strcmp(cmd_name, "theme") == 0) {
        return THEME;
    } else {
        return EXTERNAL;
    }
}

int turtle_execute(struct Job* job) {
    int exec_ret = 1, in_fd = 0, fd[2], job_id = -1;

    if (job->root->cmd_type == EXTERNAL) {
        job_id = turtle_insert_job(job);
    }

    struct Command* cur_cmd = job->root;
    while (cur_cmd != NULL) {
        if (cur_cmd == job->root && cur_cmd->input_path != NULL) {
            in_fd = open(cur_cmd->input_path, O_RDONLY);
            if (in_fd < 0) {
                printf("turtle found no such file or directory to read from: %s\n", cur_cmd->input_path);
                turtle_remove_job(job_id);
                return -1;
            }
        }
        // identified piping
        if (cur_cmd->next != NULL) {
            pipe(fd);
            exec_ret = turtle_execute_single(job, cur_cmd, in_fd, fd[1], PIPELINE);
            close(fd[1]);
            in_fd = fd[0];
        } else {
            int out_fd = 1;
            if (cur_cmd->output_path != NULL) {
                out_fd = open(cur_cmd->output_path, O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
                if (out_fd < 0) {
                    out_fd = 1;
                }
            }
            exec_ret = turtle_execute_single(job, cur_cmd, in_fd, out_fd, job->mode_type);
        }

        cur_cmd = cur_cmd->next;
    }

    if (job->root->cmd_type == EXTERNAL) {
        if (exec_ret >= 0 && job->mode_type == FOREGROUND) {
            turtle_remove_job(job_id);
        } else if (job->mode_type == BACKGROUND) {
            turtle_print_process(job_id);
        }
    }

    return exec_ret;
}

int turtle_insert_job(struct Job* job) {
    int id = -1;

    for (int i = 1; i <= MAX_NUM_JOBS; i++) {
        if (shell->jobs[i] == NULL) {
            id = i;
            break;
        }
    }
    
    if (id < 0) {
        return -1;
    }
    
    job->id = id;
    shell->jobs[id] = job;
    return id;
}

int turtle_remove_job(int id) {
    if (id < 0 || id > MAX_NUM_JOBS || shell->jobs[id] == NULL) {
        return -1;
    }
    
    // free all the memory associated with this job
    struct Job* job = shell->jobs[id];
    struct Command* temp;
    struct Command* cur_cmd = job->root;
    while (cur_cmd != NULL) {
        temp = cur_cmd->next;
        free(cur_cmd->argv);
        free(cur_cmd->input_path);
        free(cur_cmd->output_path);
        free(cur_cmd);
        cur_cmd = temp;
    }
    free(job);

    shell->jobs[id] = NULL;

    return 0;
}

int turtle_remove_process(int pid) {
    struct Command* cur_cmd;

    for (int i = 1; i <= MAX_NUM_JOBS; i++) {
        if (shell->jobs[i] == NULL) {
            continue;
        }
        cur_cmd = shell->jobs[i]->root;
        while (cur_cmd != NULL) {
            if (cur_cmd->pid == pid) {
                turtle_remove_job(i);
                return 0;
            }
            cur_cmd = cur_cmd->next;
        }
    }

    // couldn't find the process to change its status of
    return -1;
}

int turtle_print_process(int id) {
    if (id < 0 || id > MAX_NUM_JOBS || shell->jobs[id] == NULL) {
        return -1;
    }

    printf("[%d]", id);

    struct Command* cur_cmd = shell->jobs[id]->root;
    while (cur_cmd != NULL) {
        printf(" %d", cur_cmd->pid);
        cur_cmd = cur_cmd->next;
    }
    printf("\n");

    return 0;
}

int turtle_execute_single(struct Job* job, struct Command* cmd, int in_fd, int out_fd, enum mode mode_type) {
    cmd->status_type = RUNNING;
    // check if the command is any of the builtins
    // EXIT, CD, JOBS, FG, BG, KILL, EXPORT, UNSET
    if (cmd->cmd_type == EXIT) {
        return turtle_exit();
    } else if (cmd->cmd_type == CD) {
        return turtle_cd(cmd->argv);
    } else if (cmd->cmd_type == JOBS) {
        return turtle_jobs();
    } else if (cmd->cmd_type == FG) {
        return turtle_fg(cmd->argc, cmd->argv);
    } else if (cmd->cmd_type == BG) {
        return turtle_bg(cmd->argc, cmd->argv);
    } else if (cmd->cmd_type == KILL) {
        return turtle_kill(cmd->argc, cmd->argv);
    } else if (cmd->cmd_type == EXPORT) {
        return 0;
    } else if (cmd->cmd_type == UNSET) {
        return 0;
    } else if (cmd->cmd_type == HISTORY) {
        return turtle_history();
    } else if (cmd->cmd_type == THEME) {
        return turtle_theme(cmd->argv);
    }

    // check if the command is assigning a variable
    for (int i = 0; i < strlen(cmd->argv[0]); i++) {
        if (cmd->argv[0][i] == '=') {
            putenv(cmd->argv[0]);
            return 1;
        }
    }

    int exec_ret = 0;
    pid_t child = fork();

    if (child < 0) {
        return -1;
    } else if (child == 0) {
        // restore all the signals
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);

        // set this cmd's pid and process group
        cmd->pid = getpid();
        if (job->pgid <= 0) {
            job->pgid = cmd->pid;
        }
        setpgid(0, job->pgid);

        // check for input redirection
        if (in_fd != 0) {
            dup2(in_fd, 0);
            close(in_fd);
        }

        // check for output redirection
        if (out_fd != 1) {
            dup2(out_fd, 1);
            close(out_fd);
        }

        execvp(cmd->argv[0], cmd->argv);
        fprintf(stderr, "turtle could not find command: %s\n", cmd->argv[0]);
        exit(0);
    } else { // parent
        cmd->pid = child;
        if (job->pgid <= 0) {
            job->pgid = cmd->pid;
        }
        setpgid(child, job->pgid);

        // wait for this process to finish
        if (mode_type == FOREGROUND) {
            tcsetpgrp(0, job->pgid);
            exec_ret = turtle_wait_job(job->id);
            signal(SIGTTOU, SIG_IGN);
            tcsetpgrp(0, getpid());
            signal(SIGTTOU, SIG_DFL);
        }
    }

    return exec_ret;

}

int turtle_wait_job(int id) {
    if (id < 0 || id > MAX_NUM_JOBS || shell->jobs[id] == NULL) {
        return -1;
    }

    // find how many subcommands we need to wait on
    int cmd_count = 0;
    struct Command* cur_cmd = shell->jobs[id]->root;
    while (cur_cmd != NULL) {
        if (cur_cmd->status_type != DONE) {
            cmd_count++;
        }
        cur_cmd = cur_cmd->next;
    }

    int wait_pid = -1;
    int wait_count = 0;
    int status = 0;

    do {
        wait_pid = waitpid(-shell->jobs[id]->pgid, &status, WUNTRACED);
        wait_count++;

        if (WIFEXITED(status)) {
            turtle_set_status(wait_pid, DONE);
        } else if (WIFSIGNALED(status)) {
            turtle_set_status(wait_pid, TERMINATED);
        } else if (WSTOPSIG(status)) {
            status = -1;
            turtle_set_status(wait_pid, SUSPENDED);
            if (wait_count == cmd_count) {
                turtle_print_job_status(wait_pid);
            }
        }
    } while (wait_count < cmd_count);

    return status;
}

int turtle_set_status(int pid, enum status status) {
    struct Command* cur_cmd;

    for (int i = 1; i <= MAX_NUM_JOBS; i++) {
        if (shell->jobs[i] == NULL) {
            continue;
        }
        cur_cmd = shell->jobs[i]->root;
        while (cur_cmd != NULL) {
            if (cur_cmd->pid == pid) {
                cur_cmd->status_type = status;
                return 0;
            }
            cur_cmd = cur_cmd->next;
        }
    }

    // couldn't find the process to change its status of
    return -1;
}

int turtle_print_job_status(int id) {
    if (id < 0 || id > MAX_NUM_JOBS || shell->jobs[id] == NULL) {
        return -1;
    }

    printf("[%d]", id);

    struct Command* cur_cmd = shell->jobs[id]->root;
    while (cur_cmd != NULL) {
        printf("\t%d\t", cur_cmd->pid);
        for(int i = 0; i < cur_cmd->argc; i++) {
            printf("%s ", cur_cmd->argv[i]);
        }
        printf("\t");
        switch(cur_cmd->status_type) {
            case RUNNING:
                printf("running");
                break;
            case DONE:
                printf("done");
                break;
            case SUSPENDED:
                printf("suspended");
                break;
            case CONTINUED:
                printf("continued");
                break;
            case TERMINATED:
                printf("terminated");
                break;

        }
        cur_cmd = cur_cmd->next;
        if (cur_cmd != NULL) {
            printf("|\n");
        } else {
            printf("\n");
        }
    }
    return 0;
}