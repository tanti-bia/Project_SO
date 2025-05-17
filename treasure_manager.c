#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>

#define max_username 32
#define max_clue 128
#define record_size sizeof(treasure)

typedef struct
{
    int id;
    char username [max_username];
    float latitude;
    float longitude;
    char clue [max_clue];
    int value;
} treasure;

void logg (char *hunt_id, char *action)
{
    char path [512];
    snprintf (path, sizeof(path), "hunts/%s/logged_hunt", hunt_id);
    int file = open (path, O_WRONLY | O_CREAT | O_APPEND, 0644);

    if (file < 0)
    {
        return;
    }

    time_t now = time (NULL);
    dprintf (file, "[%s] %s\n", ctime(&now), action);
    close (file);
}

void check_symlink (char *hunt_id)
{
    char link_name [64];
    snprintf (link_name, sizeof (link_name), "logged_hunt-%s", hunt_id);

    char target [512];
    snprintf (target, sizeof (target), "hunts/%s/logged_hunt", hunt_id);

    symlink (target, link_name);
}

void add_treasure(const char *hunt_name)
{
    treasure t;

    printf("Enter treasure ID (int): ");
    scanf("%d", &t.id);
    getchar(); // consume newline

    printf("Enter username: ");
    fgets(t.username, sizeof(t.username), stdin);
    t.username[strcspn(t.username, "\n")] = 0; // remove trailing newline

    printf("Enter latitude (double): ");
    scanf("%f", &t.latitude);
    printf("Enter longitude (double): ");
    scanf("%f", &t.longitude);
    getchar(); // consume newline

    printf("Enter clue: ");
    fgets(t.clue, sizeof(t.clue), stdin);
    t.clue[strcspn(t.clue, "\n")] = 0; // remove trailing newline

    printf("Enter value (int): ");
    scanf("%d", &t.value);

    // Build directory path
    char path_to_directory[1024];
    snprintf(path_to_directory, sizeof(path_to_directory), "hunts/%s", hunt_name);

    // Create directory if it doesn't exist
    struct stat st = {0};
    if (stat(path_to_directory, &st) == -1) {
        if (mkdir(path_to_directory, 0755) != 0) {
            perror("mkdir");
            return;
        }
    }

    // Build paths to both files
    char treasure_path[1024];
    char log_path[1024];

    if (snprintf(treasure_path, sizeof(treasure_path), "%s/treasures.dat", path_to_directory) >= sizeof(treasure_path)) {
        fprintf(stderr, "Path too long (treasures.dat)\n");
        return;
    }

    if (snprintf(log_path, sizeof(log_path), "%s/logged_hunt", path_to_directory) >= sizeof(log_path)) {
        fprintf(stderr, "Path too long (logged_hunt)\n");
        return;
    }

    // Write to binary file: treasures.dat
    int treasure_fd = open(treasure_path, O_CREAT | O_WRONLY | O_APPEND, 0644);
    if (treasure_fd == -1) {
        perror("open (treasures.dat)");
        return;
    }
    write(treasure_fd, &t, sizeof(treasure));
    close(treasure_fd);

    // Write to text log: logged_hunt
    int log_fd = open(log_path, O_CREAT | O_WRONLY | O_APPEND, 0644);
    if (log_fd == -1) {
        perror("open (logged_hunt)");
        return;
    }

    char log_buffer[2048];
    snprintf(log_buffer, sizeof(log_buffer),
             "ID: %d\nUsername: %s\nLatitude: %.6lf\nLongitude: %.6lf\nClue: %s\nValue: %d\n\n",
             t.id, t.username, t.latitude, t.longitude, t.clue, t.value);

    write(log_fd, log_buffer, strlen(log_buffer));
    close(log_fd);

    printf("Treasure added successfully.\n");
}


void list_treasures (char *hunt_id)
{
    char path_to_file [512];
    snprintf (path_to_file, sizeof (path_to_file), "hunts/%s/treasures.dat", hunt_id);

    struct stat s;

    if (stat (path_to_file, &s) == -1)
    {
        perror ("failed stat");
        return;
    }

    printf ("hunt: %s has size of %ld bytes and was last modified at %s\n", hunt_id, s.st_size, ctime (&s.st_mtime));

    int file = open (path_to_file, O_RDONLY);

    if (file < 0)
    {
        perror ("cannot open file");
        return;
    }

    treasure t;

    while (read (file, &t, sizeof (treasure)) == sizeof (treasure))
    {
        printf ("id %d, user %s, value %d\n", t.id, t.username, t.value);
    }

    close (file);
    logg (hunt_id, "list treasures:");
}

void view_treasure (char *hunt_id, int id)
{
    char path_to_file [512];
    snprintf (path_to_file, sizeof (path_to_file), "hunts/%s/treasures.dat", hunt_id);

    int file = open (path_to_file, O_RDONLY);

    if (file < 0)
    {
        perror ("cannot open file");
        return;
    }

    treasure t;
    while (read (file, &t, sizeof (treasure)) == sizeof (treasure))
    {
        if (t.id == id)
        {
            printf ("treasure id %d, user %s,coordinates (%f, %f), value %d\n", t.id, t.username, t.latitude, t.longitude, t.value);
            logg (hunt_id, "viewd treasure");
            close (file);
            return;
        }
    }

    printf ("cannot find treasure\n");
    close (file);
}

void remove_treasure (char *hunt_id, int id)
{
    char path_to_file [512];
    char temp [512];

    snprintf (path_to_file, sizeof (path_to_file), "hunts/%s/treasures.dat", hunt_id);
    snprintf (temp, sizeof (temp), "hunts/%s/tmp.dat", hunt_id);

    int file = open (path_to_file, O_RDONLY);
    int out = open (temp, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if ((file < 0) || (out < 0))
    {
        perror ("cannot open file");
        return;
    }

    treasure t;

    int found = 0;

    while (read (file, &t, sizeof (treasure)) == sizeof (treasure))
    {
        if (t.id != id)
        {
            write (out, &t, sizeof (treasure));
        }
        else
        {
            found = 1;
        }
    }

    close (file);
    close (out);

    rename (temp, path_to_file);

    if (found == 1)
    {
        printf ("removed treasure\n");
        logg (hunt_id, "treasure removed");
    }
    else
    {
        printf ("cannot find treasure\n");
    }
}

void remove_hunt (char *hunt_id)
{
    char path_to_directory [512];
    snprintf (path_to_directory, sizeof (path_to_directory), "hunts/%s", hunt_id);

    char path_to_file [512];

    if (snprintf(path_to_file, sizeof(path_to_file), "%s/treasures.dat", path_to_directory) >= sizeof(path_to_file))
    {
        fprintf(stderr, "Path too long (treasures.dat)\n");
        return;
    }

    if (snprintf(path_to_file, sizeof(path_to_file), "%s/treasures.dat", path_to_directory) >= sizeof(path_to_file))
    {
        fprintf(stderr, "Path too long (treasures.dat)\n");
        return;
    }
    unlink (path_to_file);

    if (snprintf(path_to_file, sizeof(path_to_file), "%s/logged_hunt", path_to_directory) >= sizeof(path_to_file))
    {
        fprintf(stderr, "Path too long (logged_hunt)\n");
        return;
    }
    unlink (path_to_file);

    rmdir (path_to_directory);

    char link_name [64];
    snprintf (link_name, sizeof (link_name), "logged_hunt-%s", hunt_id);
    unlink (link_name);

    printf ("removed hunt\n");
}

int main (int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf (stderr, "not enough arguments\n");
        return 1;
    }

    if (strcmp (argv[1], "--add") == 0)
    {
        add_treasure (argv[2]);
    }
    else if (strcmp (argv[1], "--list") == 0)
    {
        list_treasures (argv[2]);
    }
    else if ((strcmp (argv[1], "--view") == 0) && (argc == 4))
    {
        view_treasure (argv[2], atoi (argv[3]));
    }
    else if ((strcmp (argv[1], "--remove_treasure") == 0) && (argc == 4))
    {
        remove_treasure (argv[2], atoi (argv[3]));
    }
    else if (strcmp (argv[1], "--remove") == 0)
    {
        remove_hunt (argv[2]);
    }
    else
    {
        fprintf (stderr, "command not found\n");
        return 1;
    }
    return 0;
}