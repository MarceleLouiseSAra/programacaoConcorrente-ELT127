#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     // fork, execvp
#include <sys/types.h>  // pid_t
#include <sys/wait.h>   // wait
#include <errno.h>      // errno
#include <pthread.h>
#include <stdint.h>     // intptr_t (para conversão segura de ponteiros)
#include <termios.h>    // Para simular o _getch() no Linux

#define NTHREADS 10
#define ESC 0x1B

int unsigned contador = 0;
int tecla;

pthread_mutex_t MeuMutex;
pthread_mutexattr_t MeuMutexAttr;

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

        // conquista do mutex para adentrar a seção crítica
        status = pthread_mutex_lock(&MeuMutex);

        if (status != 0) {
            if (status == EDEADLK) {
                printf("Erro EDEADLK na conquista do Mutex!\n");
            } else {
                printf("Erro inesperado na conquista do Mutex! Codigo: %x\n", status);
            }
            return 0;
        }

        for (i = 0; i < 100000; i++) {
            contador += 1;
        }
        printf("Thread %d: contador = %u\n", index, contador);
        sleep(0.1);

        // liberação do mutex ao sair da seção crítica
        status = pthread_mutex_unlock(&MeuMutex);

        if (status != 0) {
            if (status == EPERM) {
                printf("Erro: tentativa de liberar mutex nao-conquistado!\n");
            } else {
                printf("Erro inesperado na liberação do Mutex! Código: %x\n", status);
            }
            return 0;
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
	// Criação dos mutexes
	// --------------------------------------------------------------------------

    pthread_mutexattr_init(&MeuMutexAttr);
    status = pthread_mutexattr_settype(&MeuMutexAttr, PTHREAD_MUTEX_NORMAL);

    if (status != 0) {
		printf("Erro nos atributos do Mutex! Código = %d\n", status);
		return 0;
	}

	status = pthread_mutex_init(&MeuMutex, &MeuMutexAttr);

	if (status !=0){
		printf ("Erro na criação do Mutex! Código = %d\n", status);
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
	// Destruição do mutex
	// --------------------------------------------------------------------------

    pthread_mutex_destroy(&MeuMutex);

	if (status != 0) {
		printf ("Erro na destruição do Mutex! Código = %d\n", status);
		return 0;
	}

    return 0;
}