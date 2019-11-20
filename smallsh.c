#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>

int MAX_LENGTH_OF_STR = 2048;
int MAX_NUM_OF_ARGS = 512;
pid_t pid_array[1000];
int pid_index = 0;

char ** createArr();
bool signExists(char * user_input);
char * expandUserInput(char * user_input);
int countWords(char * user_input);
bool ifValidArgNum(char * user_input);
bool ambExist(char ** array, int num_of_strings);
bool hasRedirection(char ** user_array, int num_of_strings);
void removeRedirection(char ** user_array, int num_of_strings, int index);
void cdCommand(char ** user_array);
void statusCommand(int childExitedMethod);
void exitCommand();
void redirectFore(char ** user_array, int num_of_strings);
void redirectBack(char ** user_array, int num_of_strings);
void nonBuiltCommand(char ** user_array, int num_of_strings, int * childExitedMethod);

int main() {
    char * user_input = NULL;
    char * expanded_user_input = NULL;
    char ** user_array = createArr();
    size_t input_size = MAX_LENGTH_OF_STR;
    int i = 0;
    int num_of_strings = 0;
    int childExitedMethod = -5;

    do {
        printf(": ");
        fflush(stdout);
        getline(&user_input, &input_size, stdin);
        expanded_user_input = expandUserInput(user_input);
        num_of_strings = countWords(expanded_user_input);
        
        if(ifValidArgNum(expanded_user_input) != true) {
            printf("Input length exceeds the limit.\n");
            fflush(stdout);
            continue;
        }
        
        if(expanded_user_input[0] == '#') {
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
        else if(strcmp(user_array[0], "status") == 0) {
            statusCommand(childExitedMethod);
        }
        else if(strcmp(user_array[0], "exit") == 0) {
            printf("This is before exit command is called.\n");
            exitCommand();
        }
        else {
            nonBuiltCommand(user_array, num_of_strings, &childExitedMethod);
        }
        
        int pid = 0;
        int exitState = 0;
        int exit = 0;
        int sig = 0;
        while((pid = waitpid(-1, &exitState, WNOHANG)) > 0){
            if(WIFEXITED(exitState) != 0) {
                exit = WEXITSTATUS(exitState);
                printf("exit value %d\n", exit);
                fflush(stdout);
            }
            else {
                sig = WTERMSIG(exitState);
                printf("terminated by signal %d\n", sig); // being printed twice... why?
                fflush(stdout);
            }
        }
    } while (strcmp(user_input, "exit") != 0);
    
    return 0;
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

bool signExists(char * user_input) {
    int i;
    for(i = 0; i < strlen(user_input); i++) {
        if((user_input[i] == '$') && (user_input[i+1] == '$')) {
            return true;
        }
    }
    return false;
}

char * expandUserInput(char * user_input) { // This should be the string that we use for the rest of the program
    pid_t pid = getpid();
    char newString[MAX_LENGTH_OF_STR]; // MIGHT NEED TO INCREASE THIS INCASE IT OVERFLOWS
    while(signExists(user_input) == true) {
        char * dds = strstr(user_input, "$$");
        dds[0] = '%';
        dds[1] = 'd';
        sprintf(newString, user_input, (int)pid);
        strcpy(user_input, newString);
    }
    return user_input;
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

bool ifValidArgNum(char * user_input) {
    int count = 1;
    int i;
    for(i = 0; i < strlen(user_input); i++) {
        if((user_input[i] == ' ') && (user_input[i+1] != ' ')) {
            count++;
        }
    }
    if(count <= MAX_NUM_OF_ARGS) {
        return true;
    }
    return false;
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

void removeRedirection(char ** user_array, int num_of_strings, int index) {
    int i;
    for(i = index; i < num_of_strings; i++) {
        user_array[i] = NULL;
    }
}

void removeAmb(char ** user_array, int num_of_strings) {
    if(strcmp(user_array[num_of_strings-1], "&") == 0) {
        user_array[num_of_strings-1] = NULL;
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

void exitCommand() {
    int i;
    for(i = 0; i < pid_index; i++) {
        kill(pid_array[i], SIGKILL);
    }
    exit(0);
}

void redirectFore(char ** user_array, int num_of_strings) {
    int inputFile = 0;
    int outputFile = 0;
    int inResult = 0;
    int outResult = 0;
    int i;
    for(i = num_of_strings-1; i >= 0; i--) {
        if(strcmp(user_array[i], "<") == 0) {
            inputFile = open(user_array[i+1], O_RDONLY);
            if(inputFile < 0) {
                printf("Input file could not be opened for reading.\n");
                fflush(stdout);
                exit(1); // How to not exit the shell??
            }
            inResult = dup2(inputFile, 0);
            if(inResult == -1) {
                printf("dup2(inputFile, 0) failed.\n");
                fflush(stdout);
                exit(1);
            }
            removeRedirection(user_array, num_of_strings, i);
        }
        else if(strcmp(user_array[i], ">") == 0) {
            outputFile = open(user_array[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0770);
            if(outputFile < 0) {
                printf("Input file could not be opened for writing.\n");
                fflush(stdout);
                exit(1);
            }
            outResult = dup2(outputFile, 1);
            if(outResult == -1) {
                printf("dup2(outputFile, 1) failed.\n");
                fflush(stdout);
                exit(1);
            }
            removeRedirection(user_array, num_of_strings, i);
        }
        else {
            continue;
        }
    }
}

void redirectBack(char ** user_array, int num_of_strings) {
    int devNullInput = 0;
    int devNullOutput = 0;
    int inputFile = 0;
    int outputFile = 0;
    int inDevNullResult = 0;
    int outDevNullResult = 0;
    int inResult = 0;
    int outResult = 0;
    
    if(strcmp(user_array[num_of_strings-1], "&") == 0) {
        user_array[num_of_strings-1] = NULL;
    }
    
    int i;
    for(i = num_of_strings-1; i >= 0; i--) {        
        if(strcmp(user_array[i], "<") == 0) {
            if(user_array[i+1] == NULL) {
                devNullInput = open("/dev/null", 0);
                if(devNullInput == -1) {
                    printf("Input file could not be opened for reading.\n");
                    fflush(stdout);
                    exit(1);
                }
                inDevNullResult = dup2(devNullInput, 0);
                if(inDevNullResult == -1) {
                    printf("dup2(devNullInput, 0) failed.\n");
                    fflush(stdout);
                    exit(1);
                }
                removeRedirection(user_array, num_of_strings, i);
            }
            else {
                inputFile = open(user_array[i+1], O_RDONLY);
                if(inputFile == -1) {
                    printf("Input file could not be opened for reading.\n");
                    fflush(stdout);
                    exit(1); // How to not exit the shell??
                }
                inResult = dup2(inputFile, 0);
                if(inResult == -1) {
                    printf("dup2(inputFile, 0) failed.\n");
                    fflush(stdout);
                    exit(1);
                }
                removeRedirection(user_array, num_of_strings, i);
            }
        }
        else if(strcmp(user_array[i], ">") == 0) {
            if(user_array[i+1] == NULL) {
                devNullOutput = open("/dev/null", 1);
                if(devNullOutput == -1) {
                    printf("Input file could not be opened for writing.\n");
                    fflush(stdout);
                    exit(1); // How to not exit the shell??
                }
                outDevNullResult = dup2(devNullOutput, 1);
                if(outDevNullResult == -1) {
                    printf("dup2(devNullOutput, 1) failed.\n");
                    fflush(stdout);
                    exit(1);
                }
                removeRedirection(user_array, num_of_strings, i);
            }
            else {
                outputFile = open(user_array[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if(outputFile == -1) {
                    printf("Input file could not be opened for writing.\n");
                    fflush(stdout);
                    exit(1);
                }
                outResult = dup2(outputFile, 1);
                if(outResult == -1) {
                    printf("dup2(outputFile, 1) failed.\n");
                    fflush(stdout);
                    exit(1);
                }
                removeRedirection(user_array, num_of_strings, i);
            }
        }
        else {
            continue;
        }
    }
}

void nonBuiltCommand(char ** user_array, int num_of_strings, int * childExitedMethod) {
    pid_t spawnpid = -5;
    
    spawnpid = fork();
    switch (spawnpid) {
        case -1:
            perror("Error with forking.");
            exit(1);
            break;
        case 0:
            if(hasRedirection(user_array, num_of_strings) == true) {
                if(ambExist(user_array, num_of_strings) == true) {
                    pid_array[pid_index] = spawnpid;
                    pid_index++;
                    redirectBack(user_array, num_of_strings);
                }
                else {
                    redirectFore(user_array, num_of_strings);
                }
            }
            execvp(user_array[0], user_array);
            if(execvp(user_array[0], user_array) == -1) {
                printf("%s: no such file or directory.\n", user_array[0]);
                fflush(stdout);
            }          
            break;
        default:
            if(ambExist(user_array, num_of_strings) == true) {
                waitpid(spawnpid, childExitedMethod, WNOHANG);
            }
            else {
                waitpid(spawnpid, childExitedMethod, 0);
            }
            break;
    }
}





