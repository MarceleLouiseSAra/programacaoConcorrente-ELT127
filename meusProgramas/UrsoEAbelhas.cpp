/*********************************************************************************
*
*  AUTOMA��O EM TEMPO REAL - ELT012
*  Prof. Luiz T. S. Mendes - 2019/1
*
*  Atividade em classe - 27/03/2019
*
*  Este programa deve ser completado com as linhas de programa necess�rias
*  para solucionar o "problema do urso e das abelhas" ("The Bear and the Honeybees",
*  G. Andrew, "Multithread, Parallel and Distributed Computing",
*  Addison-Wesley, 2000).
*
* O programa � composto de uma thread prim�ria e 21 threads secund�rias. A thread
* prim�ria cria os objetos de sincroniza��o e as threads secund�rias. As threads
* secund�rias correspondem a um urso e 20 abelhas.
*
* A sinaliza��o de t�rmino de programa � feita atrav�s da tecla ESC. Leituras da
* �ltima tecla digitada devem ser feitas em pontos apropriados para que as threads
* detectem esta tecla.
*
**********************************************************************************/

#define WIN32_LEAN_AND_MEAN 
#define HAVE_STRUCT_TIMESPEC
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     // fork, execvp, usleep, read
#include <sys/types.h>  // pid_t
#include <sys/wait.h>   // wait
#include <errno.h>      // errno
#include <stdint.h>     // intptr_t (para conversão segura de ponteiros)
#include <pthread.h>
#include <semaphore.h>
#include <termios.h>    // Para simular o _getch() no Linux
// #include <conio.h>							//_getch
// #include <windows.h>

#define	ESC				0x1B			// Tecla para encerrar o programa
#define N_ABELHAS		20				// N�mero de abelhas
#define MAX_PORCOES     10              // Capacidade do pote de mel

// #define WHITE    FOREGROUND_RED   | FOREGROUND_GREEN     | FOREGROUND_BLUE
// #define HLGREEN  FOREGROUND_GREEN | FOREGROUND_INTENSITY
// #define HLRED    FOREGROUND_RED   | FOREGROUND_INTENSITY
// #define HLYELLOW FOREGROUND_RED   | FOREGROUND_GREEN     | FOREGROUND_INTENSITY

#define WHITE    "\033[0;37m"
#define HLGREEN  "\033[1;32m"
#define HLRED    "\033[1;31m"
#define HLYELLOW "\033[1;33m"
#define RESET    "\033[0m"

/* Declaracao dos prototipos de funcoes correspondetes aas threads secundarias*/
/* Atencao para o formato conforme o exemplo abaixo! */
void *Thread_Abelha(void *arg);
void *Thread_Urso(void *arg);

/* Declaração dos objetos de sincronização */
pthread_mutexattr_t MutexAttr;  // Atributos de mutex
pthread_mutex_t AcessaPote;		// Mutex para proteger acesso ao pote de mel
sem_t AcordaUrso;				// Semáforo para acordar o urso

int nTecla;						// Variável que armazena a tecla digitada para sair
int nPorcoes = 0;				// Número de porcoes depositadas no pote de mel

// HANDLE hOut;					 //Handle para a saída da console

/*=====================================================================================*/
/* Corpo das funcoes locais Wait(), Signal(), LockMutex e UnLockMutex. Estas funcoes   */
/* assumem que o semaforo [Wait() e Signal()] ou o mutex [LockMutex() e UnLockMutex()] */
/* recebido como parametro já tenha sido previamente criado.                          */
/*=====================================================================================*/

void Wait(sem_t *Semaforo) {
	int status;
	status = sem_wait(Semaforo);
	if (status != 0) {
		printf("Erro na obtencao do semaforo! Codigo = %x\n", errno);
		exit(0);
	}
}

void Signal(sem_t *Semaforo) {
	int status;
	status = sem_post(Semaforo);
	if (status != 0) {
		printf("Erro na liberacao do semaforo! Codigo = %x\n", errno);
		exit(0);
	}
}

void LockMutex(pthread_mutex_t *Mutex) {
	int status;
	status = pthread_mutex_lock(Mutex);
	if (status != 0) {
		printf("Erro na conquista do mutex! Codigo = %d\n", status);
		exit(0);
	}
}

void UnLockMutex(pthread_mutex_t *Mutex) {
	int status;
	status = pthread_mutex_unlock(Mutex);
	if (status != 0) {
		printf("Erro na liberacao do mutex! Codigo = %d\n", status);
		exit(0);
	}
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

/*=====================================================================================*/
/* Thread Primaria                                                                     */
/*=====================================================================================*/

int main(){
	pthread_t hThreads[N_ABELHAS+1];
	void *tRetStatus;
	// int i, status;
	int status;
    intptr_t i;

	// --------------------------------------------------------------------------
    // Obtém um handle para a saída da console
    // --------------------------------------------------------------------------

	// hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	// if (hOut == INVALID_HANDLE_VALUE)
	// 	printf("Erro ao obter handle para a saída da console\n");

	// --------------------------------------------------------------------------
	// Criação dos mutexes
	// --------------------------------------------------------------------------

	pthread_mutexattr_init(&MutexAttr); //sempre retorna 0
	status = pthread_mutexattr_settype(&MutexAttr, PTHREAD_MUTEX_ERRORCHECK);
	if (status != 0) {
		printf("Erro nos atributos do Mutex ! Codigo = %d\n", status);
		exit(0);
	}
	status = pthread_mutex_init(&AcessaPote, &MutexAttr);
	if (status != 0) {
		printf("Erro na criação do mutex AcessaPote! Codigo = %d\n", status);
		exit(0);
	}
	
	// --------------------------------------------------------------------------
	// Criação do semáforo binário
	// --------------------------------------------------------------------------

	status = sem_init(&AcordaUrso, 0, 0); //sempre retorna 0
	// semáforo inicializado com zero: temporariamente bloqueado
	if (status != 0) {
		printf("Erro na inicializacao do semaforo! Codigo = %d\n", errno);
		exit(0);
	}

	// --------------------------------------------------------------------------
	// Criação das threads secundárias
	// --------------------------------------------------------------------------

	for (i = 0; i < N_ABELHAS; i++) {
		status = pthread_create(&hThreads[i], NULL, Thread_Abelha, (void *)i);
		// SetConsoleTextAttribute(hOut, WHITE);

		if (status == 0) {
			printf("%sThread Abelha %ld criada com Id = %lu %s\n", WHITE, i, (unsigned long)hThreads[i], RESET);
		} else {
			printf("Erro na criacao da thread Abelha %ld! Codigo = %d\n", i, status);
			exit(0);
		}
	}// end for

	status = pthread_create(&hThreads[N_ABELHAS], NULL, Thread_Urso, (void *)(intptr_t)-1);
	if (status == 0) {
		printf("%sThread Urso criada com Id = %lu %s\n", WHITE, (unsigned long)hThreads[N_ABELHAS], RESET);
	} else {
		printf("Erro na criacao da thread urso! Codigo = %d\n", status);
		exit(0);
	}

	// --------------------------------------------------------------------------
	// Leitura do teclado
	// --------------------------------------------------------------------------

	printf("\n%sTecle <ESC> para terminar o programa...%s\n\n", HLGREEN, RESET);

	do {
        nTecla = getch_linux();
    } while (nTecla != ESC);

	Signal(&AcordaUrso); // Senão, Thread_Urso fica travada no Wait
	
	// --------------------------------------------------------------------------
	// Aguarda termino das threads secundarias
	// --------------------------------------------------------------------------

	for (i = 0; i < N_ABELHAS + 1; i++) {
		// SetConsoleTextAttribute(hOut, WHITE);
		printf("%sAguardando termino da thread %ld [%ld]...%s\n", WHITE, i, (intptr_t)&hThreads[i], RESET);
		status = pthread_join(hThreads[i], (void **) &tRetStatus);
		if (status != 0) {
			printf("Erro em pthread_join()! Codigo = %d\n", status);
		}
		else {
			if ((intptr_t)tRetStatus != -1) {
				printf("Thread Abelha %ld: status de retorno = %ld\n", i, (intptr_t)tRetStatus);
			} else {
				printf("Thread Urso: status de retorno = %ld\n", (intptr_t)tRetStatus);
			}
		}
	}
	
	// --------------------------------------------------------------------------
	// Elimina os objetos de sincronização criados
	// --------------------------------------------------------------------------

	// SetConsoleTextAttribute(hOut, WHITE);
	status = pthread_mutex_destroy(&AcessaPote);
	if (status != 0) printf("Erro na remocao do mutex! valor = %d\n", status);

	status = sem_destroy(&AcordaUrso);
	if (status != 0) printf("Erro na remocao do semaforo! Valor = %d\n", errno);

	return EXIT_SUCCESS;
	
}//end main

/*=====================================================================================*/
/* Threads secundarias                                                                 */
/*=====================================================================================*/

void *Thread_Abelha(void *arg) {  /* Threads representando as abelhas */

	// int i = (int)arg;
	intptr_t i = (intptr_t)arg;

	do {

		// ACRESCENTE OS COMANDOS DE SINCRONIZACAO VIA SEMAFOROS ONDE NECESSARIO

		// Conquistando o mutex AcessaPote
		LockMutex(&AcessaPote);

		// Deposita uma porcao de mel no pote e incrementa a contagem de porcoes
		if (nPorcoes < MAX_PORCOES) { // Instrução pertencente à seção crítica
			nPorcoes = nPorcoes + 1; // Instrução pertencente à seção crítica
			// SetConsoleTextAttribute(hOut, HLYELLOW);
			printf("%sAbelha %02ld depositou uma porcao. Numero atual de porcoes = %d%s\n", HLYELLOW, i, nPorcoes, RESET);
			if (nPorcoes == MAX_PORCOES) { // Instrução pertencente à seção crítica
				// Pote cheio: acorda o urso
				// Conquistando o semáforo acordaUrso
				Signal(&AcordaUrso); // incrementa o contador do semáforo e a thread Thread_Urso entra na fila de execução
				printf("%sAbelha %02ld encheu o pote: acorda o urso e espera o pote esvaziar-se...%s\n", HLYELLOW, i, RESET);
			}
		}
		// Liberando o mutex AcessaPote
		UnLockMutex(&AcessaPote);

		// Dorme um tempo apenas para facilitar visualização das mensagens
		//Sleep(100);
		usleep(100000);

	} while (nTecla != ESC);

	// Encerramento da thread. Aqui passamos (a título de exemplo) o valor da variável "i"
	// como status de encerramento da thread.
	printf("Thread abelha %ld encerrando execucao...\n", i);
	pthread_exit((void *) i);
	// O comando "return" abaixo é desnecessário, mas presente aqui para compatibilidade
	// com o Visual Studio da Microsoft
	return(0);
}//Thread abelha

void *Thread_Urso(void *arg) {  /* Thread representando o urso */

	// int i = (int)arg;
	intptr_t i = (intptr_t)arg;

	do {

        // ACRESCENTE OS COMANDOS DE SINCRONIZACAO VIA SEMAFOROS ONDE NECESSARIO

		// Aguarda ser acordado pelas abelhas
		Wait(&AcordaUrso);
		// como as abelhas nunca chamam Wait(&AcordaUrso), 
		// Thread_Urso é a única thread na fila de espera do semáforo

		// SetConsoleTextAttribute(hOut, HLRED);
		printf("%sUrso dormindo...%s\n", HLRED, RESET);

		// Conquistando o mutex AcessaPote
		LockMutex(&AcessaPote);

		// Esvazia o pote de mel
		printf("%sUrso acordou!%s\n", HLRED, RESET);
		printf("%sUrso consumiu todo o mel do pote%s\n", HLRED, RESET);
		nPorcoes = 0;

		// Liberando o mutex AcessaPote
		UnLockMutex(&AcessaPote);

	} while (nTecla != ESC);

	// Encerramento da thread. Aqui passamos (a título de exemplo) o valor da variável "i"
	// como status de encerramento da thread.
	printf("Thread urso encerrando execucao...\n");
	pthread_exit((void *) i);
	// O comando "return" abaixo é desnecessário, mas presente aqui para compatibilidade
	// com o Visual Studio da Microsoft
	return(0);
}//Thread urso
