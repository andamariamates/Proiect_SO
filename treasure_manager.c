#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h> //folosita la create_hunt_directory
#include <dirent.h> //folosita la remove_hunt

typedef struct Treasure 
{
    int treasure_id;
    char user_name[20];
    float latitude;
    float longitude;
    char clue[1000];
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
{ //scop: stergerea sirului de caractere hunt_dir 
    struct stat st; //informatii despre directorul specificat
    if (stat(hunt_dir, &st) != 0) //verifica daca directorul hunt_dir exista folosind stat-ul declarat anterior
    {
        perror("remove, dir"); //afiseaza eroare la incercarea de a sterge directorul in cauza
        return;
    }

    DIR *dir = opendir(hunt_dir); //opendir, mai "usoara" decat open, dar e speciala pentru directoare, ca sa le putem deschide la momentul compilarii
    if (!dir) //eroare la deschiderea directorului
    {
        perror("opendir");
        return;
    }

    struct dirent *entry; //dirent, ca un fel de stat, este scris deja in bilioteca <dirent.h> si are infromatiile despre fiecare fisier sau director in parte
    char path[100];

    while ((entry = readdir(dir)) != NULL) //parcurge intrarile, adica fisierrel si directoarele din hunt_dir
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        //verifica daca intrarea e directorul curent sau directorul parinte, pe care le evitam ca sa nu ramanem blocati in ciclu infinit

        snprintf(path, sizeof(path), "%s/%s", hunt_dir, entry->d_name);
        //concatenam calea completa a fisierului sau directorului curent cu '/' ca sa stim ce trebuie sters

        struct stat entry_stat; //informatiile despre fisiere
        if (stat(path, &entry_stat) == 0 && S_ISDIR(entry_stat.st_mode)) //a doua verifica saca e director, prima daca e fisier. in orice caz, trebuie sa-l stergem
        {
            remove_hunt(path);
        } 
        else {
            if (remove(path) != 0)
                perror("remove");
        }
    }
    closedir(dir); //am terminat de mers prin director si de sters fisierele din el

    if (rmdir(hunt_dir) == 0) //stergem directorul in sine
    {
        printf("Hunt-ul '%s' a fost sters cu succes!\n", hunt_dir);
    } 
    else //posibila eroare la stergerea directorului 
    {
        perror("rmdir");
    }

    char msg[100];
    snprintf(msg, sizeof(msg), "delete %s", hunt_dir); //concatenam mesajul de logare cu numele directorului ca sa  il adaugam in fisierul log.txt
    log_action(msg);
}

void remove_treasure(const char *hunt_dir, int delete_id) 
{ //scop: stergerea unei comori cu id-ul int delete_if dintr-un hunt hunt_dir
    char filepath[100], temp_filepath[100];
    //filepath contine calea completa catre fisieruul "treasure.txt" din directorul hunt_dir
    //temp_filepath continea calea completa catre un fisier temporar "temp.txt" care va fi folosit pentru a rescrie datele 
    snprintf(filepath, sizeof(filepath), "%s/treasures.txt", hunt_dir);
    snprintf(temp_filepath, sizeof(temp_filepath), "%s/temp.txt", hunt_dir);

    int fd_read = open(filepath, O_RDONLY);
    //deschidem fisierul "treasure.txt" in modul de citire 
    if (fd_read < 0) //posibila eroare
    {
        perror("open read");
        return;
    }

    int fd_write = open(temp_filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    //deschidem fisierul temporar "temp.txt" in modul scriere + se creeaza si trunchiaza aka sterge ce era deja
    //0644 = 0000 0110 0100 0100
    if (fd_write < 0) //posibila eroare
    {
        perror("open write");
        close(fd_read); //renuntam aka inchidem fisierul
        return;
    }

    char buf[1000]; //un buffer pentru a stoca toate datele din "treasure.txt" daca e nevoie
    ssize_t bytes_read; //il vom folosi la write
    char line[200]; //construim o linie odata ce avem informatiile bune, ca s-o scriem in "temp.txt"
    int i = 0, kept = 0;
    //kept tine cont cate linii s-au sters si il vom folosi in sine la finalul functiei sa stim ce scriem in terminal pentru utilizator
    while ((bytes_read = read(fd_read, buf, sizeof(buf))) > 0) {
        //citim dae din "treasure.txt" in buff, cate sizeof(buff) deodata
        for (ssize_t j = 0; j < bytes_read; ++j) //traversam fiecare caracter din buff
        {
            if (buf[j] == '\n' || i == sizeof(line) - 1) //daca am ajuns la un final de linie sau daca linia a atins dimeniunea maxima
            {
                line[i] = '\0'; //terminam linia 
                int id; 
                sscanf(line, "%d", &id); //extragem primul numar care reprezinta id-ul; stim asta din enunt, asa bagam datele de la tastatura
                if (id != delete_id) { //daca nu e id-ul cautat
                    write(fd_write, line, strlen(line)); //scriem linia in fisierul temporar, pentru ca nu pe ea o cautam
                    write(fd_write, "\n", 1); //adaugam si un new line sa arate bine
                    kept++; //crestem contoroul de linii pastrate
                }
                i = 0;
            } else { //daca linia nu e completa aka a trecut de limita noastra, adaugam caracterul curent din buffer in linia curenta ca sa nu pierdem dateee
                line[i++] = buf[j];
            }
        }
    }

    close(fd_read); //am terminat algoritmul de stergere, asa ca inchidem fisierele
    close(fd_write);
    rename(temp_filepath, filepath); //"temp.txt" devine noul "treasure.txt"

    printf("Comoara cu ID-ul %d %s.\n", delete_id, kept ? "a fost stearsa" : "nu a fost gasita");

    char msg[100];
    snprintf(msg, sizeof(msg), "remove_treasure %d", delete_id); //concatenam mesajul de logare cu id-ul comorii ca sa  il adaugam in fisierul log.txt
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