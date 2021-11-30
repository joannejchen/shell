#define input_size 1024
#define args_size 64

struct Command {
    int argc;                   // number of arguments
    char* cmd_name;             // name of the command
    char** argv;                // list of arguments
    int fds[2];                 // file descriptors for input/output of the command
};

struct Commands {
    int cmd_count;              // number of commands to be executed
    struct Command** commands;   // list of the commands

};

struct History {
    struct History* turtle_next;
    char* history_command;
};

struct History* turtle_head;

struct sigaction act_int;

pid_t pid;

void close_pipes(int (*pipes)[2], int pipe_count);
void free_commands(struct Commands* commands);
void signal_handler_int(int p);
int turtle_execute(struct Commands* commands);
int turtle_execute_single(struct Commands* commands, struct Command* command, int (*pipes)[2]);
void turtle_init();
struct Commands* turtle_parse(char* input);
struct Command* turtle_parse_single(char* command);
void turtle_pipe(char** args);
char* turtle_read();
void turtle_run();
void turtle_welcome();