#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     // fork, execvp
#include <sys/types.h>  // pid_t
#include <sys/wait.h>   // wait
#include <sys/select.h>
#include <errno.h>      // errno
#include <pthread.h>
#include <stdint.h>     // intptr_t (para conversão segura de ponteiros)
#include <termios.h>    // Para simular o _getch() no Linux
#include <cstring>
#include <fcntl.h>

#define  ESC 0x1B

int kbhit(void) {
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}

int getch_linux(void) {
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO); // Desativa o modo canônico e o echo do teclado
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

void SignalHandlerUSR1(int sinal) {
    if (sinal == SIGUSR1) {
        printf("SIGNAL HANDLER: Capturado sinal %d [SIGUSR1]\n", sinal);
    } else {
        printf("SIGNAL HANDLER: Capturado sinal %d\n", sinal);
    }
}

void SignalHandlerCTRLC(int sinal) {
    int tempo_restante;
    time_t hora;

    if (sinal == SIGINT) {
        printf("SIGNAL HANDLER: Capturado sinal %d [CTRL-C]\n", sinal);
    } else {
        printf("SIGNAL HANDLER: Capturado sinal %d\n", sinal);
    }

    hora = time(NULL);
    printf("SIGINT signal handler: dormindo por 10 segundos. Hora local = %s", ctime(&hora));
    
    tempo_restante = 10;

    do {
        tempo_restante = sleep(tempo_restante);

        if (tempo_restante != 0) {
            printf ("SIGINT signal handler: sleep() interrompida, %d segundos restantes\n", tempo_restante);
        }

    } while (tempo_restante != 0);
    hora = time(NULL);
    printf("SIGINT signal handler: acordando. Hora local = %s", ctime(&hora));
}

int main() {
    printf("=========================================\n");
    printf("PROCESSO INICIADO! O PID DESTE PROGRAMA É: %d\n", getpid());
    printf("=========================================\n\n");

    struct sigaction sa_kbd;
    struct sigaction sa_usr;
    int status;
    int tecla = 0;

    printf ("Definindo 'signal handler' para SIGINT...\n");
    memset(&sa_kbd, 0, sizeof(sa_kbd)); //IMPORTANTE - SA_KBD PODE CONTER LIXO E ISTO
                                        // PROVOCAR MAU FUNCIONAMENTO

    sa_kbd.sa_handler = SignalHandlerCTRLC;
    sa_kbd.sa_flags = SA_RESTART;
    status = sigaction(SIGINT, &sa_kbd, NULL);

    if (status != 0) {
        printf("Erro em sigaction [1]: valor = %d\n", errno);
        exit (-1);
    }

    printf ("Definindo 'signal handler' para SIGUSR1...\n");
    memset(&sa_usr, 0, sizeof(sa_usr)); 
    sa_usr.sa_handler = SignalHandlerUSR1;
    status = sigaction(SIGUSR1, &sa_usr, NULL);

    if (status != 0) {
        printf("Erro em sigaction [2]: valor = %d\n", errno);
        exit (-1);
    }

    do {
        printf("Digite uma tecla qualquer (ESC para encerrar):\n");

        if (kbhit()) {
            tecla = getch_linux();
        }

        sleep(1);

    } while (tecla != ESC);

    return 0;
}