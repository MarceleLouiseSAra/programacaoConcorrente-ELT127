#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     // fork, execvp
#include <sys/types.h>  // pid_t
#include <sys/wait.h>   // wait
#include <errno.h>      // errno
#include <pthread.h>
#include <stdint.h>     // intptr_t (para conversão segura de ponteiros)
#include <termios.h>    // Para simular o _getch() no Linux

#define HAVE_STRUCT_TIMESPEC
#define NTHREADS	5
#define	ESC			0x1B
#define TeclaA      0x61
#define TeclaB      0x62
#define TeclaEsp	0x20

// VARIAVEIS GLOBAIS
long contador = 0;   // Variavel incrementada continuamente pelas threads
int  tecla = 0;      // Caracter digitado no teclado

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

pthread_cond_t CondVarA, CondVarB, CondVarEsp;

struct thread_data {
    int tecla;
    int index;
    pthread_cond_t *condvar;
};

void* AlocFunc(void* thread_arg) {
    struct thread_data *parametros;
    // pthread_cond_t *condvar;
    int index = 0, status = 0;

    parametros = (struct thread_data *) thread_arg;
    index = (*parametros).index;
    // condvar = (*parametros).condvar;

    do {

        LockMutex(&MeuMutex);

        if (status !=0){
		  printf ("Erro na conquista do Mutex! Codigo = %d\n", status);
		  exit(0);
		}

        while (tecla != (*parametros).tecla) { // testa se a tecla digitada é a tecla que a thread espera
            // solta a trava do mutex e bloqueia a thread
            status = pthread_cond_wait((*parametros).condvar, &MeuMutex);

            if (status !=0){
				printf ("Thread %d: Erro %d na espera da condicao!\n", index, status);
		        exit(0);
		    }

            printf ("Thread %d: condicao sinalizada! tecla = \"%c\"\n", index, tecla);

            if (tecla == ESC) {
                break;
            }
        }

        if (tecla != ESC) {
            tecla = 0;
        }

        UnlockMutex(&MeuMutex);

        if (status !=0){
		  printf ("Erro na liberacao do Mutex! Codigo = %d\n", status);
		  exit(0);
		}

    } while (tecla != ESC);

    pthread_exit((void *)(intptr_t) index);
    return((void *)(intptr_t) index);
}

int main() {

    pthread_t hThreads[NTHREADS];
    void *tRetStatus;
    int status = 0, teclas[5] = {TeclaA, TeclaB, TeclaEsp, TeclaEsp, TeclaEsp};
    pthread_cond_t *condvars[5] = {&CondVarA, &CondVarB, &CondVarEsp, &CondVarEsp, &CondVarEsp};
    thread_data parametros[5];

    // --------------------------------------------------------------------------
	// Criação dos objetos de sincronização
	// --------------------------------------------------------------------------

    pthread_mutexattr_init(&MeuMutexAttr);
    status = pthread_mutexattr_settype(&MeuMutexAttr, PTHREAD_MUTEX_ERRORCHECK);

    if (status != 0) {
		printf("Erro nos atributos do mutex! Codigo = %d\n", status);
		exit(0);
	}

    pthread_mutex_init(&MeuMutex, &MeuMutexAttr);

    if (status != 0) {
		printf("Erro na criação do mutex! Codigo = %d\n", status);
		exit(0);
	}

    status = pthread_cond_init(&CondVarA, NULL);
    if (status !=0){
		printf ("Erro na criacao de CondVarA! Codigo = %d\n", status);
		return 0;
	}

    status = pthread_cond_init(&CondVarB, NULL);
    if (status !=0){
		printf ("Erro na criacao de CondVarB! Codigo = %d\n", status);
    }

    status = pthread_cond_init(&CondVarEsp, NULL);
    if (status !=0){
		printf ("Erro na criacao de CondVarEsp! Codigo = %d\n", status);
		return 0;
	}

    // --------------------------------------------------------------------------
	// Criação das threads secundárias
	// --------------------------------------------------------------------------

    for (int i = 0; i < NTHREADS; i++) {
        parametros[i].tecla = teclas[i];
        parametros[i].index = i;
        parametros[i].condvar = condvars[i];

        status = pthread_create(
            &hThreads[i],
            NULL,
            AlocFunc,
            (void *) &parametros[i]
        );

        if (!status) {
			printf("Thread %d criada com ID = %ld\n", i, (unsigned long)  &hThreads[i]);
		} else {
			printf("Erro ao criar a thread %d! Codigo = %d\n", i, status);
		}
    }

	// --------------------------------------------------------------------------
	// Leitura do teclado
	// --------------------------------------------------------------------------

    do {
        printf("Digite \"a\", \"b\", <Espaco> ou <Esc> para terminar:\n");

        tecla = getch_linux();

        if (tecla == TeclaA || tecla == ESC) {
            status = pthread_cond_signal(&CondVarA);
            if (status !=0){
		       printf ("Erro na sinalizacao de CondVarA! Codigo = %d\n", status);
		       return 0;
	        }
        }

        if (tecla == TeclaB || tecla == ESC) {
            status = pthread_cond_signal(&CondVarB);
            if (status !=0){
		       printf ("Erro na sinalizacao de CondVarB! Codigo = %d\n", status);
		       return 0;
	        }
        }

        if (tecla == TeclaEsp || tecla == ESC) {
            status = pthread_cond_broadcast(&CondVarEsp);
            if (status !=0){
		       printf ("Erro na sinalizacao de CondVarEsp! Codigo = %d\n", status);
		       return 0;
	        }
        }

    } while (tecla != ESC);

	// --------------------------------------------------------------------------
	// Aguarda termino das threads secundarias
	// --------------------------------------------------------------------------

    for (int i =0; i < NTHREADS; i++) {
        printf("Aguardando o término da thread %ld... Código de saída: ", (unsigned long) &hThreads[i]);
        status = pthread_join(
            hThreads[i],
            (void **) &tRetStatus
        );

        if (status != 0) {
            printf("Erro em pthread_join()! Codigo = %d\n", status);
        } else {
            if ((intptr_t)tRetStatus != -1) {
				printf("Thread Abelha %ld: status de retorno = %ld\n", (intptr_t) i, (intptr_t)tRetStatus);
			} else {
				printf("Thread Urso: status de retorno = %ld\n", (intptr_t)tRetStatus);
			}
        }
    }

    // --------------------------------------------------------------------------
	// Destruição dos objetos de sincronização
	// --------------------------------------------------------------------------

    pthread_mutex_destroy(&MeuMutex);
    if (status !=0){
		printf ("Erro na destruicao do Mutex! C�digo = %d\n", status);
		exit(0);
	}

    pthread_cond_destroy(&CondVarA);
    if (status !=0){
		printf ("Erro na destruicao de CondVarA! C�digo = %d\n", status);
		exit(0);
	}

    pthread_cond_destroy(&CondVarB);
    if (status !=0){
		printf ("Erro na destruicao de CondVarB! C�digo = %d\n", status);
		exit(0);
	}

    pthread_cond_destroy(&CondVarEsp);
    if (status !=0){
		printf ("Erro na destruicao de CondVarEsp! C�digo = %d\n", status);
		exit(0);
	}

    return 0;
}

