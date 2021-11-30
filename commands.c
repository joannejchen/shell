#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "commands.h"
#include "main.h"

#define RESET 0
#define BLK 30
#define RED 31
#define GRN 32
#define YEL 33
#define BLU 34
#define MAG 35
#define CYN 36
#define WHT 37
#define HGRN 92

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

int turtle_history() {
    int i = 0;
    struct History* current = turtle_head;
    while (i < 5 && current != NULL) {
        printf("[%d] %s\n", i, current->history_command);
        current = current->turtle_next;
        i++;
    }

    printf("enter command number > ");
    int letter;
    int index = 0;
    char* buffer = malloc(sizeof(char) * 2);
    while (1) {
        letter = getchar();
        if (letter == EOF || letter == '\n' || letter == '\r') {
            break;
        }
        buffer[index] = letter;
        index++;
    }
    buffer[index] = 0;
    letter = atoi(buffer);

    // find the command with that number
    i = 0;
    current = turtle_head;
    while (i < letter && current != NULL) {
        current = current->turtle_next;
        i++;
    }

    struct Commands* list_commands = turtle_parse(current->history_command);

    int status = turtle_execute(list_commands);
    
    return status;
}

/* change theme of shell */
void turtle_theme(char** args) {
    if(strcmp(args[1], "help") == 0) turtle_theme_help();
    else if(strcmp(args[1], "reset") == 0) set_theme(RESET, RESET, RESET);
    else if(strcmp(args[1], "red") == 0) set_theme(RED, RED, RED);
    else if(strcmp(args[1], "yellow") == 0) set_theme(YEL, YEL, YEL);
    else if(strcmp(args[1], "green") == 0) set_theme(GRN, GRN, GRN);
    else if(strcmp(args[1], "cyan") == 0) set_theme(CYN, CYN, CYN);
    else if(strcmp(args[1], "blue") == 0) set_theme(BLU, BLU, BLU);
    else if(strcmp(args[1], "magenta") == 0) set_theme(MAG, MAG, MAG);
    else if(strcmp(args[1], "turtle") == 0) set_theme(GRN, BLU, CYN);
    else if(strcmp(args[1], "summer") == 0) set_theme(YEL, MAG, CYN);
    else if(strcmp(args[1], "winter") == 0) set_theme(BLU, CYN, WHT);
    else if(strcmp(args[1], "christmas") == 0) set_theme(RED, GRN, YEL);
    else if(strcmp(args[1], "hanukkah") == 0) set_theme(BLU, CYN, YEL);
    else { //invalid theme
        fprintf(stderr, "turtle: invalid theme name\n");
    }
}

void turtle_theme_help() {
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("hey there!\n");
    printf("to change your shell theme, type theme followed by the name of one of the themes below: \n");
    printf("\033[1;%dm red\n", RED);
    printf("\033[1;%dm yellow\n", YEL);
    printf("\033[1;%dm green\n", GRN);
    printf("\033[1;%dm cyan\n", CYN);
    printf("\033[1;%dm blue\n", BLU);
    printf("\033[1;%dm magenta\n", MAG);
    printf("\033[1;%dm tu\033[1;%dmrt\033[1;%dmle\n", GRN, BLU, CYN); //turtle
    printf("\033[1;%dm su\033[1;%dmmm\033[1;%dmer\n", YEL, MAG, CYN); //summer
    printf("\033[1;%dm wi\033[1;%dmnt\033[1;%dmer\n", BLU, CYN, WHT); //winter
    printf("\033[1;%dm chr\033[1;%dmist\033[1;%dmmas\n", RED, GRN, YEL); //christmas
    printf("\033[1;%dm han\033[1;%dmuk\033[1;%dmkah\n", BLU, CYN, YEL); //hanukkah
    set_text(RESET);
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
