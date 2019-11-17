#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <fcntl.h>

int MAX_LENGTH_OF_STR = 2048;
int MAX_NUM_OF_ARGS = 512;

bool ifValidArgNum(char * input);
bool signExists(char * input);
char * expandUserInput(char * input);
char ** commandArr(char ** array, char * input, int * numStr);
void printArr(char ** array, int numStr);
void cdCommand(char ** array); 
char ** createArr();
bool isComment(char ** array);
bool ambExist(char ** array, int numStr);
bool notBulitIn(char ** array);

int main() {
    int num_of_strings = 0;
    char * user_input;
    char * expanded_user_input;
    char ** user_array = createArr();
    int i;
    size_t input_size = MAX_LENGTH_OF_STR;
    do {
        printf(": ");
        fflush(stdout);
        getline(&user_input, &input_size, stdin);
        expanded_user_input = expandUserInput(user_input);
        num_of_strings = countWords(expanded_user_input);
        
        if(ifValidArgNum(expanded_user_input) != true) {
            printf("input is too big\n");
            fflush(stdout);
            continue;
        }
        
        i = 1;    
            
        // Place each string from the input into the array until NULL terminator is found
        user_array[0] = strtok(expanded_user_input, " \n");    
        while((user_array[i] = strtok(NULL, " \n"))) i++;
        
        if((user_array[0] == NULL) || (strcmp(user_array[0], "#") == 0)) { // Check for cases where #fork (no spaces)
            continue;
        }
        
        if(strcmp(user_array[0], "cd") == 0) {
            cdCommand(user_array);
        }
        else if(strcmp(user_array[0], "exit") == 0) {
            // run the exit function
        }
        else if(strcmp(user_array[0], "status") == 0) {
            // run the status function
        }
        else {
            // execvp(user_array[0], user_array); // Runs commands that are not bulit in
            nonBuiltCommand(user_array);
            continue;
        }


        // if(notBulitIn(array) == true) {
        //     printf("I made it here\n");
        //     execvp(array[0], array);
        // }
    
    } while(strcmp(user_input, "exit") != 0);
    
    
    return 0;
}

bool ifValidArgNum(char * input) {
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

char * expandUserInput(char * input) { // This should be the string that we use for the rest of the program
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
    return input;
}

char ** createArr() {
    char ** array;
    array = malloc(MAX_NUM_OF_ARGS * sizeof(char *));
    int k;
    for(k = 0; k < MAX_NUM_OF_ARGS; k++) {
        array[k] = malloc(MAX_LENGTH_OF_STR);
    }
    
    int i;
    for(i = 0; i < MAX_NUM_OF_ARGS; i++) {
        array[i] = NULL;
    }
    
    return array;
}

char ** commandArr(char ** array, char * input, int * numStr) {
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

int countWords(char * user_input) {
    int num_of_strings = 0;
    int i;
    for(i = 0; i < strlen(user_input) + 1; i++) {
        if(user_input[i] == ' ' || user_input[i] == '\0') {
            num_of_strings++;
        }
    }
    return num_of_strings;
}

void printArr(char ** array, int numStr) {
    int i;
    for(i = 0; i < numStr; i++) {
        printf("%s\n", array[i]);
    }
}

void cdCommand(char ** user_array) {
    char dir_path[256];

    if(user_array[1] == NULL) {
        chdir(getenv("HOME"));
    }
    else {
        chdir(user_array[1]);
    }
    getcwd(dir_path, sizeof(dir_path));
    printf("%s\n", dir_path);
    fflush(stdout);
}

void exitCommand(char ** array) {
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

void statusCommand(char ** array) {
    // Print out exit status or the terminating signal of the last foreground process
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

void redirectCommand(char ** array, int numStr) {
    int inputFile;
    int outputFile;
    int result;
    int i;
    
    // using > or < to redirection
    // dup2()
    // cat < red needs to become cat
    // cat < red > output also n eeds to become cat
    // background process redirect to /dev/null
    // open the file to /dev/null, dup2(), stdin = 0;

    if(ambExist(array, numStr) == true) {
        for(i = numStr-1; i > 0; i--) {
            if(strcmp(array[i-2], "<") == 0) {
                inputFile = open(array[i-1], O_RDONLY);
                if(inputFile < 0) {
                    printf("Input file could not be opened for reading.\n");
                    // fflush();
                    exit(1); // How to not exit the shell??
                }
                else {
                    result = dup2(inputFile, 0);
                }
            }
                    
            // redirect to stdout
            if(strcmp(array[i-2], ">") == 0) {
                outputFile = open(array[i-1], O_WRONLY | O_CREAT | O_TRUNC); // These will go into dup
                if(outputFile < 0) {
                    printf("Input file could not be opened for writing.\n");
                    // fflush();
                    exit(1); // How to not exit the shell??
                }
                else {
                    result = dup2(outputFile, 1);
                }
            }
        }
        execvp(array[0], array);
    }
    else {
        for(i = numStr; i > 0; i--) {
            // redirect to stdin
            if(strcmp(array[i-2], "<") == 0) {
                inputFile = open(array[i-1], O_RDONLY);
                if(inputFile < 0) {
                    printf("Input file could not be opened for reading.\n");
                    // fflush();
                    exit(1); // How to not exit the shell??
                }
                else {
                    result = dup2(inputFile, 0);
                }
            }
            
            // redirect to stdout
            if(strcmp(array[i-2], ">") == 0) {
                outputFile = open(array[i-1], O_WRONLY | O_CREAT | O_TRUNC); // These will go into dup
                if(outputFile < 0) {
                    printf("Input file could not be opened for writing.\n");
                    // fflush();
                    exit(1); // How to not exit the shell??
                }
                else {
                    result = dup2(outputFile, 1);
                }
            }
        }
        execvp(array[0], array);
    }
}


bool notBulitIn(char ** array) {
    if((strcmp(array[0], "cd") != 0) && (strcmp(array[0], "exit") != 0) && (strcmp(array[0], "status") != 0)) {
        return true;    
    }
    return false;
}

// using > or < to redirection
// dup2()
// cat < red needs to become cat
// cat < red > output also n eeds to become cat
// background process redirect to /dev/null
// open the file to /dev/null, dup2(), stdin = 0;
