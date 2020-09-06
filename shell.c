#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

enum {
    BUF_SIZE = 256,
    ARG_SIZE = 16,
    CMD_SIZE = 8,
};

int left[2], right[2];

typedef struct Command Command;

struct Command {
    char cmd[CMD_SIZE * ARG_SIZE];
    int bg, output, indices[ARG_SIZE];
    char *infile, *outfile;
};

int splitinput(Command *, char *);
int splitcmd(Command *, char **);
int cleanupargs(Command *, char **, int);
int run(char **, Command, int, int);

int main(){
    char line[BUF_SIZE], *args[ARG_SIZE];
    int i, j, k, numcommands, argc, pipecount;
    Command command, commands[CMD_SIZE];
    
    while(1){
        printf(">>> ");
        
        if (fgets(line, BUF_SIZE, stdin) == NULL){
            printf("\n");
            break;
        }
        
        if (line[0] == '\n'){
            continue;
        }
        
        if (line[strlen(line) - 1] == '\n'){
            line[strlen(line) - 1] = '\0';
        }
        
        numcommands = splitinput(commands, line);
        
        for (i = 0; i < numcommands; i++){
            argc = splitcmd(&commands[i], args);
            pipecount = cleanupargs(&commands[i], args, argc);
            for (j = 0; j <= pipecount; j++){
                if (strcmp(args[commands[i].indices[j]], "exit") == 0){
                    return(0);
                }
                if (strcmp(args[0], "cd") == 0){
                    if (args[2]){
                        fprintf(stderr, "Too Many Arguments.\n");
                        continue;
                    }
                    if (!args[1] || strcmp(args[1], "~") == 0){
                        chdir(getenv("HOME"));
                    } else {
                        chdir(args[1]);
                    }
                } else {
                    k = run(args, commands[i], pipecount, j);
                    if (k == 3){
                        continue;
                    }
                    if (k != 0){
                        return(k);
                    }
                }
            }
            if (pipecount){
                close(left[0]);
                close(left[1]);
            }
        }

        for (i = 0; i < argc; i++){
            args[i] = NULL;
        }
    }
    return(0);
}

/*
 * Adds a command to the commands struct array
 */
void addcommand(Command commands[], char *line, int length, int bg, int *index){
    Command command;

    command.bg = bg;
    strncpy(command.cmd, line, length);
    command.cmd[length] = '\0';
    commands[*index] = command;
    (*index)++;
}

/* 
 * Splits input by semicolons for consecutive execution 
 * Splits input by ampersands for simultaneous execution
 */
int splitinput(Command commands[], char *line){
    int i, j, numcommands;

    j = numcommands = 0;
    for (i = 0; i < strlen(line); i++){
        if (strchr(line + j, ';') || strchr(line + j, '&')){
            if (line[i] == ';'){
                addcommand(commands, line + j, i - j, 0, &numcommands);
                j = i + 1;
            } else if (line[i] == '&'){
                addcommand(commands, line + j, i - j, 1, &numcommands);
                j = i + 1;
            }
        } else {
            addcommand(commands, line + j, strlen(line) - j, 0, &numcommands);
            break;
        }
    }
    return numcommands;
}

/*
 * Splits each command into arguments by spaces, tabs, and newlines
 */ 
int splitcmd(Command *command, char **args){
    int pipecount, i, argc;

    args[0] = strtok(command->cmd, " \t\n");
    for (argc = 1; argc < ARG_SIZE; argc++){
        args[argc] = strtok(NULL, " \t\n");
        if (args[argc] == NULL){
            break;
        }
    }
    return argc;
}

/*
 * Checks if piping and redirection is present
 */
int cleanupargs(Command *command, char **args, int argc){
    int pipecount, i;
    
    pipecount = command->indices[0] = 0;
    for (i = 0; i < argc; i++){
        if (args[i] == NULL){
            break;
        }
        if (strcmp(args[i], "<") == 0){
            command->infile = args[i + 1];
            args[i] = NULL;
        } else if (strcmp(args[i], ">>") == 0){
            command->outfile = args[i + 1];
            command->output = 2;
            args[i] = NULL;
        } else if (strcmp(args[i], ">") == 0){
            command->outfile = args[i + 1];
            command->output = 1;
            args[i] = NULL;
        } else if (strcmp(args[i], "|") == 0){
            args[i] = NULL;
            command->indices[pipecount + 1] = i + 1;
            pipecount++;
        } else {
            command->indices[i] = i;
        }
    }
    return pipecount;
}

/* 
 * Replaces stdin by opening a file
 */
void redirin(char *file){
    int fd;

    fd = open(file, O_RDONLY);
    dup2(fd, 0);
    close(fd);
}

/* 
 * Replaces stdout by opening a file
 */
void redirout(char *file, int flag){
    int fd;

    fd = open(file, flag, 0664);
    dup2(fd, 1);
    close(fd);
}

/*
 * Runs the command, piping and redirecting as necessary
 */
int run(char** args, Command command, int pipecount, int index){
    int chpid, bgpid, status, flag;

    if (pipecount && index != pipecount){
        pipe(right);
    }
    chpid = fork();
    if (chpid < 0){
        perror("fork");
        return(1);
    }
    if (chpid == 0){
        if (index == 0 && command.infile){
            redirin(command.infile);
        }
        if (index == pipecount && command.outfile){
            if (command.output == 1){
                flag = O_WRONLY | O_CREAT | O_TRUNC;
            } else if (command.output == 2){
                flag = O_WRONLY | O_CREAT | O_APPEND;
            }
            redirout(command.outfile, flag);
        }
        if (index != pipecount){
            dup2(right[1], 1);
            close(right[0]);
            close(right[1]);
        }
        if (index != 0){
            dup2(left[0], 0);
            close(left[0]);
            close(left[1]);
        }
        execvp(args[command.indices[index]], &args[command.indices[index]]);
        perror("execvp");
        return(2);
    } else {
        if (index != 0){
            close(left[0]);
            close(left[1]);
        }
        left[0] = right[0];
        left[1] = right[1];
        if (!command.bg){
            wait(&status);
        } else {
            printf("Running %s in the background.\n", args[command.indices[index]]);
            
            bgpid = waitpid(chpid, &status, WNOHANG);
            printf("PID = %d\n", bgpid);
            if (bgpid == chpid){
                printf("Ran %s successfully.\n", args[command.indices[index]]);
                return(3);
            }
        }
    }
    return(0);
}
