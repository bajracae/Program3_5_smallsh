#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <fcntl.h>
#include <signal.h>

int MAX_LENGTH_OF_STR = 2048;
int MAX_NUM_OF_ARGS = 512;

bool ifValidArgNum(char * input);
bool signExists(char * input);
char * expandUserInput(char * input);
char ** commandArr(char ** array, char * input, int * num_of_strings);
void printArr(char ** array, int num_of_strings);
void cdCommand(char ** array); 
void exitCommand();
char ** createArr();
bool isComment(char ** array);
bool ambExist(char ** array, int num_of_strings);
void nonBuiltBackground(char ** user_array, int num_of_strings);
void nonBuiltForeground(char ** user_array, int num_of_strings, int * childExitedMethod);
bool hasRedirection(char ** user_array, int num_of_strings);
void removeRedirection(char ** user_array, int num_of_strings);
void statusCommand(int childExitedMethod);

int main() {
    int num_of_strings = 0;
    int childExitedMethod = -5;
    char * user_input = NULL;
    char * expanded_user_input = NULL;
    char ** user_array = createArr();
    int i;
    size_t input_size = MAX_LENGTH_OF_STR;
    pid_t pid_array[1000];
    
    // Catch ctrl-C to terminate foreground
    // https://stackoverflow.com/questions/1641182/how-can-i-catch-a-ctrl-c-event
    // struct sigaction controlCHandler;
    // sigaction(SIGINT, &controlCHandler, NULL);
    // create a function that prints out the error
    
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
            exitCommand();
        }
        else if(strcmp(user_array[0], "status") == 0) {
            statusCommand(childExitedMethod);
        }
        else {
            if(ambExist(user_array, num_of_strings)) {
                nonBuiltBackground(user_array, num_of_strings);
            }
            else {
                nonBuiltForeground(user_array, num_of_strings, &childExitedMethod);
            }
            // continue;
        }    
    } while(strcmp(user_input, "exit") != 0);
    
    
    return 0;
}

void nonBuiltForeground(char ** user_array, int num_of_strings, int * childExitedMethod) {
    pid_t spawnpid = -5;
    // int childExitedMethod = 0;
    int inputFile = 0;
    int outputFile = 0;
    int result = 0;
    int i;

    spawnpid = fork();
    switch (spawnpid) {
        case -1: // if error in forking()
            perror("ERROR WHILE FORKING");
            exit(1);
            break;
        case 0: // child process stuff runs here 
            if(hasRedirection(user_array, num_of_strings) == true) {
                for(i = num_of_strings-1; i >= 0; i--) {
                    // redirect to stdin
                    if(strcmp(user_array[i], "<") == 0) {
                        inputFile = open(user_array[i+1], O_RDONLY);
                        if(inputFile == -1) {
                            printf("Input file could not be opened for reading.\n");
                            fflush(stdout);
                            exit(1); // How to not exit the shell??
                        }
                        result = dup2(inputFile, 0);
                        if(result == -1) {
                            printf("(error with stdin)\n");
                            fflush(stdout);
                        }
                    }
                    // redirect to stdout
                    else if(strcmp(user_array[i], ">") == 0) {
                        outputFile = open(user_array[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644); // These will go into dup
                        if(outputFile == -1) {
                            printf("Input file could not be opened for writing.\n");
                            fflush(stdout);
                            exit(1); // How to not exit the shell??
                        }
                        result = dup2(outputFile, 1);
                        if(result == -1) {
                            printf("(error with stdout)\n");
                            fflush(stdout);
                        }
                    }
                    else {
                        continue;
                    }
                }
            }
            removeRedirection(user_array, num_of_strings);
            printf("I ran here\n");
            execvp(user_array[0], user_array);
            break;
        default: // parent process
            waitpid(spawnpid, childExitedMethod, 0); // Block the parent until the child process with specified PID terminates
            break;
    }
}

void nonBuiltBackground(char ** user_array, int num_of_strings) {
    pid_t spawnpid = -5;
    int childExitedMethod = 0;
    int inputFile = 0;
    int outputFile = 0;
    int devNULLInput = 0;
    int devNULLOutput = 0;
    int result = 0;
    int i;
    
    spawnpid = fork();
    switch (spawnpid) {
        case -1:
            perror("ERROR WHILE FORKING");
            exit(1);
            break;
        case 0:
            // print out the background PID here;
            printf("background pid is %d\n", getpid());
            if(ambExist(user_array, num_of_strings) == true) {
                if(hasRedirection(user_array, num_of_strings) == true) {
                    for(i = num_of_strings-1; i >= 0; i--) {
                        if(strcmp(user_array[i], "<") == 0) {
                            if((strcmp(user_array[i+1], "&") == 0)) {
                                // redirect to /dev/null
                                devNULLInput = open("/dev/null", O_RDONLY);
                                if(devNULLInput == -1) {
                                    printf("Input file could not be opened for reading.\n");
                                    fflush(stdout);
                                    exit(1); // How to not exit the shell??
                                }
                                result = dup2(devNULLInput, 0);
                                if(result == -1) {
                                    printf("(error with stdin)\n");
                                    fflush(stdout);
                                }
                            }
                            else {
                                inputFile = open(user_array[i+1], O_RDONLY);
                                if(inputFile == -1) {
                                    printf("Input file could not be opened for reading.\n");
                                    fflush(stdout);
                                    exit(1); // How to not exit the shell??
                                }
                                result = dup2(inputFile, 0);
                                if(result == -1) {
                                    printf("(error with stdin)\n");
                                    fflush(stdout);
                                }
                            }
                        }
                        else if(strcmp(user_array[i], ">") == 0) {
                            if((strcmp(user_array[i+1], "&") == 0)) {
                                devNULLOutput = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0644);
                                if(devNULLOutput == -1) {
                                    printf("Input file could not be opened for reading.\n");
                                    fflush(stdout);
                                    exit(1); // How to not exit the shell??
                                }
                                result = dup2(devNULLOutput, 0);
                                if(result == -1) {
                                    printf("(error with stdin)\n");
                                    fflush(stdout);
                                }
                            }
                            else {
                                outputFile = open(user_array[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                                if(outputFile == -1) {
                                    printf("Input file could not be opened for writing.\n");
                                    fflush(stdout);
                                    exit(1); // How to not exit the shell??
                                }
                                result = dup2(outputFile, 1);
                                if(result == -1) {
                                    printf("(error with stdout)\n");
                                    fflush(stdout);
                                }
                            }
                        }
                        else {
                            continue;
                        }
                    }
                }
                removeRedirection(user_array, num_of_strings);
                execvp(user_array[0], user_array);
            }
            break;
        case 1:
            waitpid(spawnpid, &childExitedMethod, WNOHANG);
            break;
    }
}

void removeRedirection(char ** user_array, int num_of_strings) {
    int i;
    int count = 0;
    for(i = 0; i < num_of_strings; i++) {
        if((strcmp(user_array[i], ">") == 0) || (strcmp(user_array[i], "<") == 0)) {
            count = i;
            break;
        }
    }
    if(count != 0) {
        for(count; count < num_of_strings; count++) {
            user_array[count] = NULL;
        }
    }
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

bool signExists(char * input) {
    int i;
    for(i = 0; i < strlen(input); i++) {
        if((input[i] == '$') && (input[i+1] == '$')) {
            return true;
        }
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

void exitCommand(char ** user_array, pid_t * pid_array, int pid_array_len) {
    // Kill everything
    // kill child and foreground processes
    // holder for all pid of child processes
    // run sigkill for each child process

    int i;
    if(user_array[1] == NULL) {
        for(i = 0; i < size; i++) {
            if(pid_array[i] != 0) {
                
            }
        }
        exit(0);
    }
    else {
        printf("ERROR\n");
        fflush(stdout);
    }
}

void statusCommand(int childExitedMethod) {
    int termSig = 0;
    int exitStat = 0;
    // LECTURE 3.1 Processes
    if(WIFSIGNALED(childExitedMethod) != 0) {
        termSig = WTERMSIG(childExitedMethod);
        printf("terminated by signal %d\n", termSig);
    }
    else {
        exitStat = WEXITSTATUS(childExitedMethod);
        printf("exit value %d\n", exitStat);
    }
}

bool ambExist(char ** array, int num_of_strings) {
    if(strcmp(array[num_of_strings-1], "&") == 0){
        return true;
    }
    return false;
}

bool hasRedirection(char ** user_array, int num_of_strings) {
    int i;
    for(i = 0; i < num_of_strings; i++) {
        if((strcmp(user_array[i], "<") == 0) || (strcmp(user_array[i], ">") == 0)){
            return true;
        }
    }
    return false;
}
