#ifndef COMSHELL_H
#define COMSHELL_H

void start_shell();
void execute_command(char* command);
int parse_command(char* command, char* args[]);
void handle_echo(char* args[], int arg_count);
void handle_external_command(char* args[], int arg_count);
void handle_pipe(char* args[], int pipe_index);
void execute_external_command(char* args[], int background, int redirect_input,
    char* input_file, int redirect_output, char* output_file);
void change_directory(char* path);
void print_working_directory();
void set_environment_variable(char* var, char* value);
void unset_environment_variable(char* var);
char* get_environment_variable(char* var);

#endif // COMSHELL_H