#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

typedef struct {
    int treasure_id;
    char user_name[20];
    float latitude;
    float longitude;
    char clue[1000];
    int value;
} Treasure;

typedef struct {
    char user[20];
    int total_value;
} Score;

int main(int argc, char *argv[]) 
{
    if (argc != 2) 
    {
        fprintf(stderr, "Usage: %s <hunt_dir>\n", argv[0]);
        return 1;
    }

    char path[256];
    snprintf(path, sizeof(path), "%s/treasures.dat", argv[1]); //creaaza calea completa spre treasure.dat

    int fd = open(path, O_RDONLY); //deschide fisierul in mod citire
    if (fd < 0) 
    {
        perror("open");
        return 1;
    }

    Treasure t; //initializari
    Score scores[100];
    int score_count = 0; //useri unici

    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) //citeste comorile din fisier
    {
        int found = 0; 
        for (int i = 0; i < score_count; ++i) 
        {
            if (strcmp(scores[i].user, t.user_name) == 0) //verifica daca userul exisa deja in scores
            {
                scores[i].total_value += t.value;
                found = 1;
                break;
            }
        }
        if (!found) //daca e user nou, il adauga
        {
            strncpy(scores[score_count].user, t.user_name, sizeof(scores[score_count].user));
            scores[score_count].total_value = t.value;
            score_count++;
        }
    }

    close(fd); //inchide fisierul

    for (int i = 0; i < score_count; ++i) //afiseaza scorurile pentru fiecare user
    {
        printf("%s - %d\n", scores[i].user, scores[i].total_value);
    }

    return 0;
}
