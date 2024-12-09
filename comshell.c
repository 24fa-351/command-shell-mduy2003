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
    int arg_count = parse_command(command, args);
    if (arg_count == 0) {
        return;
    }

    int contains_special_symbol = 0;
    for (int i = 0; i < arg_count; i++) {
        if (strcmp(args[i], "|") == 0 || strcmp(args[i], "<") == 0
            || strcmp(args[i], ">") == 0 || strcmp(args[i], "&") == 0) {
            contains_special_symbol = 1;
            break;
        }
    }

    if (contains_special_symbol) {
        handle_external_command(args, arg_count);
    } else if (strcmp(args[0], "cd") == 0) {
        change_directory(args[1]);
    } else if (strcmp(args[0], "pwd") == 0) {
        print_working_directory();
    } else if (strcmp(args[0], "set") == 0) {
        set_environment_variable(args[1], args[2]);
    } else if (strcmp(args[0], "unset") == 0) {
        unset_environment_variable(args[1]);
    } else if (strcmp(args[0], "echo") == 0) {
        handle_echo(args, arg_count);
    } else {
        handle_external_command(args, arg_count);
    }
}

int parse_command(char* command, char* args[])
{
    char* token = strtok(command, " ");
    int arg_count = 0;
    while (token != NULL) {
        args[arg_count++] = token;
        token = strtok(NULL, " ");
    }
    args[arg_count] = NULL;
    return arg_count;
}

void handle_echo(char* args[], int arg_count)
{
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
}

void handle_external_command(char* args[], int arg_count)
{
    int background = 0;
    int redirect_input = 0;
    int redirect_output = 0;
    int pipe_index = -1;
    char* input_file = NULL;
    char* output_file = NULL;

    for (int i = 0; i < arg_count; i++) {
        if (strcmp(args[i], "&") == 0) {
            background = 1;
            args[i] = NULL;
        } else if (strcmp(args[i], "<") == 0) {
            redirect_input = 1;
            input_file = args[i + 1];
            args[i] = NULL;
        } else if (strcmp(args[i], ">") == 0) {
            redirect_output = 1;
            output_file = args[i + 1];
            args[i] = NULL;
        } else if (strcmp(args[i], "|") == 0) {
            pipe_index = i;
            args[i] = NULL;
        }
    }

    if (pipe_index != -1) {
        handle_pipe(args, pipe_index);
    } else {
        execute_external_command(args, background, redirect_input, input_file,
            redirect_output, output_file);
    }
}

void handle_pipe(char* args[], int pipe_index)
{
    args[pipe_index] = NULL;

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        return;
    }

    pid_t pid1 = fork();
    if (pid1 == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        execvp(args[0], args);
        perror("execvp failed");
        exit(EXIT_FAILURE);
    } else if (pid1 > 0) {
        pid_t pid2 = fork();
        if (pid2 == 0) {
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
            execvp(args[pipe_index + 1], &args[pipe_index + 1]);
            perror("execvp failed");
            exit(EXIT_FAILURE);
        } else if (pid2 > 0) {
            close(pipefd[0]);
            close(pipefd[1]);
            waitpid(pid1, NULL, 0);
            waitpid(pid2, NULL, 0);
        } else {
            perror("fork failed");
        }
    } else {
        perror("fork failed");
    }
}

void execute_external_command(char* args[], int background, int redirect_input,
    char* input_file, int redirect_output, char* output_file)
{
    pid_t pid = fork();
    if (pid == 0) {
        if (redirect_input) {
            int fd = open(input_file, O_RDONLY);
            if (fd == -1) {
                perror("open failed");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
        if (redirect_output) {
            int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd == -1) {
                perror("open failed");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        execvp(args[0], args);
        perror("execvp failed");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        if (!background) {
            waitpid(pid, NULL, 0);
        }
    } else {
        perror("fork failed");
    }
}

void change_directory(char* path)
{
    if (chdir(path) != 0) {
        perror("chdir failed");
    }
}

void print_working_directory()
{
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("getcwd failed");
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