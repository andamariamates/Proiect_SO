#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>

typedef struct Treasure {
    int treasure_id;
    char user_name[20];
    float latitude;
    float longitude;
    char clue[100];
    int value;
} Treasure;

void log_action(const char *action) {
    int log_fd = open("log.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (log_fd >= 0) {
        write(log_fd, action, strlen(action));
        write(log_fd, "\n", 1);
        close(log_fd);
    }
}

void create_hunt_directory(const char *dir_name) {
    if (mkdir(dir_name, 0755) == -1 && errno != EEXIST) {
        perror("mkdir");
    }
    log_action("create_hunt_directory");
}

void build_filepath(char *buffer, const char *dir_name) {
    snprintf(buffer, 256, "%s/treasures.txt", dir_name);
}

void add(const char *filepath, Treasure *t) {
    int fd = open(filepath, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        perror("open");
        return;
    }

    char buffer[256];
    int len = snprintf(buffer, sizeof(buffer), "%d %s %.2f %.2f %s %d\n", t->treasure_id, t->user_name, t->latitude, t->longitude, t->clue, t->value);
    write(fd, buffer, len);
    close(fd);

    log_action("add");
}

void list(const char *hunt_dir) {
    struct stat st;
    if (stat(hunt_dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
        write(STDOUT_FILENO, "Directorul nu exista!\n", 23);
        return;
    }

    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/treasures.txt", hunt_dir);

    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return;
    }

    char buf[1024];
    ssize_t bytes_read;
    write(STDOUT_FILENO, "Lista comorilor:\n", 18);

    while ((bytes_read = read(fd, buf, sizeof(buf))) > 0) {
        write(STDOUT_FILENO, buf, bytes_read);
    }

    close(fd);
    log_action("list");
}

void view(const char *hunt_dir, int search_id) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/treasures.txt", hunt_dir);

    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return;
    }

    char buf[1024];
    ssize_t bytes_read;
    int found = 0;

    while ((bytes_read = read(fd, buf, sizeof(buf) - 1)) > 0) {
        buf[bytes_read] = '\0';
        char *line = strtok(buf, "\n");
        while (line) {
            int id;
            sscanf(line, "%d", &id);
            if (id == search_id) {
                printf("Comoara gasita: %s\n", line);
                found = 1;
                break;
            }
            line = strtok(NULL, "\n");
        }
        if (found) break;
    }

    if (!found) printf("Comoara cu ID-ul %d nu a fost gasita.\n", search_id);
    close(fd);
    log_action("view");
}

void remove_hunt(const char *hunt_dir) {
    struct stat st;
    if (stat(hunt_dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
        printf("Directorul '%s' nu exista sau nu este valid.\n", hunt_dir);
        return;
    }

    DIR *dir = opendir(hunt_dir);
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    char path[512];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        snprintf(path, sizeof(path), "%s/%s", hunt_dir, entry->d_name);
        remove(path);
    }
    closedir(dir);

    if (rmdir(hunt_dir) == 0) {
        printf("Hunt-ul '%s' a fost sters cu succes!\n", hunt_dir);
    } else {
        perror("rmdir");
    }

    char msg[300];
    snprintf(msg, sizeof(msg), "delete %s", hunt_dir);
    log_action(msg);
}

void remove_treasure(const char *hunt_dir, int delete_id) {
    char filepath[256], temp_filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/treasures.txt", hunt_dir);
    snprintf(temp_filepath, sizeof(temp_filepath), "%s/temp.txt", hunt_dir);

    int fd_read = open(filepath, O_RDONLY);
    if (fd_read < 0) {
        perror("open read");
        return;
    }

    int fd_write = open(temp_filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_write < 0) {
        perror("open write");
        close(fd_read);
        return;
    }

    char buf[1024];
    ssize_t bytes_read;
    char line[256];
    int i = 0, kept = 0;

    while ((bytes_read = read(fd_read, buf, sizeof(buf))) > 0) {
        for (ssize_t j = 0; j < bytes_read; ++j) {
            if (buf[j] == '\n' || i == sizeof(line) - 1) {
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

int main() {
    char hunt_name[20];
    printf("Introduceti numele hunt-ului: ");
    scanf("%s", hunt_name);

    create_hunt_directory(hunt_name);

    char filepath[256];
    build_filepath(filepath, hunt_name);

    int action;
    do {
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