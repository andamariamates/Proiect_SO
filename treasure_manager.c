#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef struct Treasure {
    int treasure_id;
    char user_name[20];
    float latitude;
    float longitude;
    char clue[100];
    int value;
}Treasure;

void create_hunt_directory(const char *dir_name)
{
    if(mkdir(dir_name, 0755) == -1 && errno != EEXIST)
    {
        perror("hello");
        return;
    }
}

void build_filepath(char *buffer, const char* dir_name)
{
    snprintf(buffer, 256, "%s/treasures.txt", dir_name);
}

void add(const char* filepath, Treasure *t)
{
    int fd = open(filepath, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if(fd < 0)
    {
        perror("open");
        return;
    }
    char buffer[256];
    int len = snprintf(buffer, sizeof(buffer), "%d %s %.2f %.2f %s %d\n", t->treasure_id, t->user_name, t->latitude, t->longitude, t->clue, t->value);

    if (write(fd, buffer, len) != len)
    {
        perror("write");
    }
    else
    {
        printf("Scriere cu succes!\n");
    }
}

int main()
{
    char hunt_name[20];
    printf("Introduceti numele hunt-ului: ");
    scanf("%s", hunt_name);

    create_hunt_directory(hunt_name);

    char filepath[256];
    build_filepath(filepath, hunt_name);

    int action;
    do
    {
        printf("\nAlegeti actiunea dorita:\n");
        printf("1. Adauga treasure:\n");
        printf("2. Iesire\n");
        printf("Alegeti o optiune: ");
        scanf("%d", &action);

        switch (action)
        {
        case 1:
        {
            Treasure t1;
            printf("Introduceți ID-ul comorii: ");
            scanf("%d", &t1.treasure_id);
            printf("Introduceți numele jucătorului: ");
            scanf("%s", t1.user_name);
            printf("Introduceți latitudinea și : ");
            scanf("%f", &t1.latitude);
            printf("Introduceți longitudinea: ");
            scanf("%f", &t1.longitude);
            printf("Introduceți indicii (clue): ");
            scanf("%s", t1.clue);
            printf("Introduceți valoarea comorii: ");
            scanf("%d", &t1.value);

            add(filepath, &t1);
            break;
        }
        case 2:
            break;
        default:
            printf("gresit\n");
            break;
        }
    } while (action != 2);

    return 0;
}