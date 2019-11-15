#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>

int MAX_LENGTH_OF_STR = 2048;
int MAX_NUM_OF_ARGS = 512;

bool checkNumArgs(char * input);
bool signExists(char * input);
void expandfunction(char * input);
char ** commandArr(char * input, int * numStr);
void printArr(char ** array, int numStr);
void cdCommand(char ** array); 

int main() {
    
    // char * input;
    // size_t size = MAX_LENGTH_OF_STR;
    // do {
    //     printf(": ");
    //     getline(&input, &size, stdin); 
    //     strtok(input, "\n");
    //     expandfunction(input);
    //     printf("%s\n", input);
    //     if(checkNumArgs(input) != true) {
    //         continue;
    //     }
    // 
    // } while(strcmp(input, "exit") != 0);
    int numStr = 0;
    char input[256] = "";
    char ** array = commandArr(input, &numStr);
    cdCommand(array);
    return 0;
}

bool checkNumArgs(char * input) {
    int count = 1;
    int i;
    for(i = 0; i < strlen(input); i++) {
        if((input[i] == ' ') && (input[i+1] != ' ')) {
            count++;
        }
    }
    if(count <= MAX_NUM_OF_ARGS) {
        return true;
    }
    return false;
}

char ** commandArr(char * input, int * numStr) {
    char ** array;
    array = malloc(MAX_NUM_OF_ARGS * sizeof(char *));
    int k;
    for(k = 0; k < MAX_NUM_OF_ARGS; k++) {
        array[k] = malloc(MAX_LENGTH_OF_STR);
    }
    
    int i, j;
    j = 0; 
    for(i = 0; i < strlen(input) + 1; i++) {
        if(input[i] == ' ' || input[i] == '\0') {
            array[*numStr][j] = '\0';
            j = 0;
            (*numStr)++;
        }
        else {
            array[*numStr][j] = input[i];
            j++;
        }
    }
    return array;
}

void printArr(char ** array, int numStr) {
    int i;
    for(i = 0; i < numStr; i++) {
        printf("%s\n", array[i]);
    }
}

void cdCommand(char ** array) {
    char str[1000];

    if(strcmp(array[0], "cd") == 0) {
        if(strcmp(array[1], "") == 0) {
            chdir(getenv("HOME"));
            getcwd(str, sizeof(str));
            printf("%s\n", str);
        }
        else {
            chdir(array[1]);
            getcwd(str, sizeof(str));
            printf("%s\n", str);
        }
    }
    else {
        printf("ERROR\n");
    }
}

void exitCommand(char ** array) {
    if(strcmp(array[0], "exit") == 0) {
        if(strcmp(array[1], "") == 0) {
            // Kill everything
            // kill child and foreground processes
            // holder for all pid of child processes
            // run sigkill for each child process
        }
        else {
            printf("ERROR\n");
        }
    }
}

void statusCommand(char ** array) {
    if(strcmp(array[0], "status") == 0) {
        // Print out exit status or the terminating signal of the last foreground process
    }
}

bool ambExist(char ** array, int numStr) {
    if(strcmp(array[numStr-1], "&") == 0){
        return true;
    }
    return false;
}

bool signExists(char * input) {
    int i;
    for(i = 0; i < strlen(input); i++) {
        if((input[i] == '$') && (input[i+1] == '$')) {
            return true;
        }
    }
    return false;
}

void expandfunction(char * input) { // This should be the string that we use for the rest of the program
    pid_t pid = getpid();
    int temp = (int)pid;
    char newString[MAX_LENGTH_OF_STR]; // MIGHT NEED TO INCREASE THIS INCASE IT OVERFLOWS
    while(signExists(input) == true) {
        char * dds = strstr(input, "$$");
        dds[0] = '%';
        dds[1] = 'd';
        sprintf(newString, input, (int)pid);
        strcpy(input, newString);
    }
}

void redirectCommand(char ** array) {
    int i;
    for()
    
}
// using > or < to redirection
// dup2()
// cat < red needs to become cat
// cat < red > output also n eeds to become cat
// background process redirect to /dev/null
// open the file to /dev/null, dup2(), stdin = 0;
