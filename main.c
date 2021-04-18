#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>



#define HELP_EXIT 11
#define HELP_FPATH "help.txt"
#define INIT_BUFF_SIZE 512
#define DELIMITERS  " \t\r\n\a"

char** parse_by_delimiter(char* line, const char* delimiters, int init_buff_size);

int found_args = 0;
/*
 * Reads whole line from command line and returns it
 */
char* read_line() {

    // Variable declaration
    int buffer_size = INIT_BUFF_SIZE;
    char *buffer = malloc(sizeof(char) * buffer_size);
    int index = 0;

    while(1) {

        // If run out of buffer realloc
        if (index >= buffer_size) {
            buffer_size *= 2;
            buffer = realloc(buffer, buffer_size);
        }
        // Read input char
        int input = getchar();

        // Check for end of input
        if (input == EOF || input == '\n') {
            buffer[index] = '\0';
            return buffer;
        } else {
            buffer[index] = (char) input;
        }

        index++;
    }
}
void redirect_check(char **command, int *in_out) {
    int index = 0;
    int out, in;

    while (command[index] != NULL) {
        if (index != 0 && strcmp(command[index], ">") == 0) {
            command[index] = NULL;
            index++;
            if ( command[index] == NULL)
                return;
            out = open(command[index], O_WRONLY | O_TRUNC | O_CREAT,  S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
            in_out[1] = out;
        }
        if (index != 0 && strcmp(command[index], "<") == 0) {
            command[index] = NULL;
            index++;
            if ( command[index] == NULL)
                return;
            in = open(command[index],  O_RDONLY);
            in_out[0] = in;
        }
        index++;
    }
}
int no_pipe_command(char **args) {
    int status;
    pid_t pid, wpid;
    int in_out[2] = {-1,-1};
    int outflag = 0, inflag = 0;
    char **command = parse_by_delimiter(args[0], DELIMITERS, 64);
    // check for redirections
    redirect_check(command,in_out);

    pid =  fork();
    if (pid < 0) {
        // Error while forking
        perror("Error while forking");

    } else if (pid == 0){
        if (in_out[0] >= 0)
            if (dup2(in_out[0], STDIN_FILENO) < 0) {
                perror("Error while dup");
                exit(1);
            }
        if (in_out[1] >= 0)
            if (dup2(in_out[1], STDOUT_FILENO) < 0) {
                perror("Error while dup");
                exit(1);
            }

        // Execute proces with arguments in args
        if (execvp(command[0], command) == -1) {
            perror("Error while executing");
        }
        close(in_out[0]);
        close(in_out[1]);
        exit(EXIT_FAILURE);
    } else {
        // Parent process was created and we have to wait
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    free(command);
    return 1;
}

/*
 * Parse input arguments into words for execution
 * @ line - command line string
 * @ delimiters - separation characters
 * @ init_buff_size - initial size of buffer
 * Returns 2D field of arguments
 */
char** parse_by_delimiter(char* line, const char* delimiters, int init_buff_size) {
    // Variable declaration
    int buffsize = init_buff_size + 1;
    int index = 0;
    char **arguments = malloc(buffsize * sizeof(char*));
    char *argument;

    // Finding first word in line
    argument = strtok(line, delimiters);

    // Main loop for parsing line
    while (argument != NULL) {
        // Save argument
        if (argument[0] == '#') {
            arguments[index] = NULL;
            return arguments;
        }
        arguments[index] = argument;
        index++;
        // Check if we need to realloc arguments
        if (index > buffsize){
            buffsize *= 2;
            arguments = realloc(arguments, buffsize * sizeof(char*));
        }
        //Load next word form line
        argument = strtok(NULL, delimiters);
    }
    arguments[index] = NULL;
    return arguments;
}

int pipe_execute(int pipe_count, char **args) {

    pid_t pid, wpid;
    int *pipe_file_des = malloc(pipe_count * 2 * sizeof (int));
    int status;
    for (int i = 0; i < pipe_count - 1; i++) {
        pipe(pipe_file_des + i * 2);
    }
    int index = 0;

    while (index  != pipe_count) {
        pid = fork();
        if (pid == 0) {
            // If not first then redirect input
            char **command = parse_by_delimiter(args[index], DELIMITERS, 64);
            int in_out[2] = {-1,-1};

            // check for redirect
            redirect_check(command, in_out);
            // Do redirections
            if (index == 0 && in_out[0] >= 0) {
                if (dup2(in_out[0], STDIN_FILENO) < 0) {
                    perror("Error while dup");
                    exit(1);
                }

            }
            if (index == pipe_count - 1 && in_out[1] >= 0) {
                if (dup2(in_out[1], STDOUT_FILENO) < 0) {
                    perror("Error while dup");
                    exit(1);
                }

            }

            if (index != 0) {
                if (dup2(pipe_file_des[(index-1) * 2], STDIN_FILENO) < 0) {
                    perror("Error while dup");
                    exit(1);
                }
            }
            // If not last redirect output
            if (index != pipe_count - 1)
                if( dup2(pipe_file_des[index * 2 + 1], STDOUT_FILENO) < 0) {
                    perror("Error while dup");
                    exit(1);
                }

            for (int j = 0; j < pipe_count - 1; j++) {
                close(pipe_file_des[j * 2]);
                close(pipe_file_des[j * 2 + 1]);
            }

            if (execvp(command[0], command) == -1) {
                perror("Error while executing");
            }
            if (in_out[0] >= 0)
                close(in_out[0]);
            if (in_out[1] >= 0)
                close(in_out[1]);
            free(command);
        }
        if (pid < 0) {
            // Error while forking
            perror("Error while forking");
            return -1;
        }
        index++;
    }
    for (int j = 0; j < pipe_count - 1; j++) {
        close(pipe_file_des[j * 2]);
        close(pipe_file_des[j * 2 + 1]);
    }

    for (int i = 0; i < pipe_count; ++i) {
        wait(&status);
    }
    free(pipe_file_des);
    return 1;
}

int execute_pipes(char** pipes) {

    int index = 0;
    int pipe_count = 0;
    while (pipes[index] != NULL) {
        pipe_count++;
        index++;
    }
    if (pipe_count > 1)
        return pipe_execute(pipe_count,pipes);
    else if ( pipe_count == 1)
        return no_pipe_command(pipes);
    return -1;
}

void shell_loop() {

    char *line;
    char **commands;
    int state = 1;

    // Main shell loop
    do {
        line = read_line();
        // split separate commands ';'
        commands = parse_by_delimiter(line, ";", 6);
        int index = 0;
        // For every separate command do:
        while ( commands[index] != NULL) {
            char **pipes;
            // Split into pipes
            pipes = parse_by_delimiter(commands[index], "|", 10);

            // Execute pipes
            state = execute_pipes(pipes);
            index++;
            free(pipes);
        }
        free(commands);
        free(line);
    } while (state > 0);
}

void print_help() {
    FILE *file_ptr;

    file_ptr = fopen(HELP_FPATH, "r");

    if (file_ptr == NULL)
        printf("Error opening file\n");
    else {
        printf("help check\n");
        char letter = fgetc(file_ptr);
        while (letter != EOF) {

            printf("%c", letter);
            letter = fgetc(file_ptr);
        }
    }
    printf("\n\n");
    fclose(file_ptr);
}

int parse_input_arguments(int argc, char **argv) {

    for (int index = 0; index < argc; index++) {
        // It's a switch

        if (argv[index][0] == '-'){
            // Print help and exit
            if (argv[index][1] == 'h'){
                print_help();
                return 1;
            }

            // Get port and continue
            else if (argv[index][1] == 'p') {
                printf("%s",argv[++index]);
            }
            // Get path and continue
            else if (argv[index][1] == 'u') {
                printf("%s", argv[++index]);
            }
        }

    }
    return 0;
}

int main(int argc, char **argv) {


    if (argc <= 0){
        printf("Not enough arguments\n");
        return 4;
    }
    int exit_flag = parse_input_arguments(argc, argv);

    if (exit_flag == 1) return HELP_EXIT;

    if (exit_flag == 0)
        shell_loop();

    return 0;
}