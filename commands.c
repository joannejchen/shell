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

/* turtle say command */
void turtlesay(char** args) {
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
}