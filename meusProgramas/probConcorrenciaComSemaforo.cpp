#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     // fork, execvp
#include <sys/types.h>  // pid_t
#include <sys/wait.h>   // wait
#include <errno.h>      // errno
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>     // intptr_t (para conversão segura de ponteiros)
#include <termios.h>    // Para simular o _getch() no Linux

#define NTHREADS 10
#define ESC 0x1B

int unsigned contador = 0;
int tecla;

sem_t MeuSemaforo;

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

void* TestFunc(void* arg) {
    int i = 0, index = 0, status = 0;

    index = (int)(intptr_t) arg; // parsing

    do {

        status = sem_wait(&MeuSemaforo);

        if (status != 0) {
            printf("Erro na obtenção do semáforo! Código: %x\n", status);
            return 0;
        }

        for (i = 0; i < 100000; i++) {
            contador += 1;
        }
        printf("Thread %d: contador = %u\n", index, contador);
        sleep(0.1);

        status = sem_post(&MeuSemaforo);

        if (status != 0 ) {
            printf("Erro inesperado na liberação do semáfoto! Código: %x\n", status);
            return  0;
        }

    } while (tecla != ESC);

    pthread_exit((void *)(intptr_t) index);
    return ((void *)(intptr_t) index);
}

int main() {
    pthread_t hThreads[NTHREADS];
    int i = 0, status = 0;
    void *tRetStatus;

	// --------------------------------------------------------------------------
	// Criação do semáforo binário
	// --------------------------------------------------------------------------

    status = sem_init(&MeuSemaforo, 0 ,0); // semáforo binário

    if (status != 0) {
        printf("Erro na inicialização do semáfoto! Código: %x\n", status);
        return 0;
    }

    // --------------------------------------------------------------------------
	// Criação das threads secundárias
	// --------------------------------------------------------------------------

    for (i = 0; i < NTHREADS; i++) {
        status = pthread_create(
            &hThreads[i],
            NULL,
            TestFunc,
            (void *)(intptr_t) i
        );

        if (!status) {
            printf("Thread %d criada com ID = %ld\n", i, (unsigned long) hThreads[i]);
        } else {
            printf("Erro ao criar a thread %d! Codigo = %d\n", i, status);
        }
    }

	// --------------------------------------------------------------------------
	// Leitura do teclado
	// --------------------------------------------------------------------------

    do {
        printf("Tecle <ESC> para terminar\n");
        tecla = getch_linux();

    } while (tecla != ESC);

    sem_post(&MeuSemaforo);

	// --------------------------------------------------------------------------
	// Aguarda termino das threads secundarias
	// --------------------------------------------------------------------------

    for (i = 0; i < NTHREADS; i++) {
        printf("Aguardando o término da thread %ld... \n", (unsigned long) &hThreads[i]);
        pthread_join(
            hThreads[i],
            &tRetStatus
        );
        printf("Thread %d: status de retorno %d\n", i, (int)(intptr_t) tRetStatus);
    }

    // --------------------------------------------------------------------------
	// Destruição do semáforo
	// --------------------------------------------------------------------------

    status = sem_destroy(&MeuSemaforo);

    if (status != 0) {
		printf("Erro na destruição do Semaforo! C�digo = %d\n", errno);
		return 0;
	}

    return 0;
}