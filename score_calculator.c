// score_calculator.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_USERNAME 32
#define MAX_USERS 100

typedef struct
{
    int id;
    char username[MAX_USERNAME];
    float latitude;
    float longitude;
    char clue[128];
    int value;
} treasure;

typedef struct
{
    char username[MAX_USERNAME];
    int total_value;
} user_score;

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <hunt_id>\n", argv[0]);
        return 1;
    }

    char path[256];
    snprintf(path, sizeof(path), "hunts/%s/treasures.dat", argv[1]);
    FILE *file = fopen(path, "rb");
    if (!file)
    {
        perror("fopen");
        return 1;
    }

    user_score users[MAX_USERS];
    int count = 0;

    treasure t;
    while (fread(&t, sizeof(treasure), 1, file) == 1)
    {
        int found = 0;
        for (int i = 0; i < count; ++i)
        {
            if (strcmp(users[i].username, t.username) == 0)
            {
                users[i].total_value += t.value;
                found = 1;
                break;
            }
        }
        if (!found && count < MAX_USERS)
        {
            strncpy(users[count].username, t.username, MAX_USERNAME);
            users[count].total_value = t.value;
            count++;
        }
    }

    fclose(file);

    for (int i = 0; i < count; ++i)
    {
        printf("%s %d\n", users[i].username, users[i].total_value);
    }

    return 0;
}
