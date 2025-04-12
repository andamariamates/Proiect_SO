#include <stdio.h> //log_action, create_symbolic_link, remove_symbolic_link, build_filepath, view, remove_treasure, remove_hunt, remove_hunt
#include <fcntl.h> //log_action, create_symbolic_link, add, view, remove_treasure
#include <unistd.h> //log_action, add, list, view, remove_ treasure, remove_hunt
#include <sys/stat.h> //create_symbolic_link, create_hunt_directory, list, remove_hunt
#include <stdlib.h> //view, remove_treasure, remove_hunt
#include <string.h> //build_filepath, view
#include <errno.h> //log_action, create_symbolic_link, remove_symbolic_link, create_hunt_directory, add, view, remove_treasure, remove_hunr
#include <dirent.h> //list, remove_hunt
#include <time.h> //list
#include <sys/types.h> //remove_hunt

typedef struct Treasure {
    int treasure_id;
    char user_name[20];
    float latitude;
    float longitude;
    char clue[1000];
    int value;
} Treasure;

void log_action(const char* hunt_dir, const char *action) 
{ //scop : logheaza actiunea in fisierul log.txt
    char log_path[100];
    snprintf(log_path, sizeof(log_path), "%s/log.txt", hunt_dir); //creeaza calea catre fisierul lox.txt
    int fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0644); //deschide fisierul log.txt in mod scriere
    if (fd >= 0) //am deschis fisierul cu succes
    { 
        write(fd, action, strlen(action)); //scriem actiunea in fisier
        write(fd, "\n", 1); //adaugam un newline
        close(fd); //inchidem fisierul
    } 
    else //tratam posibila eroare
    { 
        perror("log"); 
    }
}

void create_symbolic_link(const char *hunt_dir, const char *hunt_id) 
{ //scop : creeaza un link simbolic catre fisierul log.txt ca utilizatorul sa-l poata accesa mai usor
    char target[200], link_name[200]; //target = calea catre fisierul log.txt, link_name = numele link-ului simbolic
    snprintf(target, sizeof(target), "%s/log.txt", hunt_dir); //creeaza calea catre fisierul log.txt
    snprintf(link_name, sizeof(link_name), "logged_hunt-%s", hunt_id); //creeaza numele link-ului simbolic
    struct stat st; 
    if (lstat(link_name, &st) == 0) //verficam daca exista deja un link simbolic
    {
        if (S_ISLNK(st.st_mode)) //verificam daca este un link simbolic
        {
            printf("Exista deja un link symbolic cu numele %s.\n", link_name);
            return;
        }
    }
    if (symlink(target, link_name) == -1) //cream link-ul simbolic si tratam posibila eroare
    {
        perror("symlink");
        return;
    }
}

void remove_symbolic_link(const char *hunt_id) 
{ //scop : sterge link-ul simbolic creat anterior
    char link_name[200];
    snprintf(link_name, sizeof(link_name), "logged_hunt-%s", hunt_id); //creeaza numele link-ului simbolic
    if (unlink(link_name) == -1) //incercam sa stergem link-ul simbolic si tratam posibila eroare
    {
        perror("unlink");
    }
}


void create_hunt_directory(const char *dir_name) 
{ //scop : creeaza un director de hunt
    if (mkdir(dir_name, 0755) == -1 && errno != EEXIST) //incercam sa cream directorul si verificam daca acesta exista deja
    {
        perror("mkdir");
    }
}

void build_filepath(char *string, const char *dir_name) 
{ //scop : creeaza calea catre fisierul treasure.dat
    snprintf(string, 256, "%s/treasures.dat", dir_name); //creeaaza calea catre fisierul treasure.dat
}

void add(const char *hunt_path, Treasure *t) 
{ //scop : adauga un treaure in fisierul treasure.dat
    int fd = open(hunt_path, O_WRONLY | O_CREAT | O_APPEND, 0644); //deschide fisierul treasure.dat in mod scriere
    if (fd < 0) //tratam posibila eroare
    {
        perror("open");
        return;
    }
    if (write(fd, t, sizeof(Treasure)) != sizeof(Treasure)) //scriem in fisierul treasure.dat si tratam posibila eroare
    {
        perror("write");
    }
    close(fd); //inchidem fisierul
}

void list(const char *hunt_dir) 
{ //scop : listeaza toate comorile din fisierul treasure.dat
    struct stat st; 
    if (stat(hunt_dir, &st) != 0) //verificam daca directorul exista si tratam posibila eroare
    {
        perror("list, dir");
        return;
    }
    char filepath[200]; 
    build_filepath(filepath, hunt_dir);
    printf("Numele vanatorii: %s\n", hunt_dir);
    printf("Dimensiunea totala a fisierului: %lld bytes\n", (long long)st.st_size); 
    printf("Ultima modificare: %s", ctime(&st.st_mtime));
    int fd = open(filepath, O_RDONLY); //deschidem fisierul treasure.dat in mod citire
    if (fd < 0) //tratam posibila eroare
    {
        perror("open");
        return;
    }
    Treasure t; 
    ssize_t bytes_read; 
    int count = 0;
    while ((bytes_read = read(fd, &t, sizeof(Treasure))) == sizeof(Treasure)) //scriem in fisierul treasure.dat pana conditia se verifica
    {
        printf("Comoara %d: ID = %d, Jucator = %s, Latitudine = %.2f, Longitudine = %.2f, Indiciu = %s, Valoare = %d\n", ++count, t.treasure_id, t.user_name, t.latitude, t.longitude, t.clue, t.value);
    }
    close(fd); //inchidem fisierul
}

void view(const char *hunt_dir, int search_id) 
{ //scop : cauta o comoara dupa id
    char filepath[256];
    build_filepath(filepath, hunt_dir);
    int fd = open(filepath, O_RDONLY); //deschidem fisierul treasure.dat in mod citire
    if (fd < 0) //tratam posibila eroare
    {
        perror("open");
        return;
    }
    Treasure t;
    int found = 0;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) //citim din fisierul treasure.dat pana conditia se verfica
    {
        if (t.treasure_id == search_id) //daca am gasit comoara cu id-ul cautat
        { //o printam si schimbam valoare flag-ului found
            printf("Comoara gasita: ID=%d, Jucator=%s, Latitudine = %.2f, Longitudine = %.2f, Indiciu = %s, Valoare = %d\n", t.treasure_id, t.user_name, t.latitude, t.longitude, t.clue, t.value);
            found = 1;
            break;
        }
    }
    if (!found) //daca nu am gasit comoara cu id-ul cautat
    {
        printf("Comoara cu ID-ul %d nu a fost gasita.\n", search_id);
    }
    close(fd); 
}

void remove_hunt(const char *hunt_dir) 
{ //scop : stergerea unui intreg hunt
    struct stat st;
    if (stat(hunt_dir, &st) != 0) 
    { //daca nu exista directorul tratam posibila eroare
        perror("remove, dir");
        return;
    }
    DIR *dir = opendir(hunt_dir); //deschidem directorul
    if (!dir) //tratam posibila eroare
    {
        perror("opendir");
        return;
    }
    struct dirent *entry; 
    char path[100];
    while ((entry = readdir(dir)) != NULL) //cat timp citim din director fisiere
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) //ignorma "." si ".." care se gasesc in orice director
            continue;
        snprintf(path, sizeof(path), "%s/%s", hunt_dir, entry->d_name); //creeaza calea catre fisierul din director
        struct stat st;
        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) //verificam daca este un director
        {
            remove_hunt(path); //apelam recursiv functia pentru a sterge posibilele fisiere
        } 
        else 
        {
            if (remove(path) != 0) //incercam sa stergem toate fisierele
                perror("remove");
        }
    }
    closedir(dir); //inchidem directoturl
    if (rmdir(hunt_dir) == 0) //incercam sa stergem directorul si tratam posibila eroare
    {
        remove_symbolic_link(hunt_dir);
    } 
    else 
    {
        perror("rmdir");
    }
}

void remove_treasure(const char *hunt_dir, int delete_id) 
{ //scop : sterge o singura comoara din fisierul treasure.dat
    char filepath[100], temp_filepath[100]; //filepath = calea catre treasure.dat, temp_filepath = calea catre fisierul temporat unde vor fi scrise comorile care sunt pastrate
    snprintf(filepath, sizeof(filepath), "%s/treasures.dat", hunt_dir); //creeaza calea catre treasure.dat
    snprintf(temp_filepath, sizeof(temp_filepath), "%s/temp.dat", hunt_dir); //creeaza calea catre fisierul temporar
    int fd_read = open(filepath, O_RDONLY); //deschidem fisierul treasure.dat in mod citire
    if (fd_read < 0) //tratam posibila eroare
    {
        perror("open, read");
        return;
    }
    int fd_write = open(temp_filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644); //deschidem si creeam fisierul temporar in mod scriere
    if (fd_write < 0) //tratam posbila eroare
    {
        perror("open, write");
        close(fd_read); //inchidem fisierul treasure.dat
        return;
    }
    Treasure t;
    int found = 0; //flag
    while (read(fd_read, &t, sizeof(Treasure)) == sizeof(Treasure)) //citim pe rand cate o comoara din fisierul treasures.dat
    {
        if (t.treasure_id != delete_id) //daca id-ul comorii nu este cel cautat
        {
            write(fd_write, &t, sizeof(Treasure)); //il scriem in fisierul temporar
        }
        else //altfel, am gasit comoara
        {
            found = 1; //update flag
        }
    }
    close(fd_read); //inchidem fisierul treasures.dat
    close(fd_write); //inchideem fisierul temporar
    rename(temp_filepath, filepath); //fisierul temporar devine noul treasures.dat
    if(!found)
    {
        printf("Comoara cu ID-ul %d nu a fost gasita\n", delete_id);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2) 
    {
        printf("Eroare! Actiune lipsa.\n");
        return 1;
    }
    const char *actiune = argv[1];
    const char *hunt_id = NULL;
    char comanda[100] = "";
    for (int i = 1; i < argc; i++) 
    {
        strcat(comanda, argv[i]);
        if (i < argc - 1)
            strcat(comanda, " ");
    }
    if (strcmp(actiune, "add") == 0 || strcmp(actiune, "list") == 0 ||strcmp(actiune, "view") == 0 || strcmp(actiune, "remove_treasure") == 0 ||strcmp(actiune, "remove_hunt") == 0) 
    {
        if (argc >= 3) 
        {
            hunt_id = argv[2];
        } 
        else 
        {
            printf("Utilizare: %s <hunt_id> ...\n", actiune);
            return 1;
        }
    }

    if (strcmp(actiune, "add") == 0) 
    {
        if (argc != 3) 
        {
            printf("Utilizare: add <hunt_id>\n");
            return 1;
        }
        create_hunt_directory(hunt_id);
        char filepath[256];
        build_filepath(filepath, hunt_id);
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
        scanf(" %[^\n]", t.clue);
        printf("Valoare: ");
        scanf("%d", &t.value);
        add(filepath, &t);
        create_symbolic_link(hunt_id, hunt_id);
        log_action(hunt_id, comanda);
    } 
    else if (strcmp(actiune, "list") == 0) 
    {
        if (argc != 3) 
        {
            printf("Utilizare: list <hunt_id>\n");
            return 1;
        }
        log_action(hunt_id, comanda);
        list(hunt_id);
    } 
    else if (strcmp(actiune, "view") == 0) 
    {
        if (argc != 4) 
        {
            printf("Utilizare: view <hunt_id> <id>\n");
            return 1;
        }
        log_action(hunt_id, comanda);
        int id = atoi(argv[3]);
        view(hunt_id, id);
    } 
    else if (strcmp(actiune, "remove_treasure") == 0) 
    {
        if (argc != 4) 
        {
            printf("Utilizare: remove_treasure <hunt_id> <id>\n");
            return 1;
        }
        log_action(hunt_id, comanda);
        int id = atoi(argv[3]);
        remove_treasure(hunt_id, id);
    } 
    else if (strcmp(actiune, "remove_hunt") == 0) 
    {
        if (argc != 3) 
        {
            printf("Utilizare: remove_hunt <hunt_id>\n");
            return 1;
        }
        remove_hunt(hunt_id);
    } 
    else 
    {
        printf("Actiune necunoscuta: %s\n", actiune);
        return 1;
    }
    return 0;
}