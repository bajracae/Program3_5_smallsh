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
int mostRecentProcessPid = -10;
pid_t parentPID = -5;

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
    
    ///////////////////////////////////////////////////////////////////
    // Source: Lecture 3.3 Signals
    // Author: Benjamin Brewster
    // Date: 2017
    // Code version: N/A
    // Availability: http://web.engr.oregonstate.edu/~brewsteb/CS344Slides/3.3%20Signals.pdf
    ///////////////////////////////////////////////////////////////////
    // CONTROL Z COMMAND
    SIGTSTP_action.sa_handler = catchSIGTSTP;
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

    parentPID = getpid();

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
        if((user_input[i] == '$') && (user_input[i+1] == '$')) { // If "$$" exists in the string
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
    pid_t pid = getpid(); // Get the pid of the process
    char newString[MAX_LENGTH_OF_STR]; // String to store the extended user input
    while(signExists(user_input) == true) { // While there are "$$"
        
        // Finds the "$$" and store it into a char* variable
        char * dds = strstr(user_input, "$$"); 
        
        // Replace "$" (first) with "%" and "$" (second) with "d"
        dds[0] = '%';
        dds[1] = 'd';
        
        // "%d" is replaced by the pid
        // Extended string is copied into user_input
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
        if(user_input[i] == ' ' || user_input[i] == '\0') { // Increment num_of_strings if there is a " " or "\0" in the user input
            num_of_strings++;
        }
    }
    return num_of_strings;
}

///////////////////////////////////////////////////////////////////
// Function: bool ifValidArgNum(char * user_input)
// Description: Checks the number of argumements
// Parameters: char * user_input
// Pre-Conditions: Takes in user input
// Post-Conditions: Returns true the number of args is 512 or less
///////////////////////////////////////////////////////////////////
bool ifValidArgNum(char * user_input) {
    int count = 1;
    int i;
    for(i = 0; i < strlen(user_input); i++) {
        if((user_input[i] == ' ') && (user_input[i+1] != ' ')) { // If there is a space and a character afterwards, increment the count
            count++;
        }
    }
    if(count <= MAX_NUM_OF_ARGS) { // If the count is 512 or less
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////
// Function: bool ambExist(char ** array, int num_of_strings)
// Description: Checks if an & exists in the user input
// Parameters: char ** array, int num_of_strings
// Pre-Conditions: Takes in the user input array and the number of
// strings in the array
// Post-Conditions: Return true if there is an &
///////////////////////////////////////////////////////////////////
bool ambExist(char ** array, int num_of_strings) {
    if (num_of_strings == 0 ){ // If the user input is a length of 0, return false
        return false;
    }
    if(strcmp(array[num_of_strings-1], "&") == 0){ // If the last character of a string is an & then return true
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////
// Function: bool hasRedirection(char ** user_array, int num_of_strings)
// Description: Checks if there is redirection character in the user input
// Parameters: char ** array, int num_of_strings
// Pre-Conditions: Takes in the user input array and the number of
// strings in the array
// Post-Conditions: Return true if there is a redirection sign in the strings
///////////////////////////////////////////////////////////////////
bool hasRedirection(char ** user_array, int num_of_strings) {
    int i;
    for(i = 0; i < num_of_strings; i++) {
        if((strcmp(user_array[i], "<") == 0) || (strcmp(user_array[i], ">") == 0)){ // If user_array[i] is ">" or "<," return true
            return true;
        }
    }
    return false;
}

///////////////////////////////////////////////////////////////////
// Function: void removeRedirection(char ** user_array, int num_of_strings, int index)
// Description: Sets everything to the right of index to NULL;
// Parameters: char ** array, int num_of_strings, int index
// Pre-Conditions: Takes in the user input array and the number of
// strings in the array and an index
// Post-Conditions: Change the user input array so there are NULLs at the right of a redirection
///////////////////////////////////////////////////////////////////
void removeRedirection(char ** user_array, int num_of_strings, int index) {
    int i;
    for(i = index; i < num_of_strings; i++) { // Starting at index, set everything to the right of it to NULL 
        user_array[i] = NULL;
    }
}

///////////////////////////////////////////////////////////////////
// Function: void removeAmb(char ** user_array, int num_of_strings)
// Description: Removes the & at the end of user input
// Parameters: char ** array, int num_of_strings
// Pre-Conditions: Takes in the user input array and the number of
// strings in the array
// Post-Conditions: Removes the & if there is one at the end of the string
///////////////////////////////////////////////////////////////////
void removeAmb(char ** user_array, int num_of_strings) {
    if(strcmp(user_array[num_of_strings-1], "&") == 0) { // If the last character is an &, set it to NULL
        user_array[num_of_strings-1] = NULL;
        num_of_strings--; // Decrement the num_of_strings
    }
}

///////////////////////////////////////////////////////////////////
// Function: void cdCommand(char ** user_array)
// Description: Run the cd command when the user inputs it
// Parameters: char ** user_array
// Pre-Conditions: Takes in the user input array
// Post-Conditions: Changes directory when the command is entered
///////////////////////////////////////////////////////////////////
void cdCommand(char ** user_array) {
    char dir_path[256];
    if(user_array[1] == NULL) {
        chdir(getenv("HOME")); // Change directory to the home directory if the user inputs "cd"
    }
    else {
        chdir(user_array[1]); // Else change to the directory named in the second arg
    }
    getcwd(dir_path, sizeof(dir_path));
    printf("%s\n", dir_path); // Prints the directory path
    fflush(stdout);
}

///////////////////////////////////////////////////////////////////
// Function: void statusCommand(int childExitedMethod)
// Description: Run the status command when the user inputs it
// Parameters: int childExitedMethod
// Pre-Conditions: Takes in the status int
// Post-Conditions: Prints out the status int depending on if the process
// was killed by a signal or exited
///////////////////////////////////////////////////////////////////
void statusCommand(int childExitedMethod) {
    int termSig = 0;
    int exitStat = 0;
    ///////////////////////////////////////////////////////////////////
    // Source: Lecture 3.1 Processes
    // Author: Benjamin Brewster
    // Date: 2017
    // Code version: N/A
    // Availability: http://web.engr.oregonstate.edu/~brewsteb/CS344Slides/3.1%20Processes.pdf
    ///////////////////////////////////////////////////////////////////
    if(mostRecentProcessPid == -10) {
        printf("No foreground process ran yet. exit value 0\n");
        fflush(stdout);
        return;
    }
    if(WIFSIGNALED(childExitedMethod) != 0) { // If the child was terminated by a signal
        termSig = WTERMSIG(childExitedMethod);
        printf("terminated by signal %d\n", termSig); // Print the termination signal
        fflush(stdout);
    }
    else { // If the child exits
        exitStat = WEXITSTATUS(childExitedMethod);
        printf("exit value %d\n", exitStat); // Print the exit value
        fflush(stdout);
    }
}

///////////////////////////////////////////////////////////////////
// Function: void exitCommand()
// Description: Kills the child processes in the pid_t array
// Parameters: N/A
// Pre-Conditions: Takes in the pid_array and pid_index
// Post-Conditions: Kills all the processes with the pids in the array
///////////////////////////////////////////////////////////////////
void exitCommand() {
    int i;
    for(i = 0; i < pid_index; i++) { // Iterate through all the pids and terminate them
        kill(pid_array[i], SIGTERM); 
    }
    exit(0);
}

///////////////////////////////////////////////////////////////////
// Function: void redirectFore(char ** user_array, int num_of_strings)
// Description: Run the redirection in the foreground
// Parameters: char ** user_array, int num_of_strings
// Pre-Conditions: Takes in the user input array and the number of strings
// Post-Conditions: Runs the redirection in the foreground
///////////////////////////////////////////////////////////////////
void redirectFore(char ** user_array, int num_of_strings) {
    int inputFile = 0;
    int outputFile = 0;
    int inResult = 0;
    int outResult = 0;
    int i;
    for(i = num_of_strings-1; i >= 0; i--) { // Loop through the user input array
        if(strcmp(user_array[i], "<") == 0) { // If "<" is found
            inputFile = open(user_array[i+1], O_RDONLY); // Open a file with the name given to the right of the "<"
            if(inputFile < 0) { // If the file can't be opened for reading, print an error
                printf("Input file could not be opened for reading.\n");
                fflush(stdout);
                exit(1); 
            }
            inResult = dup2(inputFile, 0); // Creates a copy of the file descriptor 
            if(inResult == -1) { // If dup2 fails, print an error
                printf("dup2(inputFile, 0) failed.\n");
                fflush(stdout);
                exit(1);
            }
            removeRedirection(user_array, num_of_strings, i); // Set the redirection sign and anything afterwards 
        }
        else if(strcmp(user_array[i], ">") == 0) { // If ">" is found
            outputFile = open(user_array[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0770); // Open a file with the name given to the right of the ">"
            if(outputFile < 0) { // If the file can't be opened for writing, print an error
                printf("Input file could not be opened for writing.\n");
                fflush(stdout);
                exit(1);
            }
            outResult = dup2(outputFile, 1); // Creates a copy of the file descriptor 
            if(outResult == -1) { // If dup2 fails, print an error
                printf("dup2(outputFile, 1) failed.\n");
                fflush(stdout);
                exit(1);
            }
            removeRedirection(user_array, num_of_strings, i); // Set the redirection sign and anything afterwards
        }
        else {
            continue;
        }
    }
}

///////////////////////////////////////////////////////////////////
// Function: void redirectBack(char ** user_array, int num_of_strings)
// Description: Run the redirection in the background
// Parameters: char ** user_array, int num_of_strings
// Pre-Conditions: Takes in the user input array and the number of strings
// Post-Conditions: Runs the redirection in the background
///////////////////////////////////////////////////////////////////
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
    for(i = num_of_strings-2; i >= 0; i--) { // Loop through the user input array       
        if(strcmp(user_array[i], "<") == 0) { // If "<" is found
            if(strcmp(user_array[i+1], "&") == 0) { // If there is a "&" after it
                devNullInput = open("/dev/null", 0); // Open the /dev/null
                if(devNullInput == -1) { // If the file can't be opened for reading, print an error
                    printf("Input file could not be opened for reading.\n");
                    fflush(stdout);
                    exit(1);
                }
                inDevNullResult = dup2(devNullInput, 0); // Creates a copy of the file descriptor
                if(inDevNullResult == -1) { // If dup2 fails, print an error
                    printf("dup2(devNullInput, 0) failed.\n");
                    fflush(stdout);
                    exit(1);
                }
                removeRedirection(user_array, num_of_strings, i); // Set the redirection sign and anything afterwards
            }
            else {
                inputFile = open(user_array[i+1], O_RDONLY); // Open a file with the name given to the right of the "<"
                if(inputFile == -1) { // If the file can't be opened for reading, print an error
                    printf("Input file could not be opened for reading.\n");
                    fflush(stdout);
                    exit(1); 
                }
                inResult = dup2(inputFile, 0); // Creates a copy of the file descriptor
                if(inResult == -1) { // If dup2 fails, print an error
                    printf("dup2(inputFile, 0) failed.\n");
                    fflush(stdout);
                    exit(1);
                }
                removeRedirection(user_array, num_of_strings, i); // Set the redirection sign and anything afterwards
            }
        }
        else if(strcmp(user_array[i], ">") == 0) { // If ">" is found
            if(strcmp(user_array[i+1], "&") == 0) { // If there is a "&" after it
                devNullOutput = open("/dev/null", 1); // Open the /dev/null
                if(devNullOutput == -1) { // If the file can't be opened for writing, print an error
                    printf("Input file could not be opened for writing.\n");
                    fflush(stdout);
                    exit(1); 
                }
                outDevNullResult = dup2(devNullOutput, 1); // Creates a copy of the file descriptor
                if(outDevNullResult == -1) { // If dup2 fails, print an error
                    printf("dup2(devNullOutput, 1) failed.\n");
                    fflush(stdout);
                    exit(1);
                }
                removeRedirection(user_array, num_of_strings, i); // Set the redirection sign and anything afterwards
            }
            else {
                outputFile = open(user_array[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644); // Open a file with the name given to the right of the "<"
                if(outputFile == -1) { // If the file can't be opened for writing, print an error
                    printf("Input file could not be opened for writing.\n");
                    fflush(stdout);
                    exit(1);
                }
                outResult = dup2(outputFile, 1); // Creates a copy of the file descriptor
                if(outResult == -1) { // If dup2 fails, print an error
                    printf("dup2(outputFile, 1) failed.\n");
                    fflush(stdout);
                    exit(1);
                }
                removeRedirection(user_array, num_of_strings, i); // Set the redirection sign and anything afterwards
            }
        }
        else {
            continue;
        }
    }
}

///////////////////////////////////////////////////////////////////
// Function: void nonBuiltCommand(char ** user_array, int num_of_strings, int * childExitedMethod)
// Description: Run non built in commands
// Parameters: char ** user_array, int num_of_strings, int * childExitedMethod
// Pre-Conditions: Takes in the user input array, number of strings, and the status number
// Post-Conditions: Runs the non built in commands
///////////////////////////////////////////////////////////////////
void nonBuiltCommand(char ** user_array, int num_of_strings, int * childExitedMethod) {
    pid_t spawnpid = -5; // Declare and initialize the variable 
    int execVal = 0;
    
    // Signal code for CONTROL Z
    sigemptyset(&mask);
    sigaddset(&mask, SIGTSTP);
    sigprocmask(SIG_BLOCK, &mask, NULL); // Mask the signal for control z
    
    spawnpid = fork(); // Forks happens here
    if(getpid() == parentPID) {
        mostRecentProcessPid = spawnpid;
    }
    switch (spawnpid) {
        case -1:
            perror("Error with forking.");
            exit(1);
            break;
        case 0:
            if( ambExist(user_array, num_of_strings)  && isAllowBackground ) { // If "&" exits and background is allowed
                if(hasRedirection(user_array, num_of_strings) == true) { // If there is redirection signs in the user input array
                    pid_array[pid_index] = spawnpid; // Add the pid of the child into the array
                    pid_index++; // Increment the pid array index
                    if(strcmp(user_array[num_of_strings-1], "&") == 0) { // If there is an "&" at the end of the input
                        user_array[num_of_strings-1] = NULL; // Set the "&" to NULL;
                        num_of_strings--; // Decrement the num_of_strings variable
                    }
                    redirectBack(user_array, num_of_strings); // Redirect in the background
                }
                else {
                    if(strcmp(user_array[num_of_strings-1], "&") == 0) { // If there is an "&" at the end of the input
                        user_array[num_of_strings-1] = NULL; // Set the "&" to NULL;
                        num_of_strings--; // Decrement the num_of_strings variable
                    }
                }
            }
            else {
                // Signal code for CONTROL C
                SIGINT_action.sa_handler = SIG_DFL;
                sigaction(SIGINT, &SIGINT_action, NULL);
                if(strcmp(user_array[num_of_strings-1], "&") == 0) { // If there is an "&" at the end of the input
                    user_array[num_of_strings-1] = NULL; // Set the "&" to NULL;
                    num_of_strings--; // Decrement the num_of_strings variable
                }
                if(hasRedirection(user_array, num_of_strings) == true) {
                    redirectFore(user_array, num_of_strings); // Redirect in the foreground
                }
            }
            
            execVal = execvp(user_array[0], user_array); // Execute non built in commands
            if(execVal == -1) {
                printf("%s: %s\n", user_array[0], strerror(errno));
                fflush(stdout);
                exit(2);
            }          
            break;
        default:
            sigprocmask(SIG_UNBLOCK, &mask, NULL); // Unmask the signal for control Z
            if(ambExist(user_array, num_of_strings) && isAllowBackground ) {
                printf("background pid is %d\n", spawnpid);
                fflush(stdout);
                waitpid(spawnpid, childExitedMethod, WNOHANG); // No hang in the background
            }
            else {
                waitpid(spawnpid, childExitedMethod, 0); // hang in the foreground
            }
            break;
    }
}





