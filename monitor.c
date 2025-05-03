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

volatile sig_atomic_t running = 1;

void write_str(const char *s) {
    write(STDOUT_FILENO, s, strlen(s));
}

void list_hunts() {
    DIR *dir = opendir(".");
    if (!dir) return;
    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (entry->d_type == DT_DIR && strncmp(entry->d_name, "Hunt", 4) == 0) {
            char path[256];
            snprintf(path, sizeof(path), "%s/treasures.dat", entry->d_name);
            int fd = open(path, O_RDONLY);
            if (fd < 0) continue;
            int count = 0;
            Treasure t;
            while (read(fd, &t, sizeof(t)) == sizeof(t)) count++;
            close(fd);
            char out[256];
            snprintf(out, sizeof(out), "%s - %d comori\n", entry->d_name, count);
            write_str(out);
        }
    }
    closedir(dir);
}

void list_treasures(const char *hunt_name) {
    char path[256];
    snprintf(path, sizeof(path), "%s/treasures.dat", hunt_name);
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        write_str("Nu s-a putut deschide fișierul.\n");
        return;
    }
    Treasure t;
    while (read(fd, &t, sizeof(t)) == sizeof(t)) {
        char out[1200];
        snprintf(out, sizeof(out), "ID: %d, User: %s, Lat: %.2f, Long: %.2f, Indiciu: %s, Val: %d\n", t.treasure_id, t.user_name, t.latitude, t.longitude, t.clue, t.value);
        write_str(out);
    }
    close(fd);
}

void view_treasure(const char *hunt_name, int id) {
    char path[256];
    snprintf(path, sizeof(path), "%s/treasures.dat", hunt_name);
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        write_str("Nu s-a putut deschide fișierul.\n");
        return;
    }
    Treasure t;
    while (read(fd, &t, sizeof(t)) == sizeof(t)) {
        if (t.treasure_id == id) {
            char out[1200];
            snprintf(out, sizeof(out), "ID: %d, User: %s, Lat: %.2f, Long: %.2f, Indiciu: %s, Val: %d\n", t.treasure_id, t.user_name, t.latitude, t.longitude, t.clue, t.value);
            write_str(out);
            close(fd);
            return;
        }
    }
    write_str("Comoara nu a fost găsită.\n");
    close(fd);
}

void handle_usr1(int sig) {
    char buf[256];
    int fd = open("/tmp/monitor_command", O_RDONLY);
    if (fd < 0) return;
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0) return;
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
    running = 0;
}

int main() {
    struct sigaction sa1, sa2;
    sa1.sa_handler = handle_usr1;
    sigemptyset(&sa1.sa_mask);
    sa1.sa_flags = 0;
    sigaction(SIGUSR1, &sa1, NULL);

    sa2.sa_handler = handle_usr2;
    sigemptyset(&sa2.sa_mask);
    sa2.sa_flags = 0;
    sigaction(SIGUSR2, &sa2, NULL);

    write_str("Monitor-ul rulează.\n");

    while (running) {
        pause();
    }

    write_str("Monitor-ul se închide.\n");
    return 0;
}
