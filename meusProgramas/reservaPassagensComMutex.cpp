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
#include <time.h>

#define NUM_THREADS	20	// Vamos disparar 20 threads concorrentes

#define LIVRE 0
#define OCUPADO 1
int MapaOcupacao[6] = {LIVRE, LIVRE, LIVRE, LIVRE, LIVRE, LIVRE};

#define FALHA -1
#define SUCESSO 1

pthread_mutex_t MeuMutex;
pthread_mutexattr_t MeuMutexAttr;

void LockMutex(pthread_mutex_t *Mutex) {
	int status = 0;
	status = pthread_mutex_lock(Mutex);

	if (status != 0) {
		printf("Erro na conquista do mutex! Código - %d\n", status);
		exit(0);
	}
}

void UnlockMutex(pthread_mutex_t *Mutex) {
	int status = 0;
	status = pthread_mutex_unlock(Mutex);

	if (status != 0) {
		printf("Erro na liberação do mutex! Código = %d\n", status);
		exit(0);
	}
}

void* AlocFunc(void* arg) {
	int assento = 0, valorRetornado = 0;

	assento = (intptr_t) arg;

	LockMutex(&MeuMutex);

	if (MapaOcupacao[(intptr_t)assento] == LIVRE) {
		// SwitchToThread();
		sleep(0.1);
		
		MapaOcupacao[(intptr_t)assento] = OCUPADO;
		valorRetornado = assento;
	} else {
		valorRetornado = (intptr_t) NULL;
	}

	UnlockMutex(&MeuMutex);

	pthread_exit((void *)(intptr_t) valorRetornado);
	return (void *)(intptr_t) NULL;
}

int main() {

	pthread_t hThreads[NUM_THREADS];
	int assento = 0, status = 0, tRetStatus[NUM_THREADS];

	// --------------------------------------------------------------------------
	// Criação do objeto de sincronização
	// --------------------------------------------------------------------------

	pthread_mutexattr_init(&MeuMutexAttr);
	status = pthread_mutexattr_settype(&MeuMutexAttr, PTHREAD_MUTEX_ERRORCHECK);

	if (status != 0) {
		printf("Erro nos atributos do Mutex ! Codigo = %d\n", status);
		exit(0);
	}

	status = pthread_mutex_init(&MeuMutex, &MeuMutexAttr);

	if (status != 0) {
		printf("Erro na criação do mutex AcessaPote! Codigo = %d\n", status);
		exit(0);
	}

    // --------------------------------------------------------------------------
	// Criação das threads secundárias
	// --------------------------------------------------------------------------

	srand((unsigned)time(NULL));

	for (int i = 0; i < NUM_THREADS; i++) {
		assento = rand() % 6;
		status = pthread_create(
			&hThreads[i],
			NULL,
			AlocFunc,
			(void *)(intptr_t) assento
		);

		if (!status) {
			printf("Thread %d criada com assento %d ID = %ld\n", i, assento, (unsigned long) &hThreads[i]);
		} else {
			printf("Erro ao criar a thread %d! Codigo = %d\n", i, status);
		}
	}

	// --------------------------------------------------------------------------
	// Leitura do teclado
	// --------------------------------------------------------------------------

	// --------------------------------------------------------------------------
	// Aguarda termino das threads secundarias
	// --------------------------------------------------------------------------

	for (int i = 0; i < NUM_THREADS; i++) {
		printf("Aguardando o término da thread %ld... Código de saída: ", (unsigned long) &hThreads[i]);
		pthread_join(
			hThreads[i],
			(void **) &tRetStatus[i]
		);

		if (tRetStatus[i] == -1) {
			printf("Falha\n");
		} else {
			printf("Assento %d\n", tRetStatus[i]);
		}
	}

    // --------------------------------------------------------------------------
	// Destruição do objeto de sincronização
	// --------------------------------------------------------------------------

	status = pthread_mutex_destroy(&MeuMutex);

	if (status != 0) {
		printf("Erro na destruição do mutex! valor = %d\n", status);
	}

    return 0;
}