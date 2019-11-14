#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

int MAX_LENGTH_OF_STR = 2048;
int MAX_NUM_OF_ARGS = 512;

bool checkNumArgs(char * input);
bool signExists(char * input);
void expandfunction(char * input);

int main() {
    char * input = (char *)malloc(sizeof(char) * MAX_LENGTH_OF_STR);
    
    do {
        printf(": ");
        fgets(input, MAX_LENGTH_OF_STR, stdin); // Might need to change to getline
        strtok(input, "\n");
        if(checkNumArgs(input) != true) {
            continue;
        }
        if(strcmp("exit", input) == 0) {
    
        } 
    
    } while(strcmp(input, "exit") != 0);
    
    return 0;
    char input[256] = "echo $$ dsfds $$ $$$$";
    expandfunction(input);
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

void changeDir() {
    
    // find the current directory: cwd
    // if it cd doesn't have any arguments, then change the director to home directory
    // getenv() and pass in the string "HOME", returns a char *
    // chdir() to change to another directory
    // if the user includes an argument with the name of a folder, check if the directory exists and if does 
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

void expandfunction(char * input) {
    // cd aeijan $$ - the dollar signs have to become a pid of parent
    // change string?? erase the dollar signs and change the string
    // example: echo $$ expands to echo 5234
    // return 1 if  the dollar signs exist
    // Makes this program easier to grade?
    
    // input: "echo $$ dsfds $$ $$$$";
    pid_t pid = getpid();
    int temp = (int)pid;
    char newString[256];
    while(signExists(input) == true) {
        char * dds = strstr(input, "$$");
        dds[0] = '%';
        dds[1] = 'd';
        sprintf(newString, input, (int)pid);
        strcpy(input, newString);
    }
    printf("%s\n",newString);
}

void exitShell() {
    // kill child and foreground processes
    // holder for all pid of child processes
    // run sigkill for each child process
}

// redirection() {}
// using > or < to redirection
// dup2()
// cat < red needs to become cat
// cat < red > output also n eeds to become cat
// background process redirect to /dev/null
// open the file to /dev/null, dup2(), stdin = 0;
