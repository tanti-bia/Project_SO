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

// void calculate_score() {
//     DIR *dir = opendir("hunts");
//     if (!dir) {
//         perror("opendir");
//         return;
//     }
//
//     struct dirent *entry;
//     while ((entry = readdir(dir)) != NULL) {
//         if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
//             int pipefd[2];
//             if (pipe(pipefd) == -1) {
//                 perror("pipe");
//                 continue;
//             }
//
//             pid_t pid = fork();
//             if (pid == 0) {
//                 // Child process
//                 close(pipefd[0]); // Close read
//                 dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to pipe
//                 execl("./score", "score", entry->d_name, (char *)NULL);
//                 perror("execl");
//                 exit(1);
//             } else if (pid > 0) {
//                 // Parent process
//                 close(pipefd[1]); // Close write
//                 char buffer[256];
//                 printf("Scores for hunt '%s':\n", entry->d_name);
//                 while (read(pipefd[0], buffer, sizeof(buffer) - 1) > 0) {
//                     buffer[255] = '\0';
//                     printf("%s", buffer);
//                 }
//                 close(pipefd[0]);
//                 waitpid(pid, NULL, 0);
//             }
//         }
//     }
//
//     closedir(dir);
// }

void calculate_score() {
    DIR *dir = opendir("hunts");
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            int pipefd[2];
            if (pipe(pipefd) == -1) {
                perror("pipe");
                continue;
            }

            pid_t pid = fork();
            if (pid == 0) {
                // Child process
                close(pipefd[0]); // Close read end
                dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to pipe
                close(pipefd[1]);
                execl("./score", "score", entry->d_name, (char *)NULL);
                perror("execl");
                exit(1);
            } else if (pid > 0) {
                // Parent process
                close(pipefd[1]); // Close write end

                printf("Scores for hunt '%s':\n", entry->d_name);

                char buffer[256];
                ssize_t bytesRead;
                while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
                    buffer[bytesRead] = '\0';
                    printf("%s", buffer);
                }
                close(pipefd[0]);
                waitpid(pid, NULL, 0);
            }
        }
    }

    closedir(dir);
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
        else if (strcmp(line, "calculate_score") == 0) {
            calculate_score();
        }
        else
        {
            printf("Unknown command: %s\n", line);
        }

        usleep(50000); // slight delay to ensure output order
    }

    return 0;
}

