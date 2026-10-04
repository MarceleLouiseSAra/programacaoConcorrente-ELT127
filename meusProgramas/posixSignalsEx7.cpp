#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
// #include "conio.h"   // kbhit() e getch_linux()
#include <errno.h>      // errno
#include <termios.h>    // Para simular o getch_linux() no Linux
#include <cstring>

#define SP	0x20
#define	ESC	0x1B
#define NUM_THREADS	2

int kbhit(void) {
    struct termios oldt, newt;
    int bytesWaiting;

    // Obtém as configurações atuais do terminal (STDIN_FILENO = 0)
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;

    // Desativa o modo canônico (não espera o ENTER) e desativa o ECHO do teclado
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    // Configura a leitura para não ser bloqueante
    newt.c_cc[VMIN] = 0;
    newt.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    // Verifica quantos bytes estão esperando no buffer de entrada do teclado
    struct timeval tv = {0, 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    
    // select consulta o descritor STDIN imediatamente (tempo limite 0)
    select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
    bytesWaiting = FD_ISSET(STDIN_FILENO, &fds);

    // Restaura as configurações originais do terminal
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    return bytesWaiting;
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

void *WaitSignalFunc(void *arg) {

    int signum;
    int status;
    int id;
    sigset_t thread_sigset;

    //id = (int) arg;
    id = (int) (intptr_t) arg;

    status = pthread_sigmask(-1, NULL, &thread_sigset);
    if (status != 0) {
		printf("WAITSIGNALFUNC- Erro em pthread_sigmask: i=%d erro = %d\n",
		       id, status);
		exit(-1);
    }

    do {

        status  = sigwait(&thread_sigset, &signum);
        if (status == -1) {
            printf("Erro em sigwait()! id = %d erro = %d\n", id, errno);
            exit(-1);
        }
        // Testa o sinal recebido e age de acordo
        if (signum == SIGUSR1) //Tecla de espa�o digitada
            printf("Thread %d: Sinal SIGUSR1 recebido\n", id);
        else if (signum == SIGUSR2) {
            printf("Thread %d: Sinal SIGUSR2 recebido... encerrando\n", id);
            break;	// Abandona la�o e encerra
        }
        else {
            printf("Thread %d: Sinal inesperado %d recebido... encerrando\n", id, signum);
            break;	// Abandona la�o e encerra
        }

    } while(signum == SIGUSR1);

    printf("Thread WaitSignalFunc %d terminando...\n", id);
    pthread_exit((void *)(intptr_t)id);
}

int main() {

    pthread_t hThread[NUM_THREADS];
    sigset_t sigset;            
    struct sigaction sa_kbd;    
                                
    void *thread_ret;           
    int status;                 
    // int signum;                 
    int i, nTecla, vez_thread;  

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGUSR1);
    sigaddset(&sigset, SIGUSR2);

    // EXPERIMENTO REMOTO I: Com a aplicação em execução e antes de digitar qualquer
    // coisa, verifique o efeito de CTRL-C no funcionamento da aplicação. Em seguida,
    // adicione o sinal SIGINT (CTRL-C) à máscara de sinais, recompile a aplicação e
    // teste novamente o efeito de CTRL-C, descrevendo os efeitos observados.
    // ============ DESBLOQUEAR A LINHA ABAIXO ============
    // sigaddset(&sigset, SIGINT);

    status = pthread_sigmask(SIG_BLOCK, &sigset, NULL);
    if (status != 0) {
        printf("Erro em pthread_sigmask: %d\n", status);
        exit(-1);
    }

    for (i = 0; i < NUM_THREADS; i++) {
        status = pthread_create(&hThread[i], NULL, WaitSignalFunc, (void *) (intptr_t) i);
        if (status == 0) printf("Thread WaitSignalFunc #%d criada com Id= %#lx \n", i,
                                (long unsigned) hThread[i]);
        else {
            printf ("Erro na criacao da thread WaitSignalFunc #%d! Codigo = %d\n", i, status);
            exit(-1);
        }
    }

    sleep(1);

    // EXPERIMENTO REMOTO II: Cancele o experimento I acima, se este estiver em vigor,
    // e ajuste novamente a máscara de sinais desta thread primária de forma a bloquear
    // o sinal SIGINT. Recompile a aplicação, digite CTRC-C e explique porque o
    // comportamento é diferente do EXPERIMENTO I, visto que o sinal SIGINT também
    // foi bloqueado aqui.
    // ============ DESBLOQUEAR AS LINHAS ABAIXO ============
    // sigaddset(&sigset, SIGINT);
    // status = pthread_sigmask(SIG_BLOCK, &sigset, NULL);
    // if (status != 0) {
    	// printf("Erro em pthread_sigmask: %d\n", status);
    	// exit(-1);
    // }

    // EXPERIMENTO REMOTO III: Cancele os experimentos I e II acima, se estiverem em vigor,
    // e defina a disposição do sinal SIGINT para SIG_IGN nesta thread primária. Recompile
    // e execute novamente a aplicação e, sem seguida, diite novamente CTRL-C. Verifique
    // o efeito observado e explique porque o mesmo ocorre, visto que as threads secundárias
    // continuam com SIGINT desbloqueado.
    // ============ DESBLOQUEAR AS LINHAS ABAIXO ============
    memset(&sa_kbd, 0, sizeof(sa_kbd)); // IMPORTANTE - SA_KBD PODE CONTER LIXO E ISTO
                                        // PROVOCAR MAU FUNCIONAMENTO
    sa_kbd.sa_handler = SIG_IGN;
    status = sigaction(SIGINT, &sa_kbd, NULL);
    if (status != 0) {
    	printf("Erro em sigaction: valor = %d\n", errno);
    	exit (-1);
    }

    do {
        printf("Tecle <SP> para enviar sinal ou <Esc> para terminar\n");
        nTecla = getch_linux();
        if (nTecla == SP) {
                status = pthread_kill(hThread[vez_thread], SIGUSR1);     // Gera sinal
            if (status != 0) {
                if (status == 3)
                printf("Erro em pthread_kill: thread tid = %#lx inexistente ou ja' encerrada\n",
                    (long unsigned)hThread[i]);
            else
                printf("Erro em pthread_kill: i=%d tid = %#lx erro = %d\n", i,
                    (long unsigned)hThread[i], status);
            exit(-1);
            }
        vez_thread = 1 - vez_thread;
        }
        else if (nTecla == ESC) {
            for (i = 0; i < NUM_THREADS; i++) {
            status = pthread_kill(hThread[i], SIGUSR2);     // Gera sinal
            if (status != 0) {
                if (status == 3)
                printf("Erro em pthread_kill: thread tid = %#lx inexistente ou ja' encerrada\n",
                    (long unsigned)hThread[i]);
                else
                printf("Erro em pthread_kill: i=%d tid = %#lx erro = %d\n", i,
                    (long unsigned)hThread[i], status);
                exit(-1);
            }
            }
	    }
    } while (nTecla != ESC);

    for (i = 0; i < NUM_THREADS; i++){
        status = pthread_join(hThread[i], &thread_ret);
        if (status == 0) {
            printf ("Thread %#lx: status de retorno = %d\n", (long unsigned) hThread[i], (int) (intptr_t) thread_ret);
        } else {
            printf ("Erro em pthread_join! i = %d status = %d\n", i, status);
        }
    }

    return EXIT_SUCCESS;

}