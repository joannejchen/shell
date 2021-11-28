// shell attributes for init
static pid_t turtle_pgid;
static int turtle_terminal;
static int turtle_is_interactive;
static struct termios turtle_tmodes;

// shell attributes for maintaining information about the environment
static char* cur_directory;
static char** environ;