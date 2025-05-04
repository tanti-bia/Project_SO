// #define _GNU_SOURCE
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <signal.h>
// #include <sys/types.h>
// #include <dirent.h>
// #include <sys/stat.h>
// #include <fcntl.h>
// #include <errno.h>
// #include <time.h>
// #include <sys/wait.h>
// #include <ctype.h>

// #define MAX_BUF 256
// #define CMD_FILE "/tmp/treasure_cmd.txt"

// pid_t monitor_pid = -1;
// int shutting_down = 0;

// void list_hunts_info() {
//     DIR *d = opendir("hunts");
//     if (!d) {
//         perror("opendir");
//         return;
//     }

//     struct dirent *entry;
//     while ((entry = readdir(d)) != NULL) {
//         if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
//             char path[512];
//             snprintf(path, sizeof(path), "hunts/%s/treasures.dat", entry->d_name);

//             int count = 0;
//             int fd = open(path, O_RDONLY);
//             if (fd >= 0) {
//                 struct stat st;
//                 if (fstat(fd, &st) == 0) {
//                     count = st.st_size / sizeof(struct treasure);
//                 }
//                 close(fd);
//             }
//             printf("Hunt: %s - Treasures: %d\n", entry->d_name, count);
//         }
//     }
//     closedir(d);
// }

// void list_treasures_info(char *hunt_id) {
//     char path[512];
//     snprintf(path, sizeof(path), "hunts/%s/treasures.dat", hunt_id);
//     int fd = open(path, O_RDONLY);
//     if (fd < 0) {
//         perror("open");
//         return;
//     }

//     struct treasure {
//         int id;
//         char username[32];
//         float latitude;
//         float longitude;
//         char clue[128];
//         int value;
//     } t;

//     while (read(fd, &t, sizeof(t)) == sizeof(t)) {
//         printf("ID: %d, User: %s, Value: %d\n", t.id, t.username, t.value);
//     }

//     close(fd);
// }

// void view_treasure_info(char *hunt_id, int tid) {
//     char path[512];
//     snprintf(path, sizeof(path), "hunts/%s/treasures.dat", hunt_id);
//     int fd = open(path, O_RDONLY);
//     if (fd < 0) {
//         perror("open");
//         return;
//     }

//     struct treasure {
//         int id;
//         char username[32];
//         float latitude;
//         float longitude;
//         char clue[128];
//         int value;
//     } t;

//     while (read(fd, &t, sizeof(t)) == sizeof(t)) {
//         if (t.id == tid) {
//             printf("ID: %d, User: %s, Coords: (%.2f, %.2f), Clue: %s, Value: %d\n",
//                    t.id, t.username, t.latitude, t.longitude, t.clue, t.value);
//             close(fd);
//             return;
//         }
//     }

//     printf("Treasure ID %d not found.\n", tid);
//     close(fd);
// }

// void monitor_signal_handler(int sig) {
//     if (sig == SIGUSR1) {
//         list_hunts_info();
//     } else if (sig == SIGUSR2) {
//         FILE *f = fopen(CMD_FILE, "r");
//         if (!f) {
//             perror("fopen");
//             return;
//         }
//         char cmd[MAX_BUF], hunt[MAX_BUF];
//         int tid;
//         fscanf(f, "%s", cmd);
//         if (strcmp(cmd, "list_treasures") == 0) {
//             fscanf(f, "%s", hunt);
//             list_treasures_info(hunt);
//         } else if (strcmp(cmd, "view_treasure") == 0) {
//             fscanf(f, "%s %d", hunt, &tid);
//             view_treasure_info(hunt, tid);
//         }
//         fclose(f);
//     } else if (sig == SIGTERM) {
//         printf("Monitor shutting down...\n");
//         usleep(1000000); // simulate delay
//         exit(0);
//     }
// }

// void start_monitor() {
//     if (monitor_pid > 0) {
//         printf("Monitor already running.\n");
//         return;
//     }

//     monitor_pid = fork();
//     if (monitor_pid == 0) {
//         // child - monitor
//         struct sigaction sa;
//         sa.sa_handler = monitor_signal_handler;
//         sigemptyset(&sa.sa_mask);
//         sa.sa_flags = SA_RESTART;

//         sigaction(SIGUSR1, &sa, NULL);
//         sigaction(SIGUSR2, &sa, NULL);
//         sigaction(SIGTERM, &sa, NULL);

//         printf("Monitor started with PID %d\n", getpid());

//         while (1) pause();
//     } else if (monitor_pid < 0) {
//         perror("fork");
//     }
// }

// void stop_monitor() {
//     if (monitor_pid <= 0) {
//         printf("No monitor running.\n");
//         return;
//     }

//     kill(monitor_pid, SIGTERM);
//     shutting_down = 1;
//     printf("Monitor shutdown signal sent.\n");
// }

// void wait_for_monitor() {
//     int status;
//     waitpid(monitor_pid, &status, 0);
//     printf("Monitor stopped with status %d.\n", WEXITSTATUS(status));
//     monitor_pid = -1;
//     shutting_down = 0;
// }

// int main() {
//     char input[MAX_BUF];

//     while (1) {
//         if (!shutting_down)
//             printf("hub> ");
//         fflush(stdout);

//         if (!fgets(input, sizeof(input), stdin))
//             break;

//         input[strcspn(input, "\n")] = 0;

//         if (shutting_down) {
//             printf("Monitor is shutting down. Please wait...\n");
//             if (waitpid(monitor_pid, NULL, WNOHANG) == monitor_pid) {
//                 wait_for_monitor();
//             }
//             continue;
//         }

//         if (strcmp(input, "start_monitor") == 0) {
//             start_monitor();
//         } else if (strcmp(input, "list_hunts") == 0) {
//             if (monitor_pid > 0)
//                 kill(monitor_pid, SIGUSR1);
//             else
//                 printf("Monitor not running.\n");
//         } else if (strncmp(input, "list_treasures", 14) == 0) {
//             char hunt[MAX_BUF];
//             if (sscanf(input, "list_treasures %s", hunt) == 1) {
//                 FILE *f = fopen(CMD_FILE, "w");
//                 if (f) {
//                     fprintf(f, "list_treasures %s\n", hunt);
//                     fclose(f);
//                     kill(monitor_pid, SIGUSR2);
//                 } else {
//                     perror("fopen");
//                 }
//             } else {
//                 printf("Usage: list_treasures <hunt_id>\n");
//             }
//         } else if (strncmp(input, "view_treasure", 13) == 0) {
//             char hunt[MAX_BUF];
//             int tid;
//             if (sscanf(input, "view_treasure %s %d", hunt, &tid) == 2) {
//                 FILE *f = fopen(CMD_FILE, "w");
//                 if (f) {
//                     fprintf(f, "view_treasure %s %d\n", hunt, tid);
//                     fclose(f);
//                     kill(monitor_pid, SIGUSR2);
//                 } else {
//                     perror("fopen");
//                 }
//             } else {
//                 printf("Usage: view_treasure <hunt_id> <id>\n");
//             }
//         } else if (strcmp(input, "stop_monitor") == 0) {
//             stop_monitor();
//         } else if (strcmp(input, "exit") == 0) {
//             if (monitor_pid > 0) {
//                 printf("Monitor still running. Stop it first.\n");
//             } else {
//                 printf("Exiting.\n");
//                 break;
//             }
//         } else {
//             printf("Unknown command.\n");
//         }

//         if (shutting_down && waitpid(monitor_pid, NULL, WNOHANG) == monitor_pid) {
//             wait_for_monitor();
//         }
//     }

//     return 0;
// }

// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <signal.h>
// #include <sys/types.h>
// #include <sys/wait.h>
// #include <fcntl.h>
// #include <errno.h>

// #define COMMAND_FILE "command.txt"
// #define RESPONSE_FILE "response.txt"

// pid_t monitor_pid = -1;

// void handle_sigchld(int sig) {
//     int saved_errno = errno;
//     wait(NULL);
//     monitor_pid = -1;
//     errno = saved_errno;
// }

// void start_monitor() {
//     if (monitor_pid != -1) {
//         printf("Monitor is already running.\n");
//         return;
//     }

//     monitor_pid = fork();
//     if (monitor_pid == 0) {
//         // Child process: Monitor
//         while (1) {
//             pause(); // Wait for signals
//             FILE *fp = fopen(COMMAND_FILE, "r");
//             if (fp) {
//                 char command[256];
//                 if (fgets(command, sizeof(command), fp)) {
//                     // Process command
//                     if (strcmp(command, "list_hunts\n") == 0) {
//                         // Simulate listing hunts
//                         printf("Listing hunts...\n");
//                         usleep(2000000); // Simulate delay
//                         printf("Hunt 1: 3 treasures\nHunt 2: 5 treasures\n");
//                     } else if (strncmp(command, "list_treasures ", 15) == 0) {
//                         // Simulate listing treasures
//                         printf("Listing treasures for %s...\n", command + 15);
//                         usleep(2000000); // Simulate delay
//                         printf("Treasure 1: Gold\nTreasure 2: Silver\n");
//                     } else if (strncmp(command, "view_treasure ", 14) == 0) {
//                         // Simulate viewing a treasure
//                         printf("Viewing treasure %s...\n", command + 14);
//                         usleep(2000000); // Simulate delay
//                         printf("Treasure ID: 1, Value: 100\n");
//                     } else if (strcmp(command, "stop_monitor\n") == 0) {
//                         printf("Stopping monitor...\n");
//                         break;
//                     }
//                 }
//                 fclose(fp);
//             }
//         }
//         exit(0);
//     } else if (monitor_pid < 0) {
//         perror("Failed to fork");
//     }
// }

// void stop_monitor() {
//     if (monitor_pid == -1) {
//         printf("Monitor is not running.\n");
//         return;
//     }
//     FILE *fp = fopen(COMMAND_FILE, "w");
//     if (fp) {
//         fprintf(fp, "stop_monitor\n");
//         fclose(fp);
//         wait(NULL); // Wait for monitor to terminate
//     }
// }

// void list_hunts() {
//     if (monitor_pid == -1) {
//         printf("Monitor is not running.\n");
//         return;
//     }
//     FILE *fp = fopen(COMMAND_FILE, "w");
//     if (fp) {
//         fprintf(fp, "list_hunts\n");
//         fclose(fp);
//     }
// }

// void list_treasures(char *hunt_id) {
//     if (monitor_pid == -1) {
//         printf("Monitor is not running.\n");
//         return;
//     }
//     FILE *fp = fopen(COMMAND_FILE, "w");
//     if (fp) {
//         fprintf(fp, "list_treasures %s\n", hunt_id);
//         fclose(fp);
//     }
// }

// void view_treasure(char *treasure_id) {
//     if (monitor_pid == -1) {
//         printf("Monitor is not running.\n");
//         return;
//     }
//     FILE *fp = fopen(COMMAND_FILE, "w");
//     if (fp) {
//         fprintf(fp, "view_treasure %s\n", treasure_id);
//         fclose(fp);
//     }
// }

// int main() {
//     struct sigaction sa;
//     sa.sa_handler = handle_sigchld;
//     sigemptyset(&sa.sa_mask);
//     sa.sa_flags = 0;
//     sigaction(SIGCHLD, &sa, NULL);

//     char command[256];
//     while (1) {
//         printf("Enter command (start_monitor, list_hunts, list_treasures <hunt_id>, view_treasure <treasure_id>, stop_monitor, exit): ");
//         fgets(command, sizeof(command), stdin);

//         if (strcmp(command, "start_monitor\n") == 0) {
//             start_monitor();
//         } else if (strcmp(command, "list_hunts\n") == 0) {
//             list_hunts();
//         } else if (strncmp(command, "list_treasures ", 15) == 0) {
//             list_treasures(command + 15);
//         } else if (strncmp(command, "view_treasure ", 14) == 0) {
//             view_treasure(command + 14);
//         } else if (strcmp(command, "stop_monitor\n") == 0) {
//             stop_monitor();
//         } else if (strcmp(command, "exit\n") == 0) {
//             if (monitor_pid != -1) {
//                 printf("Cannot exit while monitor is running. Please stop it first.\n");
//             } else {
//                 break;
//             }
//         } else {
//             printf("Unknown command.\n");
//         }
//     }

//     return 0;
// }

// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <signal.h>
// #include <sys/types.h>
// #include <sys/wait.h>
// #include <fcntl.h>

// #define COMMAND_FILE "monitor_command.txt"
// #define MAX_INPUT 256

// pid_t monitor_pid = -1;
// int monitor_active = 0;
// int waiting_for_exit = 0;

// void handle_child_exit(int sig) {
//     int status;
//     pid_t pid = waitpid(-1, &status, WNOHANG);
//     if (pid == monitor_pid) {
//         printf("Monitor terminated with status: %d\n", WEXITSTATUS(status));
//         monitor_active = 0;
//         waiting_for_exit = 0;
//         monitor_pid = -1;
//     }
// }

// void write_command(const char* hunt_id, const char* command, int treasure_id) {
//     FILE* f = fopen(COMMAND_FILE, "w");
//     if (f) {
//         fprintf(f, "%s\n%s\n%d\n", command, hunt_id ? hunt_id : "", treasure_id);
//         fclose(f);
//     }
// }

// void monitor_process() {
//     while (1) {
//         pause(); // Wait for signals
        
//         // Read command from file
//         FILE* f = fopen(COMMAND_FILE, "r");
//         if (!f) continue;
        
//         char command[MAX_INPUT];
//         char hunt_id[MAX_INPUT];
//         int treasure_id;
        
//         fgets(command, MAX_INPUT, f);
//         fgets(hunt_id, MAX_INPUT, f);
//         fscanf(f, "%d", &treasure_id);
//         fclose(f);
        
//         // Remove newlines
//         command[strcspn(command, "\n")] = 0;
//         hunt_id[strcspn(hunt_id, "\n")] = 0;
        
//         if (strcmp(command, "list_hunts") == 0) {
//             // Implementation for listing hunts
//             DIR* dir = opendir("hunts");
//             if (dir) {
//                 struct dirent* entry;
//                 while ((entry = readdir(dir)) != NULL) {
//                     if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
//                         printf("Hunt: %s\n", entry->d_name);
//                     }
//                 }
//                 closedir(dir);
//             }
//         } else if (strcmp(command, "list_treasures") == 0) {
//             char* args[] = {"./treasure_manager", "--list", hunt_id, NULL};
//             execvp(args[0], args);
//         } else if (strcmp(command, "view_treasure") == 0) {
//             char tid_str[16];
//             snprintf(tid_str, sizeof(tid_str), "%d", treasure_id);
//             char* args[] = {"./treasure_manager", "--view", hunt_id, tid_str, NULL};
//             execvp(args[0], args);
//         } else if (strcmp(command, "stop") == 0) {
//             usleep(500000); // Delay before exit
//             exit(0);
//         }
//     }
// }

// int main() {
//     // Set up signal handlers
//     struct sigaction sa;
//     memset(&sa, 0, sizeof(sa));
//     sa.sa_handler = handle_child_exit;
//     sigaction(SIGCHLD, &sa, NULL);
    
//     char input[MAX_INPUT];
//     char hunt_id[MAX_INPUT];
//     int treasure_id;
    
//     while (1) {
//         printf("treasure_hub> ");
//         fgets(input, MAX_INPUT, stdin);
//         input[strcspn(input, "\n")] = 0;
        
//         if (strcmp(input, "start_monitor") == 0) {
//             if (monitor_active) {
//                 printf("Monitor is already running\n");
//                 continue;
//             }
            
//             monitor_pid = fork();
//             if (monitor_pid == 0) {
//                 monitor_process();
//             } else {
//                 monitor_active = 1;
//                 printf("Monitor started with PID: %d\n", monitor_pid);
//             }
//         } else if (strcmp(input, "stop_monitor") == 0) {
//             if (!monitor_active) {
//                 printf("No monitor is running\n");
//                 continue;
//             }
//             waiting_for_exit = 1;
//             write_command(NULL, "stop", 0);
//             kill(monitor_pid, SIGUSR1);
//         } else if (strcmp(input, "list_hunts") == 0) {
//             if (!monitor_active || waiting_for_exit) {
//                 printf("Monitor is not available\n");
//                 continue;
//             }
//             write_command(NULL, "list_hunts", 0);
//             kill(monitor_pid, SIGUSR1);
//         } else if (strcmp(input, "list_treasures") == 0) {
//             if (!monitor_active || waiting_for_exit) {
//                 printf("Monitor is not available\n");
//                 continue;
//             }
//             printf("Enter hunt ID: ");
//             fgets(hunt_id, MAX_INPUT, stdin);
//             hunt_id[strcspn(hunt_id, "\n")] = 0;
//             write_command(hunt_id, "list_treasures", 0);
//             kill(monitor_pid, SIGUSR1);
//         } else if (strcmp(input, "view_treasure") == 0) {
//             if (!monitor_active || waiting_for_exit) {
//                 printf("Monitor is not available\n");
//                 continue;
//             }
//             printf("Enter hunt ID: ");
//             fgets(hunt_id, MAX_INPUT, stdin);
//             hunt_id[strcspn(hunt_id, "\n")] = 0;
//             printf("Enter treasure ID: ");
//             scanf("%d", &treasure_id);
//             getchar(); // Consume newline
//             write_command(hunt_id, "view_treasure", treasure_id);
//             kill(monitor_pid, SIGUSR1);
//         } else if (strcmp(input, "exit") == 0) {
//             if (monitor_active) {
//                 printf("Cannot exit while monitor is running\n");
//                 continue;
//             }
//             break;
//         } else {
//             printf("Unknown command\n");
//         }
//     }
    
//     return 0;
// }


// treasure_hub.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <fcntl.h>

#define CMD_FILE "/tmp/monitor_cmd"

pid_t monitor_pid = 0;

// --- Monitor signal handlers ---
void monitor_list_hunts() 
{
    DIR *dir = opendir("hunts");
    if (!dir) 
    {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) 
    {
        if ((entry->d_type == DT_DIR) && (strcmp(entry->d_name, ".") != 0) && (strcmp(entry->d_name, "..") != 0)) 
        {
            printf("Hunt found: %s\n", entry->d_name);
        }
    }
    closedir(dir);
}

void monitor_list_treasures(const char *hunt_id) 
{
    if (hunt_id == 0) 
    {
        return;
    }

    char path[256];
    snprintf(path, sizeof(path), "hunts/%s/treasures.dat", hunt_id);
    FILE *fp = fopen(path, "rb");
    if (!fp) 
    {
        perror("fopen");
        return;
    }

    int id, value;
    char username[32], clue[128];
    float lat, lon;

    printf("Treasures in %s:\n", hunt_id);
    while (fread(&id, sizeof(int), 1, fp) == 1) 
    {
        fread(username, sizeof(username), 1, fp);
        fread(&lat, sizeof(float), 1, fp);
        fread(&lon, sizeof(float), 1, fp);
        fread(clue, sizeof(clue), 1, fp);
        fread(&value, sizeof(int), 1, fp);

        printf("ID: %d, User: %s, Value: %d\n", id, username, value);
    }

    fclose(fp);
}

void monitor_view_treasure(const char *hunt_id, int treasure_id) 
{
    if (hunt_id == 0) 
    {
        return;
    }

    char path[256];
    snprintf(path, sizeof(path), "hunts/%s/treasures.dat", hunt_id);
    FILE *fp = fopen(path, "rb");
    if (!fp) 
    {
        perror("fopen");
        return;
    }

    int id, value;
    char username[32], clue[128];
    float lat, lon;

    while (fread(&id, sizeof(int), 1, fp) == 1) 
    {
        fread(username, sizeof(username), 1, fp);
        fread(&lat, sizeof(float), 1, fp);
        fread(&lon, sizeof(float), 1, fp);
        fread(clue, sizeof(clue), 1, fp);
        fread(&value, sizeof(int), 1, fp);

        if (id == treasure_id) 
        {
            printf("Treasure ID: %d\nUser: %s\nCoords: %.2f, %.2f\nClue: %s\nValue: %d\n", id, username, lat, lon, clue, value);
            break;
        }
    }

    fclose(fp);
}

void signal_handler(int sig) 
{
    FILE *cmd = fopen(CMD_FILE, "r");
    if (!cmd) return;

    char line[256];
    if (!fgets(line, sizeof(line), cmd)) 
    {
        fclose(cmd);
        return;
    }
    fclose(cmd);

    char *args[3] = { NULL };
    char *token = strtok(line, " \n");
    int i = 0;
    while (token && i < 3) 
    {
        args[i++] = token;
        token = strtok(NULL, " \n");
    }

    switch (sig) 
    {
        case SIGUSR1:
            monitor_list_hunts();
            break;
        case SIGUSR2:
            monitor_list_treasures(args[1]);
            break;
        case SIGTERM:
            if (args[1] && args[2])
                monitor_view_treasure(args[1], atoi(args[2]));
            break;
        case SIGINT:
            printf("Monitor stopping...\n");
            exit(0);
    }
}

void run_monitor() 
{
    signal(SIGUSR1, signal_handler);
    signal(SIGUSR2, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    while (1) pause(); // Wait for signals
}

void write_command_to_file(const char *cmdline) 
{
    FILE *f = fopen(CMD_FILE, "w");
    if (!f) 
    {
        perror("fopen");
        return;
    }
    fprintf(f, "%s\n", cmdline);
    fclose(f);
}

void start_monitor() 
{
    if (monitor_pid > 0) 
    {
        printf("Monitor is already running.\n");
        return;
    }

    pid_t pid = fork();
    if (pid == 0) 
    {
        run_monitor();
        exit(0);
    } 
    else if (pid > 0) 
    {
        monitor_pid = pid;
        printf("Monitor started with PID %d\n", monitor_pid);
    } 
    else 
    {
        perror("fork");
    }
}

void stop_monitor() 
{
    if (monitor_pid > 0) 
    {
        kill(monitor_pid, SIGINT);
        waitpid(monitor_pid, NULL, 0);
        monitor_pid = 0;
        printf("Monitor stopped.\n");
    } 
    else 
    {
        printf("No monitor running.\n");
    }
}

int main() 
{
    char line[256];

    printf("Welcome to treasure hub.\n");

    while (1) 
    {
        printf("hub> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin))
            break;

        line[strcspn(line, "\n")] = 0;

        if (strcmp(line, "start_monitor") == 0) 
        {
            start_monitor();
        } 
        else if (strcmp(line, "stop_monitor") == 0) 
        {
            stop_monitor();
        } 
        else if (strcmp(line, "list_hunts") == 0) 
        {
            if (monitor_pid <= 0) 
            {
                printf("Monitor not running.\n");
                continue;
            }
            write_command_to_file("list_hunts");
            kill(monitor_pid, SIGUSR1);
        } 
        else if (strncmp(line, "list_treasures ", 15) == 0) 
        {
            if (monitor_pid <= 0) 
            {
                printf("Monitor not running.\n");
                continue;
            }
            write_command_to_file(line);
            kill(monitor_pid, SIGUSR2);
        } 
        else if (strncmp(line, "view_treasure ", 14) == 0) 
        {
            if (monitor_pid <= 0) 
            {
                printf("Monitor not running.\n");
                continue;
            }
            write_command_to_file(line);
            kill(monitor_pid, SIGTERM);
        } 
        else if (strcmp(line, "exit") == 0) 
        {
            stop_monitor();
            break;
        } 
        else 
        {
            printf("Unknown command: %s\n", line);
        }

        usleep(50000); // slight delay to ensure output order
    }

    return 0;
}

