#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>

pid_t monitor_pid = -1; 
int monitor_stopping = 0; 

void write_str(const char *str) {
    write(0, str, strlen(str));
}

void sigchld_handler(int sig) { 
    int status;
    pid_t pid = waitpid(monitor_pid, &status, WNOHANG);
    if (pid > 0) {
        monitor_pid = -1;
        monitor_stopping = 0;
        write_str("Monitor-ul a fost oprit complet.\n");
    }
}

void start_monitor() { 
    if (monitor_pid != -1) {
        write_str("Monitor-ul este deja pornit.\n");
        return;
    }

    pid_t pid = fork();
    if (pid == 0) {
        perror("Eroare.");
        exit(1);
    } else if (pid > 0) { 
        monitor_pid = pid;
        usleep(200000);
        write_str("Monitor-ul a fost pornit.\n");
    } else {
        write_str("Eroare la fork.\n");
    }
}

void send_command(const char *cmd) {
    if (monitor_pid == -1) {
        write_str("Monitor-ul nu este activ.\n");
        return;
    }

    if (monitor_stopping) {
        write_str("Monitor-ul este în curs de oprire. Așteptați...\n");
        return;
    }

    int fd = open("/tmp/monitor_command", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return;
    write(fd, cmd, strlen(cmd));
    close(fd);
    kill(monitor_pid, SIGUSR1);
}

void stop_monitor() {
    if (monitor_pid == -1) {
        write_str("Monitor-ul nu este activ.\n");
        return;
    }

    monitor_stopping = 1; 
    send_command("stop");
    kill(monitor_pid, SIGUSR2);
    usleep(300000);
    write_str("Comanda de oprire a fost trimisă.\n");
}

void exit_program() { 
    if (monitor_pid != -1) {
        write_str("Monitor-ul încă rulează. Folosiți mai întâi comanda stop_monitor.\n");
        return;
    }
    _exit(0);
}

int main() {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);

    char buf[256];
    while (1) {
        usleep(200000);
        write_str("treasure_hub> ");
        ssize_t len = read(0, buf, sizeof(buf) - 1);
        if (len <= 0) continue;
        buf[len] = '\0';
        if (buf[len - 1] == '\n') buf[len - 1] = '\0';

        if (strcmp(buf, "start_monitor") == 0) {
            start_monitor();
        } else if (strcmp(buf, "stop_monitor") == 0) {
            stop_monitor();
        } else if (strcmp(buf, "list_hunts") == 0) {
            send_command("list_hunts");
        } else if (strncmp(buf, "list_treasures ", 15) == 0) {
            send_command(buf);
        } else if (strncmp(buf, "view_treasure ", 14) == 0) {
            send_command(buf);
        } else if (strcmp(buf, "exit") == 0) {
            exit_program();
        } else {
            write_str("Comandă necunoscută.\n");
        }
    }
    return 0;
}
