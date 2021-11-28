#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "commands.h"

/* change directory */
int turtle_cd(char** args) {
    //check for invalid input
    if(args[1] == NULL) {
        fprintf(stderr, "turtle: invalid argument for cd\n");
    } else {
        if(chdir(args[1]) != 0) {
            perror("turtle");
        }
    }
    return 1;
}

/* exits the shell */
int turtle_exit() {
    return 0;
}

/* prints basic information about this shell */
int turtle_help() {
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("welcome to the turtle shell!\n");
    printf("to use, type a valid command followed by any relevant arguments\n");
    printf("in addition to most general shell commands, the following are also builtin:\n");
    printf("\thelp\n");
    printf("\tturtlesay\n");
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    return 1;
}

/* change theme of shell */
void turtle_theme(char** args) {
    if(strcmp(args[1], "help") == 0) turtle_theme_help();
    else if(strcmp(args[1], "reset") == 0) set_theme(0, 0, 0);
    else if(strcmp(args[1], "red") == 0) set_theme(31, 31, 31);
    else if(strcmp(args[1], "yellow") == 0) set_theme(33, 33, 33);
    else if(strcmp(args[1], "green") == 0) set_theme(32, 32, 32);
    else if(strcmp(args[1], "lightblue") == 0) set_theme(36, 36, 36);
    else if(strcmp(args[1], "darkblue") == 0) set_theme(34, 34, 34);
    else if(strcmp(args[1], "purple") == 0) set_theme(35, 35, 34);
    else if(strcmp(args[1], "turtle") == 0) set_theme(34, 36, 32);
    else { //invalid theme
        fprintf(stderr, "turtle: invalid theme name\n");
    }
}

void turtle_theme_help() {
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("hey there!\n");
    printf("to change your shell theme, type theme followed by the name of one of the themes below: \n");
    set_text(31);
    printf("red\n");
    set_text(33);
    printf("yellow\n");
    set_text(32);
    printf("green\n");
    set_text(36);
    printf("lightblue\n");
    set_text(34);
    printf("darkblue\n");
    set_text(35);
    printf("purple\n");
    set_text(0);
    printf("to reset your theme to the default, type theme reset\n");
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
}

/* turtle say command */
int turtlesay(char** args) {
    printf("\n"); //next line

    //determine output string
    int index = 1;
    char* output = malloc(60); //TODO: make this a pointer? doesn't work for some reason
    char* cur_arg = args[index];
    while(cur_arg != NULL) {
        strcat(output, cur_arg);
        strcat(output, " ");
        index++;
        cur_arg = args[index];
    } 

    int length = strlen(output);

    //top of textbox
    printf("                      ");
    for(int i = 0; i<length-1; i++) {
        printf("_");
    }
    printf("\n");


    printf("                    < "); //start of text bubble
    printf("%s", output);
    printf("> \n"); //close text bubble

    //bottom of textbox
    printf("                      ");
    for(int i = 0; i<length-1; i++) {
        printf("-");
    }
    printf("\n");

    //the turtle
    printf("  _____     ____   /\n");
    printf(" /      \\  |  o | /\n");
    printf("|        |/ ___\\| \n");
    printf("|_________/     \n");
    printf("|_|_| |_|_|\n");

    printf("\n");
    
    return 1;
}

void set_text(uint32_t color) { printf("\033[1;%dm", color); }

void set_theme(uint32_t first, uint32_t second, uint32_t third) {
    first_color = first;
    second_color = second;
    third_color = third;
}
