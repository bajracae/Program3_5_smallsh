///////////////////////////////////////////////////////////////////
// Author: Aeijan Bajracharya
// Title: Program 3 - smallsh
// Description: This program runs a small version of the bash shell.
// Date: 11/21/19
///////////////////////////////////////////////////////////////////

// LIBRARIES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

// GLOBAL VARIABLES
int MAX_LENGTH_OF_STR = 2048;
int MAX_NUM_OF_ARGS = 512;
pid_t pid_array[1000];
int pid_index = 0;
struct sigaction SIGINT_action;
struct sigaction SIGTSTP_action;
bool isAllowBackground = true;

///////////////////////////////////////////////////////////////////
// Source: Linux Programming Blog
// Author: daper
// Date: 2009
// Code version: N/A
// Availability: https://www.linuxprogrammingblog.com/code-examples
///////////////////////////////////////////////////////////////////
sigset_t mask;

void catchSIGTSTP(int signo);
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
    
    ///////////////////////////////////////////////////////////////////
	// Source: Lecture 3.3 Signals
	// Author: Benjamin Brewster
	// Date: 2017
	// Code version: N/A
	// Availability: http://web.engr.oregonstate.edu/~brewsteb/CS344Slides/3.3%20Signals.pdf
	///////////////////////////////////////////////////////////////////
    // CONTROL C COMMAND
    SIGINT_action.sa_handler = SIG_IGN;
    sigfillset(&SIGINT_action.sa_mask);
    SIGINT_action.sa_flags = SA_RESTART;
    sigaction(SIGINT, &SIGINT_action, NULL);
    
    // CONTROL Z COMMAND
    SIGTSTP_action.sa_handler = catchSIGTSTP;
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

    do {
        printf(": ");
        fflush(stdout);
        getline(&user_input, &input_size, stdin); // Get user input
        expanded_user_input = expandUserInput(user_input); // Change all $$ to pid
        num_of_strings = countWords(expanded_user_input); // Get the number of arguments entered
        
        if(ifValidArgNum(expanded_user_input) != true) { // Check if the number of arguments is 512 or less
            printf("Input length exceeds the limit.\n");
            fflush(stdout);
            continue;
        }
        
        if(expanded_user_input[0] == '#') { // If the first character of the first argument is #, loop again
            continue;
        }
        
        i = 1;
        
        // Place each string from the input into the array until NULL terminator is found
        user_array[0] = strtok(expanded_user_input, " \n");    
        while((user_array[i] = strtok(NULL, " \n"))) {
            if(user_array[i][0]!=0){
                i++;
            }
        }
        if(!user_array[0]){ // If no user input is found, set i to 0
            i = 0;
        }
        num_of_strings=i; // set the num_of_string variable to i
        
        // // DELETE THIS
        // char** a = user_array;
        // while(*a){
        //     printf("\n'%s'",*a);
        //     a++;
        // }
        // 
        // // DELETE THIS
        // if(ambExist(user_array, num_of_strings)) { 
        //     printf("user_array: %s\n", user_array[num_of_strings-1]);
        // }
        
        if((user_array[0] == NULL) || (strcmp(user_array[0], "#") == 0)) { // If user input is blank or if the first character in the user input array is "#"
            continue;
        }
        if(strcmp(user_array[0], "cd") == 0) { // If user enters "cd"
            cdCommand(user_array);
        }
        else if(strcmp(user_array[0], "status") == 0) { // If user enters "status"
            statusCommand(childExitedMethod);
        }
        else if(strcmp(user_array[0], "exit") == 0) { // If user enters "exit"
            exitCommand();
        }
        else {
            nonBuiltCommand(user_array, num_of_strings, &childExitedMethod); // Else commands that were not built in
        }
        
        int pid = 0;
        int exitState = 0;
        int exit = 0;
        int sig = 0;
        while((pid = waitpid(-1, &exitState, WNOHANG)) > 0){ // While child processes exist
            if(WIFEXITED(exitState) != 0) { // If the child terminates normally
                exit = WEXITSTATUS(exitState);
                printf("background process %d is done. ", pid);
                fflush(stdout);
                printf("exit value %d\n", exit); // Print out the exit value
                fflush(stdout);
            }
            else {
                sig = WTERMSIG(exitState); // If the child terminates with a signal
                printf("background process %d was successfully killed. ", pid);
                fflush(stdout);
                printf("terminated by signal %d\n", sig); // Print out the terminating signal
                fflush(stdout);
            }
        }
    } while (strcmp(user_input, "exit") != 0); // While user does not enter "exit," keep running
    
    return 0;
}

///////////////////////////////////////////////////////////////////
// Function: void catchSIGTSTP(int signo)
// Description: When user enters ^Z, run this command and print out 
// the correct message
// Parameters: int signo
// Pre-Conditions: takes in the signal number
// Post-Conditions: Prints out message_enter when the user enters ^Z
// the first time, then print out message_exit when the user enters it
// again.
///////////////////////////////////////////////////////////////////
void catchSIGTSTP(int signo) {
    char * message_enter = "Entering foreground-only mode (& is now ignored)\n\n";
    char * message_exit = "Exiting foreground-only mode\n\n";
    if(isAllowBackground == true) { // If background processes are allowed
        write(STDOUT_FILENO, message_enter, 49); // Print this message
        isAllowBackground = false; // Set the flag to false
    }
    else {
        write(STDOUT_FILENO, message_exit, 29); // If background processes are not allowed
        isAllowBackground = true; // Set the flag to true
    }
}

///////////////////////////////////////////////////////////////////
// Function: char ** createArr()
// Description: Create an array to store the user input array
// Parameters: N/A
// Pre-Conditions: N/A
// Post-Conditions: Returns a cstring array
///////////////////////////////////////////////////////////////////
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

///////////////////////////////////////////////////////////////////
// Function: bool signExists(char * user_input)
// Description: Checks if "$$" exists in the string
// Parameters: char * user_input
// Pre-Conditions: Takes in the user input
// Post-Conditions: Return true if "$$" exists, else return false
///////////////////////////////////////////////////////////////////
bool signExists(char * user_input) {
    int i;
    for(i = 0; i < strlen(user_input); i++) {
        if((user_input[i] == '$') && (user_input[i+1] == '$')) {
            return true;
        }
    }
    return false;
}

///////////////////////////////////////////////////////////////////
// Function: char * expandUserInput(char * user_input)
// Description: Checks if "$$" exists in the string
// Parameters: char * user_input
// Pre-Conditions: Takes in the user input
// Post-Conditions: Return true if "$$" exists, else return false
///////////////////////////////////////////////////////////////////
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

///////////////////////////////////////////////////////////////////
// Function: int countWords(char * user_input)
// Description: Counts the number of strings in the user input and
// returns it
// Parameters: char * user_input
// Pre-Conditions: Takes in the user input
// Post-Conditions: Return the number of strings in the user input
///////////////////////////////////////////////////////////////////
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

///////////////////////////////////////////////////////////////////
// Function: bool ifValidArgNum(char * user_input)
// Description: C
// Parameters: char * user_input
// Pre-Conditions: Takes in the user input
// Post-Conditions: Return the number of strings in the user input
///////////////////////////////////////////////////////////////////
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
    if (num_of_strings == 0 ){
        return false;
    }
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
        num_of_strings--;
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
        fflush(stdout);
    }
    else {
        exitStat = WEXITSTATUS(childExitedMethod);
        printf("exit value %d\n", exitStat);
        fflush(stdout);
    }
}

void exitCommand() {
    int i;
    for(i = 0; i < pid_index; i++) {
        kill(pid_array[i], SIGTERM);
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
    
    int i;
    for(i = num_of_strings-2; i >= 0; i--) {        
        if(strcmp(user_array[i], "<") == 0) {
            if(strcmp(user_array[i+1], "&") == 0) {
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
            if(strcmp(user_array[i+1], "&") == 0) {
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
    int execVal = 0;
    
    sigemptyset(&mask);
    sigaddset(&mask, SIGTSTP);
    sigprocmask(SIG_BLOCK, &mask, NULL);
    
    spawnpid = fork();
    switch (spawnpid) {
        case -1:
            perror("Error with forking.");
            exit(1);
            break;
        case 0:
            if( ambExist(user_array, num_of_strings)  && isAllowBackground ) {
                if(hasRedirection(user_array, num_of_strings) == true) {
                    pid_array[pid_index] = spawnpid;
                    pid_index++;
                    if(strcmp(user_array[num_of_strings-1], "&") == 0) {
                        user_array[num_of_strings-1] = NULL;
                        num_of_strings--;
                    }
                    redirectBack(user_array, num_of_strings);
                }
                else {
                    if(strcmp(user_array[num_of_strings-1], "&") == 0) {
                        user_array[num_of_strings-1] = NULL;
                        num_of_strings--;
                    }
                }
            }
            else {
                
                SIGINT_action.sa_handler = SIG_DFL;
                sigaction(SIGINT, &SIGINT_action, NULL);
                if(strcmp(user_array[num_of_strings-1], "&") == 0) {
                    user_array[num_of_strings-1] = NULL;
                    num_of_strings--;
                }
                if(hasRedirection(user_array, num_of_strings) == true) {
                    redirectFore(user_array, num_of_strings);
                }
            }

            execVal = execvp(user_array[0], user_array);
            if(execVal == -1) {
                printf("%s: %s\n", user_array[0], strerror(errno));
                fflush(stdout);
                exit(2);
            }          
            break;
        default:
            sigprocmask(SIG_UNBLOCK, &mask, NULL);
            if(ambExist(user_array, num_of_strings) && isAllowBackground ) {
                printf("background pid is %d\n", spawnpid);
                fflush(stdout);
                waitpid(spawnpid, childExitedMethod, WNOHANG);
            }
            else {
                waitpid(spawnpid, childExitedMethod, 0);
            }
            break;
    }
}





