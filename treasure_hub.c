#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <dirent.h> 

pid_t monitor_pid = -1; 
int monitor_stopping = 0;
int pipe_fd[2]; // pipe_fd[0] = capăt de citire, pipe_fd[1] = capăt de scriere

void write_str(const char *str) 
{ //facilitarea scrierii unui mesaj la stdout
    write(STDOUT_FILENO, str, strlen(str));
}

void sigchld_handler(int sig) 
{
    int status;
    pid_t pid = waitpid(monitor_pid, &status, WNOHANG); //asteapta terminarea procesului copil (aici, monitor)
    if (pid > 0) 
    { //daca procesul copil s-a terminat
        monitor_pid = -1; //reseteaza PID-ul monitorului
        monitor_stopping = 0; //reseteaza flag-ul de oprire
        write_str("Monitor-ul a fost oprit complet.\n"); //afiseaza mesajul de confirmare
    }
}

void start_monitor() 
{ // pornește procesul monitor și configurează pipe-ul
    if (monitor_pid != -1) 
    { //verifica daca monitorul este deja pornit
        write_str("Monitor-ul este deja pornit.\n");
        return;
    }

    if (pipe(pipe_fd) == -1) 
    { // creeaza pipe-ul
        perror("pipe");
        return;
    }

    pid_t pid = fork(); //creeaza un proces copil, aici monitorul
    if (pid == 0) 
    {  // in copil – monitor
        close(pipe_fd[0]); // inchidem capatul de citire

        char fd_str[10]; 
        snprintf(fd_str, sizeof(fd_str), "%d", pipe_fd[1]); // convertim descriptorul numeric in string

        execl("./monitor", "monitor", fd_str, NULL); // lansăm monitorul cu pipe_fd[1]
        write_str("Eroare la pornirea monitor-ului.\n");
        _exit(1);
    } else if (pid > 0) { // in parinte – treasure_hub
        monitor_pid = pid; //salveaza PID-ul monitorului
        close(pipe_fd[1]); // inchidem capatul de scriere (folosit de monitor)
        write_str("Monitor-ul a fost pornit.\n");
    } else {
        write_str("Eroare la fork.\n");
    }
}

void send_command(const char *cmd) 
{
    if (monitor_pid == -1) 
    { //verifica daca monitorul nu este pornit
        write_str("Monitor-ul nu este activ.\n"); //afiseaza un mesaj
        return;
    }

    if (monitor_stopping) 
    { //verifica daca monitul este in curs de oprire
        write_str("Monitorul este în curs de oprire. Asteptati...\n"); //aviseaza un mesaj
        return;
    }

    int fd = open("/tmp/monitor_command", O_WRONLY | O_CREAT | O_TRUNC, 0644); //deschide un fisier temporar pentru a scrie comanda
    if (fd < 0) return; //deschiderea a esuat, iesire
    write(fd, cmd, strlen(cmd)); //altfel, scrie comanda in fisier
    close(fd); //inchide fisierul
    kill(monitor_pid, SIGUSR1); //trimite semnalul SIGUSR1 monitorului pentru a procesa comanda

    char buf[1024];
    ssize_t n;
    usleep(100000); // asteptam putin sa se scrie
    while ((n = read(pipe_fd[0], buf, sizeof(buf) - 1)) > 0) 
    { // citim rezultatul de la monitor din pipe
        buf[n] = '\0';
        write_str(buf);
        if (n < sizeof(buf) - 1) break;
        usleep(100000);
    }
}

void stop_monitor() 
{
    if (monitor_pid == -1) 
    { //verfica daca monitorul este oprit
        write_str("Monitorul nu este activ.\n");
        return;
    }

    monitor_stopping = 1; //flag de oprire
    send_command("stop"); //trimite comanda stop catre monitor
    kill(monitor_pid, SIGUSR2); //trimite semnalul SIGUSR2 monitorului pentru a-l opri
    usleep(300000); //sincronizare
    write_str("Comanda de oprire a fost trimisă.\n"); //afiseaza un mesaj de confirmare
}

void exit_program() 
{ 
    if (monitor_pid != -1) 
    { //verifica daca monitorul este inca pornit
        write_str("Monitorul încă rulează. Folosiți mai întâi comanda stop_monitor.\n");
        return;
    }
    _exit(0); //termina programul
}

void calculate_score() 
{
    DIR *dir = opendir("."); // deschidem directorul curent
    if (!dir) 
    {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) 
    {
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        if (entry->d_type == DT_DIR) 
        {
            int pfd[2];
            if (pipe(pfd) < 0) 
            {
                perror("pipe");
                continue;
            }

            pid_t pid = fork();
            if (pid < 0) 
            {
                perror("fork");
                continue;
            }

            if (pid == 0) 
            { // proces copil – redirecteaza stdout in pipe si executa score_calculator
                close(pfd[0]); // inchide citirea
                dup2(pfd[1], STDOUT_FILENO); // stdout -> pipe
                close(pfd[1]); // inchide vechiul descriptor (dup2 a duplicat)

                execlp("./score_calculator", "score_calculator", entry->d_name, NULL);
                perror("execlp"); // daca a esuat
                _exit(1);
            } else { // proces parinte – citeste rezultatul
                close(pfd[1]); // inchide scrierea
                char buf[512];
                ssize_t n;

                write_str("\nScoruri pentru ");
                write_str(entry->d_name);
                write_str(":\n");

                while ((n = read(pfd[0], buf, sizeof(buf) - 1)) > 0) 
                {
                    buf[n] = '\0';
                    write_str(buf); // afisam continutul primit
                }

                close(pfd[0]); // inchidem citirea
                wait(NULL); // asteptam copilul
            }
        }
    }

    closedir(dir);
}

int main() 
{
    struct sigaction sa;
    sa.sa_handler = sigchld_handler; //setare handles pentru SIGCHLD
    sigemptyset(&sa.sa_mask); //initializare masca de semnale
    sa.sa_flags = SA_NOCLDSTOP; //ignora semnalele SIGCHLD pentru procesle copil oprite temporar
    sigaction(SIGCHLD, &sa, NULL); //dad drumul la ghandler SIGCHLD

    char buf[256];
    while (1) 
    { //bucla principala
        usleep(200000); //sincronizarea, foarte importanta, altfel afiseaza urat
        write_str("treasure_hub> "); //mic ghid pentru utilizator sa stie cand poate sa scrie o noua comanda
        ssize_t len = read(0, buf, sizeof(buf) - 1); //citeste comanda de la stdin
        if (len <= 0) continue;
        buf[len] = '\0';
        if (buf[len - 1] == '\n') buf[len - 1] = '\0';

        if (strcmp(buf, "start_monitor") == 0) 
        {
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
        } else if (strcmp(buf, "calculate_score") == 0) {
            calculate_score();
        } else {
            write_str("Comandă necunoscută.\n");
        }
    }
    return 0;
} 