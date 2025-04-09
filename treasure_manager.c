#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h> //folosita la create_hunt_directory
#include <dirent.h>

typedef struct Treasure 
{
    int treasure_id;
    char user_name[20];
    float latitude;
    float longitude;
    char clue[100];
    int value;
} Treasure;

void log_action(const char *action) 
{ //action = numele actiunii ce se va scrie in fisierul text
    int log_fd = open("log.txt", O_WRONLY | O_CREAT | O_APPEND, 0644); //creeaza, deschide si scrie la finalul fisierul "log.txt"
    //0644 = 0000 0110 0100 0100
    if (log_fd >= 0) //succes
    {
        write(log_fd, action, strlen(action)); //scriem in fisierul "log.txt" numele actiunii de lungime strlen(action)
        write(log_fd, "\n", 1); //trecem pe randul urmator
        close(log_fd); //inchidem pe moment fisierul "log.txt"
    }
    else //daca nu s-a deschis fisierul "log.txt" 
    {
        perror("log"); //=> eroare
        return;
    }
}

void create_hunt_directory(const char *dir_name) 
{ //dir_name reprezinta numele directorului in care va trebui sa lucram
    if (mkdir(dir_name, 0755) /*creeaza un director*/ == -1 /*eroare la creare*/ && errno != EEXIST /*dar nu e eroare la existenta sa*/) {
    //0755 = 0000 0111 0101 0101
        perror("mkdir"); //tiparim o eroare la mkdir
    }
    //altfel, continuam cursul programului
    log_action("create_hunt_directory"); //adaugam in fisierul de "log.txt" creat anterior actiunea "create_hunt_directory", pentru ca s-a facut cu succes
}

void build_filepath(char *string, const char *dir_name) 
{ 
    snprintf(string, 256, "%s/treasures.txt", dir_name);
    //snprintf = construieste un string de maxim 256 de caractere, concatenand cele doua string-uri
}

void add(const char *hunt_dir, Treasure *t) 
{ //scop: adaugarea unei comori intr-o vanatoare
    int fd = open(hunt_dir, O_WRONLY | O_CREAT | O_APPEND, 0644);//creeaza, deschide si scrie la finalul fisierul
    if (fd < 0) { //eroare la deschiderea fisierului
        perror("open");
        return; //iesim din functia add
    }

    char buffer[256];
    int len = snprintf(buffer, sizeof(buffer), "%d %s %.2f %.2f %s %d\n", t->treasure_id, t->user_name, t->latitude, t->longitude, t->clue, t->value);
    //concatenam buffer-ul (inainte, gol) cu exact fiecare continut al campului din Treasure t, ca sa putem face fiser text, nu binar, fara erori
    write(fd, buffer, len);
    //scriem in fisier exact ce am primit de la tastatura
    close(fd); //inchidem fisierul

    log_action("add"); //adaugam in fisierul de "log.txt" creat anterior actiunea "add", pentru ca s-a facut cu succes
}

void list(const char *hunt_dir) 
{ //scop: listarea continutului fisierului "treasure.txt" in terminal 
    struct stat st; //stocam info despre un fisier sau director (permisiuni, dimenisune, tip)
    if (stat(hunt_dir, &st) != 0) //verificam daca directorul specificat in "hunt_dir" exista 
    { 
        perror("list, dir");
        return;
    }

    char filepath[256];
    build_filepath(filepath, hunt_dir); //ne folosim de functia de mai sus pentru a concatena informatiile

    int fd = open(filepath, O_RDONLY); //deschidem fisierul concatenat mai sus
    if (fd < 0) //posibila eroare la deschidere
    { 
        perror("open");
        return;
    }

    char buf[1024];
    ssize_t bytes_read;
    write(0, "Comori:\n", 10); //scriem in terminal

    while ((bytes_read = read(fd, buf, sizeof(buf))) > 0) { //citim fin fisier
        write(0, buf, bytes_read); //scriem in terminal
    }

    close(fd); //inchidem fisierul
    log_action("list"); //adaugam in fisierul de "log.txt" creat anterior actiunea "list", pentru ca s-a facut cu succes
}

void view(const char *hunt_dir, int search_id) 
{
    char filepath[256];
    build_filepath(filepath, hunt_dir); //ne folosim de functia de mai sus pentru a concatena informatiile

    int fd = open(filepath, O_RDONLY); //deschidem fisierul concatenat mai sus
    if (fd < 0) //posibila eroare la deschidere
    {
        perror("open");
        return;
    }

    char buf[1024];
    ssize_t bytes_read;
    int found = 0;

    while ((bytes_read = read(fd, buf, sizeof(buf) - 1)) > 0) { //cat timp citim "ceva" din fisier
        buf[bytes_read] = '\0'; //ultimul caracter indica ca am terminat o linie
        char *line = strtok(buf, "\n"); //cand gaseste new line, ne atentioneaza ca am terminat o linie
        while (line) { //cat timp exista linii
            int id;
            sscanf(line, "%d", &id); //citim id-ul
            if (id == search_id) 
            { //daca e id-ul de care avem nevoie
                printf("Comoara gasita: %s\n", line); //scriem la tasatatura linia
                found = 1; //flag
                break; 
            }
            line = strtok(NULL, "\n"); //continua pana cand gaseste eof sau new line
        }
        if (found == 1) 
        {
            break; //daca s-a gasit intre timp, iesim din while
        }
    }

    if (!found)  //daca nu am gasit deloc id-ul, scriem la tastatura
    {
        printf("Comoara cu ID-ul %d nu a fost gasita.\n", search_id);
    }
    close(fd);

    log_action("view"); //adaugam in fisierul de "log.txt" creat anterior actiunea "view", pentru ca s-a facut cu succes
}

void remove_hunt(const char *hunt_dir) 
{
    struct stat st;
    if (stat(hunt_dir, &st) != 0) 
    {
        perror("remove, dir");
        return;
    }

    DIR *dir = opendir(hunt_dir);
    if (!dir) 
    {

        perror("opendir");
        return;
    }

    struct dirent *entry;
    char path[512];

    while ((entry = readdir(dir)) != NULL) 
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        snprintf(path, sizeof(path), "%s/%s", hunt_dir, entry->d_name);
        remove(path);
    }
    closedir(dir);

    if (rmdir(hunt_dir) == 0) 
    {
        printf("Hunt-ul '%s' a fost sters cu succes!\n", hunt_dir);
    } 
    else 
    {
        perror("rmdir");
    }

    char msg[300];
    snprintf(msg, sizeof(msg), "delete %s", hunt_dir);
    log_action(msg);
}

void remove_treasure(const char *hunt_dir, int delete_id) 
{
    char filepath[256], temp_filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/treasures.txt", hunt_dir);
    snprintf(temp_filepath, sizeof(temp_filepath), "%s/temp.txt", hunt_dir);

    int fd_read = open(filepath, O_RDONLY);
    if (fd_read < 0) 
    {
        perror("open read");
        return;
    }

    int fd_write = open(temp_filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_write < 0) 
    {
        perror("open write");
        close(fd_read);
        return;
    }

    char buf[1024];
    ssize_t bytes_read;
    char line[256];
    int i = 0, kept = 0;

    while ((bytes_read = read(fd_read, buf, sizeof(buf))) > 0) {
        for (ssize_t j = 0; j < bytes_read; ++j) 
        {
            if (buf[j] == '\n' || i == sizeof(line) - 1) 
            {
                line[i] = '\0';
                int id;
                sscanf(line, "%d", &id);
                if (id != delete_id) {
                    write(fd_write, line, strlen(line));
                    write(fd_write, "\n", 1);
                    kept++;
                }
                i = 0;
            } else {
                line[i++] = buf[j];
            }
        }
    }

    close(fd_read);
    close(fd_write);
    rename(temp_filepath, filepath);

    printf("Comoara cu ID-ul %d %s.\n", delete_id, kept ? "a fost stearsa" : "nu a fost gasita");

    char msg[100];
    snprintf(msg, sizeof(msg), "remove_treasure %d", delete_id);
    log_action(msg);
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
        printf("1. Adauga treasure\n");
        printf("2. Listeaza comorile\n");
        printf("3. Detalii comoara dupa ID\n");
        printf("4. Sterge hunt\n");
        printf("5. Sterge comoara dupa ID\n");
        printf("6. Iesire\n");
        printf("Optiune: ");
        scanf("%d", &action);

        switch (action) {
            case 1: {
                Treasure t;
                printf("ID comoara: ");
                scanf("%d", &t.treasure_id);
                printf("Nume jucator: ");
                scanf("%s", t.user_name);
                printf("Latitudine: ");
                scanf("%f", &t.latitude);
                printf("Longitudine: ");
                scanf("%f", &t.longitude);
                printf("Indiciu: ");
                scanf("%s", t.clue);
                printf("Valoare: ");
                scanf("%d", &t.value);
                add(filepath, &t);
                break;
            }
            case 2:
                list(hunt_name);
                break;
            case 3: {
                int id;
                printf("ID comoara: ");
                scanf("%d", &id);
                view(hunt_name, id);
                break;
            }
            case 4:
                remove_hunt(hunt_name);
                break;
            case 5: {
                int id;
                printf("ID comoara de sters: ");
                scanf("%d", &id);
                remove_treasure(hunt_name, id);
                break;
            }
            case 6:
                log_action("exit");
                break;
            default:
                printf("Optiune invalida.\n");
                break;
        }
    } while (action != 6);

    return 0;
}