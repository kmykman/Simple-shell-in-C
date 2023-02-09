/*
Name: Man Yuet Ki Kimmy
Reference: COMP 3230A tutorial
https://github.com/aiot-lab/HKU-COMP3230A-Tutorialabs/blob/main/Tutorial2-Process/fork-exec-wait4-example.c
using rusage with wait4() to get user and system CPU time
Reference:
https://github.com/pavani-17/C-Shell/blob/master/piping.c
concept of opening number of pipe same as number of piped command, e.g. open 2 pipes if 2 piped commands
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/resource.h>


static int start = 0;        // 1 when SIGUSR1 is sent to start child process
static int intrp = 0;        // 1 when execvp() stopped due to interrupt
static int prog = 0;         // 1 when valid command is executed
static int status = 0;       // status of wait() in parent process
char * t_pid[1024];          // store pid for timeX in piped command
char * cmd[1024];            // store cmd for timeX in piped command
char * u_t[1024];            // store user CPU time for timeX in piped command
char * sys_t[1024];          // store system CPU time for timeX in piped command
pid_t pid;                   // pid for fork

char * kill_p_id[1024];      // store pid of background process
int k_b_proc = 0;            // store number of pid of background process



// handle interrupt when hit CTRL-C in shell
static void interrupt_handler(int signum) {
    intrp = 1;
    printf("\n");
    fflush(stdout);
}


// start child process when receive SIGUSR1 by changing start to 1
void signal_user_1(int signum) {
    if (signum == SIGUSR1) {
        start = 1;
    }
    return;
}


// handle interrupt when executed program is terminated by SIGINT or SIGTERM by changing prog to 1
void signal_exe(int signum) {
    prog = 1;
    return;
}


// exit shell when user enter "exit", also kill all background process to release all resources
void exit_in(char * command) {
    if (command[5] != '\0') {
        printf("3230shell: \"exit\" with other arguments!!!\n");
        return;
    } else {
        printf("3230shell: Terminated\n");

        for (int i = 0; i < k_b_proc; i++) {
            int bpid = atoi(kill_p_id[i]);
            kill(bpid, SIGKILL);
        }
        exit(0);
    }
}


// for executing one command only
void execute_one_command(char ** args, int timeX, int background) {
    signal(SIGUSR1, signal_user_1);

    pid = fork();
    if (background == 1) {            // set process group when it is in background mode
        setpgid(0, 0);
    }

    kill(pid, SIGUSR1);               // send SIGUSR1 to child process to start execution
    if (pid == 0) {

        while (start == 0) {}         // will not start when no SIGUSR1

        execvp(args[0], args);
        printf("3230shell: \'%s\': %s\n", args[0], strerror(errno));
        exit(0);

    } else {
        if (background == 1) {
            char pid_char[100];

            sprintf(pid_char, "%d", pid);
            kill_p_id[k_b_proc] = malloc(strlen(pid_char) + 1);
            strcpy(kill_p_id[k_b_proc], pid_char);                     // store current pid of background process
            k_b_proc++;
            return;
        } else if (background == 0) {
            if (timeX == 1) {
                struct rusage rusage;
                wait4(pid, & status, 0, & rusage);

                if (strchr(args[0], '/')) {                           // do not show path when path is entered, e.g. show "add" when "./add" is entered

                    char * part, * prev_part;
                    part = malloc(strlen(args[0]) + 1);
                    prev_part = malloc(strlen(args[0]) + 1);
                    part = strtok(args[0], "/");
                    while (part != NULL) {
                        strcpy(prev_part, part);
                        part = strtok(NULL, "/");
                        if (part == NULL) {
                            break;
                        }
                    }
                    printf("(PID)%d  (CMD)%s    (user)%.3f s  (sys)%.3f s\n", pid, prev_part, rusage.ru_utime.tv_sec + rusage.ru_utime.tv_usec / 1000000.0, rusage.ru_stime.tv_sec + rusage.ru_stime.tv_usec / 1000000.0);

                } else {
                    printf("(PID)%d  (CMD)%s    (user)%.3f s  (sys)%.3f s\n", pid, args[0], rusage.ru_utime.tv_sec + rusage.ru_utime.tv_usec / 1000000.0, rusage.ru_stime.tv_sec + rusage.ru_stime.tv_usec / 1000000.0);
                }

            } else {
                waitpid(pid, & status, 0);

            }
        }
        if (intrp == 0 && prog == 1) {                             // only show status when executed program is terminated, if no this line, CTRL-C in shell also print status
            printf("%s\n", strsignal(WTERMSIG(status)));
        }
        start = 0;
    }
    intrp = 0;
    prog = 0;
}


// for executing piped commands
void execute_piped_command(char ** args, int timeX, int background) {
    char * p_args[1024];

    int data = dup(0);
    int pipfd[5][2];
    char command_copy[1024];
    int num_command = 0;
    int current_command = 0;
    memset(command_copy, 0, sizeof command_copy);
    char buf[1024];
    memset(buf, 0, sizeof buf);
    while (args[num_command] != NULL) {                   // count number of command in pipe
        num_command++;
    }

    while (num_command > current_command) {

        int i = 0;
        char * part;

        strcpy(command_copy, args[current_command]);

        part = strtok(command_copy, " ");

        while (part) {                                    // make command into word by word, such as "ls -l" -> "ls", "-l"
            p_args[i] = malloc(strlen(part) + 1);
            strcpy(p_args[i], part);
            if (p_args[i][strlen(p_args[i]) - 1] == '\n') {
                args[i][strlen(p_args[i]) - 1] = '\0';
            }
            part = strtok(NULL, " ");
            i++;
        }
        p_args[i] = NULL;

       
        if (current_command < num_command - 1) {

            pipe(pipfd[current_command]);
            dup2(pipfd[current_command][1], 1);          //store output to current command pipe, e.g. first command in piped is pipfd[0]

            signal(SIGUSR1, signal_user_1);
            if (background == 1) {                       // running pipe command in background process
                setpgid(0, 0);
            }
            pid = fork();
            kill(pid, SIGUSR1);
            if (pid == 0) {

                while (start == 0) {}

          

                if (execvp(p_args[0], p_args) == -1) {                              // when execvp() fail, such as wrong command
                    printf("3230shell: \'%s\': %s\n", p_args[0], strerror(errno));

                    close(pipfd[current_command][1]);
                    dup2(pipfd[current_command][0], 0);

                    dup2(data, 1);

                    signal(SIGUSR1, signal_user_1);
                    if (background == 1) {
                        setpgid(0, 0);
                    }
                    pid = fork();
                    kill(pid, SIGUSR1);
                    if (pid == 0) {

                        while (start == 0) {}

                        execvp(p_args[0], p_args);
                        printf("3230shell: \'%s\': %s\n", p_args[0], strerror(errno));

                        exit(0);

                    } else {
                        if (timeX == 1) {
                            struct rusage rusage;
                            wait4(pid, & status, 0, & rusage);
                            t_pid[current_command] = malloc(sizeof(pid) + 1);
                            cmd[current_command] = malloc(strlen(p_args[0]) + 1);
                            u_t[current_command] = malloc(sizeof(rusage.ru_utime.tv_sec + rusage.ru_utime.tv_usec / 1000000.0) + 2);
                            sys_t[current_command] = malloc(sizeof(rusage.ru_stime.tv_sec + rusage.ru_stime.tv_usec / 1000000.0) + 2);
                            sprintf(t_pid[current_command], "%d", pid);
                            strcpy(cmd[current_command], p_args[0]);
                            sprintf(u_t[current_command], "%f", rusage.ru_utime.tv_sec + rusage.ru_utime.tv_usec / 1000000.0);
                            sprintf(sys_t[current_command], "%f", rusage.ru_stime.tv_sec + rusage.ru_stime.tv_usec / 1000000.0);

                        } else {
                            waitpid(pid, & status, 0);

                        }
                        if (intrp == 0 && prog == 1) {
                            printf("%s\n", strsignal(WTERMSIG(status)));
                        }
                        start = 0;
                    }
                    intrp = 0;
                    prog = 0;

                    dup2(data, 0);                                 // close write end of data

                    close(pipfd[current_command][0]);              // close write end of pipfd

                    dup2(data, pipfd[current_command][1]);         // copy data to pipfd
                    
                    close(pipfd[current_command][1]);
                    read(pipfd[current_command][0], buf, sizeof(buf));           // read output from pipfd
                    dup2(data, 0);
                    printf("%s", buf);                         // print last output

                    while (num_command > current_command) {        // since there is execvp error so it need to run number of exit(0)
                        exit(0);
                        current_command++;
                    }
                    break;
                }

            }
            
            else {
                if (timeX == 1) {                           // store pid, cmd, user time and sys time in each piped command
                    struct rusage rusage;
                    wait4(pid, & status, 0, & rusage);
                    t_pid[current_command] = malloc(sizeof(pid) + 1);
                    cmd[current_command] = malloc(strlen(p_args[0]) + 1);
                    u_t[current_command] = malloc(sizeof(rusage.ru_utime.tv_sec + rusage.ru_utime.tv_usec / 1000000.0) + 2);
                    sys_t[current_command] = malloc(sizeof(rusage.ru_stime.tv_sec + rusage.ru_stime.tv_usec / 1000000.0) + 2);
                    sprintf(t_pid[current_command], "%d", pid);
                    strcpy(cmd[current_command], p_args[0]);
                    sprintf(u_t[current_command], "%f", rusage.ru_utime.tv_sec + rusage.ru_utime.tv_usec / 1000000.0);
                    sprintf(sys_t[current_command], "%f", rusage.ru_stime.tv_sec + rusage.ru_stime.tv_usec / 1000000.0);

                } else {
                    waitpid(pid, & status, 0);

                }
                if (intrp == 0 && prog == 1) {
                    printf("%s\n", strsignal(WTERMSIG(status)));
                }
                start = 0;
                close(pipfd[current_command][1]);
                dup2(pipfd[current_command][0], 0);                 // make the content of current command pipe to be the next command input

            }
            intrp = 0;
            prog = 0;

        }

        
        else if (current_command == num_command - 1) {                   // last command when there is no execvp error in previous commands
            dup2(data, 1);                                   // stop output to screen and store it to data

            signal(SIGUSR1, signal_user_1);
            if (background == 1) {
                setpgid(0, 0);
            }
            pid = fork();
            kill(pid, SIGUSR1);
            if (pid == 0) {

                while (start == 0) {}

                execvp(p_args[0], p_args);
                printf("3230shell: \'%s\': %s\n", p_args[0], strerror(errno));

                exit(0);

            } else {
                if (timeX == 1) {
                    struct rusage rusage;
                    wait4(pid, & status, 0, & rusage);
                    t_pid[current_command] = malloc(sizeof(pid) + 1);
                    cmd[current_command] = malloc(strlen(p_args[0]) + 1);
                    u_t[current_command] = malloc(sizeof(rusage.ru_utime.tv_sec + rusage.ru_utime.tv_usec / 1000000.0) + 2);
                    sys_t[current_command] = malloc(sizeof(rusage.ru_stime.tv_sec + rusage.ru_stime.tv_usec / 1000000.0) + 2);
                    sprintf(t_pid[current_command], "%d", pid);
                    strcpy(cmd[current_command], p_args[0]);
                    sprintf(u_t[current_command], "%f", rusage.ru_utime.tv_sec + rusage.ru_utime.tv_usec / 1000000.0);
                    sprintf(sys_t[current_command], "%f", rusage.ru_stime.tv_sec + rusage.ru_stime.tv_usec / 1000000.0);

                } else {
                    waitpid(pid, & status, 0);

                }
                if (intrp == 0 && prog == 1) {
                    printf("%s\n", strsignal(WTERMSIG(status)));
                }
                start = 0;
            }
            intrp = 0;
            prog = 0;

            dup2(data, 0);

            close(pipfd[current_command][0]);

            dup2(data, pipfd[current_command][1]);                      // copy data to pipfd
            close(pipfd[current_command][1]);
            read(pipfd[current_command][0], buf, sizeof(buf));          // read pipfd content
            dup2(data, 0);
            printf("%s", buf);

        }

        
        current_command++;
    }
    dup2(data, 0);

    if (timeX == 1) {                            // print pid, cmd, user CPU time and sys CPU time of all command
        int tpid = 0;
        float user_t = 0, system_t = 0;
        for (int i = 0; i < num_command; i++) {
            tpid = atof(t_pid[i]);
            user_t = atof(u_t[i]);
            system_t = atof(sys_t[i]);
            if (strchr(cmd[i], '/')) {

                char * part, * prev_part;
                part = malloc(strlen(cmd[i]) + 1);
                prev_part = malloc(strlen(cmd[i]) + 1);
                part = strtok(cmd[i], "/");
                while (part != NULL) {
                    strcpy(prev_part, part);
                    part = strtok(NULL, "/");
                    if (part == NULL) {
                        break;
                    }
                }
                printf("(PID)%d  (CMD)%s    (user)%.3f s  (sys)%.3f s\n", tpid, prev_part, user_t, system_t);

            } else {
                printf("(PID)%d  (CMD)%s    (user)%.3f s  (sys)%.3f s\n", tpid, cmd[i], user_t, system_t);
            }
        }
    }
}


// main function to print shell script, take input and handle some error input
int main(void) {
    char command[1024];
    char command_copy[1024];
    char * args[1024];

    while (1) {
        sigaction(SIGINT, & (struct sigaction) {
            .sa_handler = interrupt_handler
        }, NULL);                                  // will not quit shell when CTRL-C is hit

        printf("$$ 3230shell ##  ");
        memset(command, 0, sizeof command);                 // clear all array contents left
        memset(command_copy, 0, sizeof command_copy);
        memset(args, 0, sizeof args);
        memset(t_pid, 0, sizeof t_pid);
        memset(cmd, 0, sizeof cmd);
        memset(u_t, 0, sizeof u_t);
        memset(sys_t, 0, sizeof sys_t);
        fgets(command, 1024, stdin);

        if (strcmp(command, "\n") == 0) {
            continue;
        }

        strncpy(command_copy, command, 4);
        command_copy[4] = '\0';
        if (strcmp(command_copy, "exit") == 0) {
            exit_in(command);
            continue;
        }

        strcpy(command_copy, command);

        int i = 0;
        char * token;

        if (!strchr(command_copy, '|')) {                  // if piped, args stores word by word; if not piped, args stores command by command
            token = strtok(command_copy, " ");
        } else if (strchr(command_copy, '|')) {
            token = strtok(command_copy, "|");
        }

        while (token) {
            args[i] = malloc(strlen(token) + 1);
            strcpy(args[i], token);

            if (args[i][strlen(args[i]) - 1] == '\n') {
                args[i][strlen(args[i]) - 1] = '\0';
            }
            if (!strchr(command, '|')) {
                token = strtok(NULL, " ");
            } else if (strchr(command, '|')) {
                token = strtok(NULL, "|");
            }
            i++;
        }
        if ((i >= 1) && ((strcmp(args[i - 1], "") == 0) || (strcmp(args[i - 1], "\n") == 0))) {
            i--;
        }

        args[i] = NULL;

        int count = -10;
        if (strchr(command, '|')) {
            count = 0;
        }
        for (int z = 0; z <= strlen(command); z++) {
            if (command[z] == 124) {
                count++;
            }
        }

        if (count >= i) {
            printf("3230shell: should not have two consecutive | without in-between command\n");
            continue;
        }

        int background = 0;
        if (strchr(command, '&')) {                          // check if & is entered (background process)
            if ((strcmp(args[i - 1], "&") == 0)) {
                args[i - 1] = NULL;
                i--;
                background = 1;
            } else if (args[i - 1][strlen(args[i - 1]) - 1] == '&') {
                args[i - 1][strlen(args[i - 1]) - 1] = '\0';
                background = 1;
            } else {
                printf("3230shell: \'&\' should not appear in the middle of the command line\n");
                continue;
            }
        }

        int timeX = 0;
        strncpy(command_copy, command, 5);
        command_copy[5] = '\0';
        if (strcmp(command_copy, "timeX") == 0) {                          // check if timeX is entered
            timeX = 1;
            if ((background == 1) && (timeX == 1)) {                          // error when timeX used with &
            printf("3230shell: \"timeX\" cannot be run in background mode\n");
            continue;
            }
            if (command[5] != 32 && command[5] != 10) {} else if ((i == 1) && (command[5] == 32 || command[5] == 10)) {
                printf("\"timeX\" cannot be a standalone command\n");
                continue;
            } else {
                timeX = 1;
                if (!strchr(command, '|')) {
                    for (int z = 0; z <= i; z++) {
                        args[z] = args[z + 1];
                    }
                } else if (strchr(command, '|')) {
                    for (int z = 0; z <= strlen(args[0]); z++) {
                        args[0][z] = args[0][z + 5];
                    }
                }

            }

        }
        
        for (int sig_num = 1; sig_num < 32; sig_num++) {
            if ((sig_num != 11) && (sig_num != 17)) {                // 11 is SIGSEGV, 17 is SIGCHLD
                signal(sig_num, signal_exe);
            }
        }

        if (!strchr(command, '|')) {
            execute_one_command(args, timeX, background);
        } else if (strchr(command, '|')) {
            execute_piped_command(args, timeX, background);
        }

        for (int sig_num = 1; sig_num < 32; sig_num++) {                // reset signal
            signal(sig_num, SIG_DFL);
        }

    }
    return (0);
}
