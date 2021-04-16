#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define HELP_EXIT 11
#define HELP_FPATH "help.txt"
#define INIT_BUFF_SIZE 512
#define DELIMITERS  " \t\r\n\a"

/*
 * Reads whole line from command line and returns it
 */
char* read_line(){

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
/*
 * Parse input arguments into words for execution
 * @ line - command line string
 * Returns 2D field of arguments
 */
char** parse_shell_line(char* line){

    // Variable declaration
    int buffsize = 64;
    int index = 0;
    char **arguments = malloc(buffsize * sizeof(char*));
    char *argument;

    // Finding first word in line
    argument = strtok(line, DELIMITERS);

    // Main loop for parsing line
    while (argument != NULL){
        // Save argument
        arguments[index] = argument;
        index++;
        // Check if we need to realloc arguments
        if (index > buffsize){
            buffsize *= 2;
            arguments = realloc(arguments, buffsize * sizeof(char*));

        }
        //Load next word form line
        argument = strtok(NULL, DELIMITERS);
    }
    arguments[index] = NULL;
    return arguments;
}

int my_fork(char **args){
    int status;
    pid_t pid, wpid;

    pid =  fork();
    if (pid < 0) {
        // Error while forking
        perror("Error while forking");

    } else if (pid == 0){
        // Execute proces with arguments in args
        if (execvp(args[0], args) == -1) {
            perror("Error while executing");
        }
        exit(EXIT_FAILURE);
    } else {
        // Parent process was created and we have to wait
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return 1;
}

int mu_execute(char **args)
{
    int i;
    if (args[0] == NULL) {
        // An empty command was entered.
        return 1;
    }

    return my_fork(args);
}
void shell_loop() {

    char *line;
    char **arguments;
    int state = 1;

    do {
        line = read_line();
        arguments = parse_shell_line(line);
        state = mu_execute(arguments);

    } while (state);
}

void print_help(){
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
int parse_input_arguments(int argc, char **argv){

    for (int index = 1; index < argc; index++){
        // It's a switch

        if (argv[index][0] == '-'){
            // Print help and exit
            if (argv[index][1] == 'h'){
                print_help();
                return 1;
            }

            // Get port and continue
            else if (argv[index][1] == 'p'){
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


    if (argc <= 1){
        printf("Not enough arguments\n");
        return 4;
    }

    int exit_flag = parse_input_arguments(argc, argv);

    if (exit_flag == 1) return HELP_EXIT;

    if (exit_flag == 0)
        shell_loop();

    return 0;
}