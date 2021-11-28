#ifndef COMMANDS_H    /* This is an "include guard" */
#define COMMANDS_H

extern uint32_t first_color;
extern uint32_t second_color;
extern uint32_t third_color;
extern int turtle_cd(char** args);
extern int turtle_exit();
extern int turtle_help();
extern int turtlesay(char** args);
void turtle_theme(char** args);
void turtle_theme_help();
void set_text(uint32_t color);
void set_theme(uint32_t first, uint32_t second, uint32_t third);
#endif