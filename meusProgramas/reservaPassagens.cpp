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

void* AlocFunc(void* arg) {
	int assento = 0, valorRetornado = 0;

	assento = (intptr_t) arg;

	if (MapaOcupacao[(intptr_t)assento] == LIVRE) {
		// SwitchToThread();
		sleep(0.1);
		
		MapaOcupacao[(intptr_t)assento] = OCUPADO;
		valorRetornado = assento;
	} else {
		valorRetornado = (intptr_t) NULL;
	}

	pthread_exit((void *)(intptr_t) valorRetornado);
	return (void *)(intptr_t) NULL;
}

int main() {

	pthread_t hThreads[NUM_THREADS];
	int assento = 0, status = 0, tRetStatus[NUM_THREADS];

	// --------------------------------------------------------------------------
	// Criação do objeto de sincronização
	// --------------------------------------------------------------------------

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

    return 0;
}