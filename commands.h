#ifndef COMMANDS_H    /* This is an "include guard" */
#define COMMANDS_H

extern int first_color;
extern int second_color;
extern int third_color;
extern int turtle_cd(int argc, char** args);
extern int turtle_exit();
extern int turtle_jobs();
extern int turtle_fg(int argc, char** argv);
extern int turtle_bg(int argc, char** argv);
extern int turtle_kill(int argc, char** argv);
extern int turtle_unset(int argc, char** argv);
extern int turtle_help();
extern int turtle_history();
extern int turtlesay(char** args);
extern int turtle_theme(int argc, char** args);
extern void turtle_theme_help();
extern void set_text(int color);
extern void set_theme(int first, int second, int third);
#endif