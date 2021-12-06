#include <ctype.h>
#include <fcntl.h>
#include <glob.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_USER_LENGTH 32
#define MAX_PATH_LENGTH 4096
#define MAX_NUM_JOBS 20
#define INPUT_SIZE 1024
#define BUFFER_SIZE 64

// signal handlers
struct sigaction act_int;

// shell attributes for current shell information
struct shell_info {
    char user[MAX_USER_LENGTH];
    char dir[MAX_PATH_LENGTH];
    char pw_dir[MAX_PATH_LENGTH];
    struct Job* jobs[MAX_NUM_JOBS];
};
struct shell_info* shell;

// information related to a command
enum command_type{EXIT, CD, JOBS, FG, BG, KILL, UNSET, EXTERNAL, HISTORY, THEME, HELP, TURTLESAY};
enum status{RUNNING, DONE, SUSPENDED, CONTINUED, TERMINATED};
struct Command {
    int argc;                   // number of arguments
    enum command_type cmd_type; // type of the command
    char** argv;                // list of arguments
    pid_t pid;                  // process id associated with this command
    char* input_path;           // where the command is reading input from
    char* output_path;          // where the command is writing output to
    enum status status_type;    // status for the command
    struct Command *next;       // any commands that follow
};

// information related to a job
enum mode{FOREGROUND, BACKGROUND, PIPELINE};
struct Job {
    int id;
    struct Command *root;
    pid_t pgid;
    enum mode mode_type;
};

// struct to keep track of list of commands
struct History {
    struct History* turtle_next;
    char* history_command;
};

struct History* turtle_head;

// list of methods
void turtle_init();
void sigint_handler(int signal);
void turtle_welcome();
void turtle_run();
char* turtle_read();
struct Job* turtle_parse(char* input);
struct Command* turtle_parse_single(char* command);
enum command_type turtle_get_cmd_type(char* command);
int turtle_execute(struct Job* job);
int turtle_insert_job(struct Job* job);
int turtle_remove_job(int id);
int turtle_remove_process(int pid);
int turtle_print_process(int id);
int turtle_execute_single(struct Job* job, struct Command* cmd, int in_fd, int out_fd, enum mode mode_type);
int turtle_wait_job(int id);
int turtle_set_status(int pid, enum status status);
int turtle_print_job_status(int id);