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
#include <semaphore.h>

#define  ESC 0x1B

sem_t semaforo;

void SignalHandlerUSR1(int sinal) {

    int status = 0;
    if (sinal == SIGUSR1) {
        printf("SIGNAL HANDLER: Capturado sinal %d [SIGUSR1]\n", sinal);

        status = sem_post(&semaforo);

        if (status != 0) {
            printf("Erro em sem_post: valor = %d\n", errno);
	        exit(-1);
        }

    } else {
        printf("SIGNAL HANDLER: Capturado sinal %d\n", sinal);
    }
}

void SignalHandlerCTRLC(int sinal) {
    // int tempo_restante;
    // time_t hora;

    if (sinal == SIGINT) {
        printf("SIGNAL HANDLER: Capturado sinal %d [CTRL-C]\n", sinal);
    } else {
        printf("SIGNAL HANDLER: Capturado sinal %d\n", sinal);
    }

    // hora = time(NULL);
    // printf("SIGINT signal handler: dormindo por 10 segundos. Hora local = %s", ctime(&hora));
    
    // tempo_restante = 10;

    // do {
    //     tempo_restante = sleep(tempo_restante);

    //     if (tempo_restante != 0) {
    //         printf ("SIGINT signal handler: sleep() interrompida, %d segundos restantes\n", tempo_restante);
    //     }

    // } while (tempo_restante != 0);
    // hora = time(NULL);
    // printf("SIGINT signal handler: acordando. Hora local = %s", ctime(&hora));
}

int main() {
    printf("=========================================\n");
    printf("PROCESSO INICIADO! O PID DESTE PROGRAMA É: %d\n", getpid());
    printf("=========================================\n\n");

    int status;
    // int tecla = 0;
    // contém as configurações de como o processo deve tratar determinado sinal
    struct sigaction sa_kbd;
    struct sigaction sa_usr;

    status = sem_init(&semaforo, 0 ,0);

    if (status != 0) {
        printf("Erro em sem_init: valor = %d\n", errno);
        exit(-1);
    }

    printf ("Definindo 'signal handler' para SIGINT...\n");
    memset(&sa_kbd, 0, sizeof(sa_kbd)); //IMPORTANTE - SA_KBD PODE CONTER LIXO E ISTO
                                        // PROVOCAR MAU FUNCIONAMENTO

    sa_kbd.sa_handler = SignalHandlerCTRLC; // Quando chegar um SIGINT, execute a função SignalHandlerCTRLC
    // **************** LINHA A DESBLOQUEAR NA ATIVIDADE PRÁTICA II ***************
    sa_kbd.sa_flags = SA_RESTART; // opção de comportamento do signal handler
    // **************** LINHA A DESBLOQUEAR NA ATIVIDADE PRÁTICA II ***************
    // Se a chegada do sinal interromper uma chamada de sistema que pode ser 
    // reiniciada, o sistema tenta reiniciar essa chamada automaticamente 
    // depois que o signal handler terminar.
    status = sigaction(SIGINT, &sa_kbd, NULL);
    if (status != 0) {
        printf("Erro em sigaction [1]: valor = %d\n", errno);
        exit (-1);
    }

    printf ("Definindo 'signal handler' para SIGUSR1...\n");
    memset(&sa_usr, 0, sizeof(sa_usr)); 
    sa_usr.sa_handler = SignalHandlerUSR1;
    // **************** LINHA A DESBLOQUEAR NA ATIVIDADE PRÁTICA II ***************
    sa_usr.sa_flags = SA_RESTART;
    // **************** LINHA A DESBLOQUEAR NA ATIVIDADE PRÁTICA II ***************
    
    status = sigaction(SIGUSR1, &sa_usr, NULL);
    if (status != 0) {
        printf("Erro em sigaction [2]: valor = %d\n", errno);
        exit (-1);
    }

    // do {
    //     printf("Digite uma tecla qualquer (ESC para encerrar):\n");

    //     if (kbhit()) {
    //         tecla = getch_linux();
    //     }

    //     sleep(1);

    // } while (tecla != ESC);

    printf("Thread primária: aguardando semáforo... \n");
    
    status = sem_wait(&semaforo); // chamada bloqueante
    // Depois que o handler termia, a chamada bloqueante 
    // pode ser reiniciada por causa do SA_RESTART.
    if (status == 0) {
        printf("Thread primaria: passou pelo semáforo.\n");
    } else if (status == -1 && errno == EINTR) {
        printf("Thread primaria: chamada a sem_wait interrompida!\n");
    } else {
        printf("Thread primaria: erro desconhecido na chamada a sem_wait = %d\n", errno);
    }

    return 0;
}