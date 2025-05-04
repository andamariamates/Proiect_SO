#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

typedef struct {
    int treasure_id;
    char user_name[20];
    float latitude;
    float longitude;
    char clue[1000];
    int value;
} Treasure;

volatile sig_atomic_t running = 1; //flag global pentru a controla bucla principala
//daca se primeste SIGUSR2 se va opri monitorul

void write_str(const char *s) {
    write(STDOUT_FILENO, s, strlen(s));
}

void list_hunts() {
    DIR *dir = opendir("."); //deschide directorul curent
    if (!dir) return; //daca deschiderea esueaza, iesim
    struct dirent *entry; 
    while ((entry = readdir(dir))) { //parcurge toate fisierele din director
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) { 
            //verfica toate directoarele, ignorand "." si ".."
            char path[256];
            snprintf(path, sizeof(path), "%s/treasures.dat", entry->d_name); //construieste calea catre fisierul "treasures.dat"
            int fd = open(path, O_RDONLY); //deschide fisierul in mod citire
            if (fd < 0) continue; //esueaza, trecem la urmatorul
            int count = 0; //numara comorile
            Treasure t; 
            while (read(fd, &t, sizeof(t)) == sizeof(t)) count++; //cat timp citim o comoara + info. sale, numaram
            close(fd); //inchidem fisierul
            char out[256];
            snprintf(out, sizeof(out), "%s - %d comori\n", entry->d_name, count); 
            write_str(out); //afisam rezultatul pentru directorul curent
        }
    }
    closedir(dir);
}

void list_treasures(const char *hunt_name) {
    char path[256];
    snprintf(path, sizeof(path), "%s/treasures.dat", hunt_name); //construieste calea catre fisierul "treasure.dat"
    int fd = open(path, O_RDONLY); //deschidem fisierul in mod citire
    if (fd < 0) { //posibila eroare
        write_str("Nu s-a putut deschide fișierul.\n");
        return;
    }
    Treasure t;
    while (read(fd, &t, sizeof(t)) == sizeof(t)) { //cat timp citim o comoara
        char out[1200];
        snprintf(out, sizeof(out), "ID: %d, User: %s, Lat: %.2f, Long: %.2f, Indiciu: %s, Val: %d\n", t.treasure_id, t.user_name, t.latitude, t.longitude, t.clue, t.value); 
        write_str(out); //o afisam la stdout
    }
    close(fd); //inchidem fisierul
}

void view_treasure(const char *hunt_name, int id) {
    char path[256];
    snprintf(path, sizeof(path), "%s/treasures.dat", hunt_name); //construim calea catre fisierul "treasures.dat"
    int fd = open(path, O_RDONLY); //il descgidem in mod citire
    if (fd < 0) { //posibila eroare
        write_str("Nu s-a putut deschide fișierul.\n");
        return;
    }
    Treasure t;
    while (read(fd, &t, sizeof(t)) == sizeof(t)) { //cat timp citim o comoara
        if (t.treasure_id == id) { //daca id-ul corespunde
            char out[1200];
            snprintf(out, sizeof(out), "ID: %d, User: %s, Lat: %.2f, Long: %.2f, Indiciu: %s, Val: %d\n", t.treasure_id, t.user_name, t.latitude, t.longitude, t.clue, t.value);
            write_str(out); //afisam rezultatul pentru directorul curent
            close(fd); //inchidem fisierul
            return;
        }
    }
    write_str("Comoara nu a fost găsită.\n"); //afisam un mesaj de eroare
    close(fd); //inchidem fisierul
}

void handle_usr1(int sig) {
    char buf[256];
    int fd = open("/tmp/monitor_command", O_RDONLY); //deschidem fisierul temporar pentru a citi comanda
    if (fd < 0) return; //esuarea deschiderii, iesim
    ssize_t n = read(fd, buf, sizeof(buf) - 1); //citeste comanda
    close(fd); //inchidem fisierul
    if (n <= 0) return; //daca citirea esueaza, iesim
    buf[n] = '\0';

    if (strncmp(buf, "list_hunts", 10) == 0) {
        list_hunts();
    } else if (strncmp(buf, "list_treasures ", 15) == 0) {
        list_treasures(buf + 15);
    } else if (strncmp(buf, "view_treasure ", 14) == 0) {
        char name[128];
        int id;
        if (sscanf(buf + 14, "%127s %d", name, &id) == 2)
            view_treasure(name, id);
    } else if (strcmp(buf, "stop") == 0) {
        running = 0;
    } else {
        write_str("Comandă necunoscută.\n");
    }
}

void handle_usr2(int sig) {
    running = 0; //strict pentru a opri monitorul, scrisa sa aiba restul codului mai mult sens :D
}

int main() {
    struct sigaction sa1, sa2; 
    sa1.sa_handler = handle_usr1; //handles pentru semnal SIGUSR1
    sigemptyset(&sa1.sa_mask); //goleste masca de semnale
    sa1.sa_flags = 0; //optiune default
    sigaction(SIGUSR1, &sa1, NULL); //instantiaza handler-ul pentru SIGUSR1

    sa2.sa_handler = handle_usr2; //handles pentru semnal SIGUSR2
    sigemptyset(&sa2.sa_mask); //goleste masca de semnale
    sa2.sa_flags = 0; //optiune default
    sigaction(SIGUSR2, &sa2, NULL); //instantiaza handler-ul pentru SIGUSR2

    write_str("Monitor-ul rulează.\n"); //mesaj de start

    while (running) { //bucla principala, ca un while(1)
        pause(); //asteapta semnale
    }

    write_str("Monitor-ul se închide.\n"); //mesaj de inchidere
    return 0;
}
