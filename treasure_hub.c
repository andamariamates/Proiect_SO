#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>

pid_t monitor_pid = -1; //pornit
int monitor_stopping = 0; //oprit

void write_str(const char *str) { //facilitarea scrierii unui mesaj la stdout
    write(0, str, strlen(str));
}

void sigchld_handler(int sig) { 
    int status;
    pid_t pid = waitpid(monitor_pid, &status, WNOHANG); //asteapta terminarea procesului copil (aici, monitor)
    if (pid > 0) { //daca procesul copil s-a terminat
        monitor_pid = -1; //reseteaza PID-ul monitorului
        monitor_stopping = 0; //reseteaza flag-ul de oprire
        write_str("Monitor-ul a fost oprit complet.\n"); //afiseaza mesajul de confirmare
    }
}

void start_monitor() { 
    if (monitor_pid != -1) { //verifica daca monitorul este deja pornit
        write_str("Monitor-ul este deja pornit.\n");
        return;
    }

    pid_t pid = fork(); //creeaza un proces copil, aici monitorul
    if (pid == 0) { //daca este proces copil
        execl("./monitor", "monitor", NULL); //inlocuieste procesul curent cu executabilul "monitor"
        write_str("Eroare la pornirea monitor-ului.\n"); //daca execll esueaza
        _exit(1);  //termina procesul copil
    } else if (pid > 0) { //daca este proces parinte
        monitor_pid = pid; //salveaza PID-ul monitorului
        usleep(200000); //intarzie putin pentru a permite monitorului sa se sincronizeze
        write_str("Monitor-ul a fost pornit.\n"); //afiseaza un mesaj de confirmare
    } else { //daca fork esueaza
        write_str("Eroare la fork.\n"); //afiseaza un mesaj de eroare
    }
}

void send_command(const char *cmd) {
    if (monitor_pid == -1) { //verifica daca monitorul nu este pornit
        write_str("Monitor-ul nu este activ.\n"); //afiseaza un mesaj
        return;
    }

    if (monitor_stopping) { //verifica daca monitul este in curs de oprire
        write_str("Monitorul este în curs de oprire. Așteptați...\n"); //aviseaza un mesaj
        return;
    }

    int fd = open("/tmp/monitor_command", O_WRONLY | O_CREAT | O_TRUNC, 0644); //deschide un fisier temporar pentru a scrie comanda
    if (fd < 0) return; //deschiderea a esuat, iesire
    write(fd, cmd, strlen(cmd)); //altfel, scrie comanda in fisier
    close(fd); //inchide fisierul
    kill(monitor_pid, SIGUSR1); //trimite semnalul SIGUSR1 monitorului pentru a procesa comanda
}

void stop_monitor() {
    if (monitor_pid == -1) { //verfica daca monitorul este oprit
        write_str("Monitorul nu este activ.\n");
        return;
    }

    monitor_stopping = 1; //flag de oprire
    send_command("stop"); //trimite comanda stop catre monitor
    kill(monitor_pid, SIGUSR2); //trimite semnalul SIGUSR2 monitorului pentru a-l opri
    usleep(300000); //sincronizare
    write_str("Comanda de oprire a fost trimisă.\n"); //afiseaza un mesaj de confirmare
}

void exit_program() { 
    if (monitor_pid != -1) { //verifica daca monitorul este
        write_str("Monitorul încă rulează. Folosiți mai întâi comanda stop_monitor.\n");
        return;
    }
    _exit(0); //termina programul
}

int main() {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler; //setare handles pentru SIGCHLD
    sigemptyset(&sa.sa_mask); //initializare masca de semnale
    sa.sa_flags = SA_NOCLDSTOP; //ignora semnalele SIGCHLD pentru procesle copil oprite temporar
    sigaction(SIGCHLD, &sa, NULL); //dad drumul la ghandler SIGCHLD

    char buf[256];
    while (1) { //bucla principala
        usleep(200000); //sincronizare, foarte importanta, altfel arata urat
        write_str("treasure_hub> "); //mic ghid pentru utilizator sa stie cand poate sa scrie o noua comanda
        ssize_t len = read(0, buf, sizeof(buf) - 1); //citeste comanda de la stdin
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
