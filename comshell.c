#include "comshell.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARG_SIZE 100

void start_shell()
{
    char input[MAX_INPUT_SIZE];
    while (1) {
        printf("xsh# ");
        if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL) {
            break;
        }
        input[strcspn(input, "\n")] = 0; // Remove newline character
        if (strcmp(input, "quit") == 0 || strcmp(input, "exit") == 0) {
            break;
        }
        execute_command(input);
    }
}

void execute_command(char* command)
{
    char* args[MAX_ARG_SIZE];
    char* token = strtok(command, " ");
    int arg_count = 0;
    while (token != NULL) {
        args[arg_count++] = token;
        token = strtok(NULL, " ");
    }
    args[arg_count] = NULL;

    if (arg_count == 0) {
        return;
    }

    if (strcmp(args[0], "cd") == 0) {
        change_directory(args[1]);
    } else if (strcmp(args[0], "pwd") == 0) {
        print_working_directory();
    } else if (strcmp(args[0], "set") == 0) {
        set_environment_variable(args[1], args[2]);
    } else if (strcmp(args[0], "unset") == 0) {
        unset_environment_variable(args[1]);
    } else if (strcmp(args[0], "echo") == 0) {
        for (int i = 1; i < arg_count; i++) {
            if (args[i][0] == '$') {
                char* var = get_environment_variable(args[i] + 1);
                if (var) {
                    printf("%s ", var);
                } else {
                    printf(" ");
                }
            } else {
                printf("%s ", args[i]);
            }
        }
        printf("\n");
    } else {
        pid_t pid = fork();
        if (pid == 0) {
            execvp(args[0], args);
            perror("Execvp failed");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            wait(NULL);
        } else {
            perror("Fork failed");
        }
    }
}

void change_directory(char* path)
{
    if (chdir(path) != 0) {
        perror("Couldn't change directory");
    }
}

void print_working_directory()
{
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("Couldn't get current working directory");
    }
}

void set_environment_variable(char* var, char* value)
{
    if (setenv(var, value, 1) != 0) {
        fprintf(stderr, "Failed to set environment variable %s\n", var);
    }
}

void unset_environment_variable(char* var)
{
    if (unsetenv(var) != 0) {
        fprintf(stderr, "Failed to unset environment variable %s\n", var);
    }
}

char* get_environment_variable(char* var) { return getenv(var); }