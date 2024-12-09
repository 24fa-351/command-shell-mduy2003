#ifndef COMSHELL_H
#define COMSHELL_H

void start_shell();
void execute_command(char* command);
void change_directory(char* path);
void print_working_directory();
void set_environment_variable(char* var, char* value);
void unset_environment_variable(char* var);
char* get_environment_variable(char* var);

#endif // COMSHELL_H