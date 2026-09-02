//***************************************************************************************
//	Programa de exemplo do emprego de "signals" em programas multithread
//
//  Segue-se aqui a arquitetura proposta em "The Linux Programming Interface" (Michael
//  Kerrisk, 2010), seção 33.2.4 ("Dealing with asychronous signals sanely"), na qual
//  uma thread é selecionada para lidar com os sinais assíncronos.
// 
//  O programa é composto de sua thread primária e duas threads secundárias. Uma das
//  threads secundárias é designada para capturar sinais por meio da função sigwait().
//  Os sinais capturados são informados às duas outras threads por meio de semáforos.
// 
//  Compile o programa com
// 
//     gcc -pthread PosixSignals06Threads.cpp -o PosixSignals06Threads
//
//	Versão: 1.0	  Data: 26/04/2020
//          1.1         28/04/2022 Mudanças cosméticas e melhoria dos comentários
//
//  Autor: Prof. Luiz T. S. Mendes (DELT/EE-UFMG)
//
//***************************************************************************************

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include "conio.h"      // kbhit() e _getch()
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

// Protótipos das funções empregadas como threads
void *ThreadDedicadaSinais(void *arg);  // Thread dedicada ao tratamento de sinais
void *ThreadNormal(void *arg);          // Thread comum

// Declaração dos semáforos
sem_t Sem1, Sem2;
int flag_termino = 0;

//====================================================================
// Thread primária
//
// Executa as seguintes ações:
// 1. Bloqueia os sinais SIGUSR1, SIGUSR2 e SIGINT em sua máscara de
//    sinais.
// 2. Cria os semáforos para sincronização enthe threads.
// 3. Cria as threads secundárias
// 4. Laço:
//    4.1 Aguarda sinalização do semáforo "Sem1"
//    4.2 Imprime mensagem e testa flag_termino
//    4.3 Se flag_termino = 1 abandona laço;
// 4. Aguarda término das threads secundárias
// 5. Destrói semáforos e encerra.
//
//====================================================================

int main()
{
	pthread_t hThread[2];
	sigset_t sigset;            // Variável para definir máscara de sinais a bloquear
    struct sigaction sa_kbd;    // Estrutura necessária para redefinir a disposição de SIGINT (CTRL-C)
	void *thread_ret;           // Status de retorno da thread secundária
	int status;                 // Status de retorno das funções chamadas
    int signum;                 // Numero do sinal
	int i, nTecla;

	// Define máscara de sinais bloqueando os sinais de sincronização SIGUSR1,
    // SIGUSR2 e SIGINT. Threads secundárias herdam uma cópia desta máscara de sinais.
	/*sigemptyset(&sigset);
	sigaddset(&sigset, SIGUSR1);
	sigaddset(&sigset, SIGUSR2);
	sigaddset(&sigset, SIGINT);
	status = pthread_sigmask(SIG_BLOCK, &sigset, NULL);*/

	// Define máscara de sinais bloqueando TODOS os sinais de sincronização.
	// (Lembre-se, contudo, que SIGKILL e SIGSTOP não são bloqueáveis.)
	sigfillset(&sigset);
    status = pthread_sigmask(SIG_BLOCK, &sigset, NULL);
	if (status != 0) {
		printf("Erro em pthread_sigmask: %d\n", status);
		exit(-1);
	}

	// Obtém o PID (Process IDentifier) do processo a que pertence e o
	// imprime na janela de terminal, para facilitar os testes
	printf("Process ID = %d\n", (int)getpid());

	// Cria os semáforos a serem utilizados
	status = sem_init(&Sem1, 0, 0); //sempre retorna 0
	if (status != 0){
		printf ("Erro na inicializacao do semaforo 1! Codigo = %d\n", errno);
		exit(0);
	}
	status = sem_init(&Sem2, 0, 0); //sempre retorna 0
	if (status != 0){
		printf ("Erro na inicializacao do semaforo 2! Codigo = %d\n", errno);
		exit(0);
	}

	// Criação das threads secundárias
	status = pthread_create(&hThread[0], NULL, ThreadDedicadaSinais, NULL);
	if (status == 0) printf("Thread primaria: ThreadDedicadaSinais criada com Id= %#lx \n",
		                    (long unsigned) hThread[0]);
	else {
		  printf ("Erro na criacao da ThreadDedicadaSinais! Codigo = %d\n", status);
		  exit(-1);
	}
	status = pthread_create(&hThread[1], NULL, ThreadNormal, NULL);
	if (status == 0) printf("Thread primaria: ThreadNormal criada com Id= %#lx \n",
		                    (long unsigned) hThread[1]);
	else {
		  printf ("Erro na criacao da ThreadNormal! Codigo = %d\n", status);
		  exit(-1);
	}

	// Aguarda sinalização do semáforo
	do {
		status = sem_wait(&Sem1);
		if (status != 0) {
			printf("Thread primaria - erro em sem_wait(): %d\n", errno);
			exit(0);
		}
		if (!flag_termino) printf("Thread primaria: simula tratamento de SIGUSR1...\n");
	}
	while (!flag_termino);
			
	// Aguarda término das threads secundárias
	printf("Thread primaria - aguardando termino das threads secundarias\n");
	for (i = 0; i < 2; i++){
		status = pthread_join(hThread[i], &thread_ret);
		// A linha [1] abaixo provocará warning do gcc:
		//    "format ‘%d’ expects argument of type ‘int’, but argument 3 has type ‘void ’"
		// Inicialmente tento um casting para int (linha [2]), mas agora aparece erro:
		//    "cast from ‘void*’ to ‘int’ loses precision".
		// A solução final é fazer um casting precedente para intptr_t.
		// https://stackoverflow.com/questions/1640423/error-cast-from-void-to-int-loses-precision
		// [1] if (status == 0) printf ("Thread %#lx: status de retorno = %d\n", (long unsigned) hThread[i], thread_ret);
		// [2] if (status == 0) printf ("Thread %#lx: status de retorno = %d\n", (long unsigned) hThread[i], (int) thread_ret);
		if (status == 0) printf ("Thread primaria: Thread %#lx status de retorno = '%s'\n", (long unsigned) hThread[i], (char *) thread_ret);
		else printf ("Erro em pthread_join! Thread %#lx: i = %d status = %d\n", (long unsigned) hThread[i], i, errno);
	}

	// Destroi semaforos
    sem_destroy(&Sem1);
	sem_destroy(&Sem2);

	//printf("\nAcione uma tecla para terminar\n");
	//_getch(); // // Pare aqui, caso não esteja executando no ambiente MDS

	return EXIT_SUCCESS;
}  // main

//====================================================================
// Thread secundária dedicada ao tratamento de sinais
//
// Executa as seguintes ações:
// 1. Desbloqueia sinais SIGUSR1, SIGUSR2 e SIGINT
// 2. Laço:
//    2.1 Aguarda entrega de sinal via sigwait();
//    2.2 Caso [sinal]
//        SIGUSR1: Indica tratamento de SIGUSR1
//                 Sinaliza Sem1 para acordar thread primária
//        SIGUSR2: Indica tratamento de SIGUSR2
//                 Sinaliza Sem2 para acordar thread secundária "normal"
//        SIGINT:  Indica término da aplicação
//                 Sinaliza Sem1 e Sem2
//                 Abandona Laço
//        OUTRO:   Imprime mensagem de erro
// 3. Encerra sua execução
// 
// Para testar o programa, execute-o em uma janela de console e,
// em outra janela, envie sinais ao respectivo processo por meio
// dos comandos:
// 
//   ps aux | grep PosixSignals06Threads (para obter o PID do processo)
//   kill -s {USR1 | USR2 | INT} <PID>
//
//====================================================================

void *ThreadDedicadaSinais(void *arg)
{	
	int signum;
	int status;
	sigset_t thread_sigset;
	char *pthread_ret = (char *)"OK";

	// Apenas como um exemplo, vamos retirar SIGFPE da lista de sinais
    // bloqueados. Assim, este sinal, quando ocorrer (desde que gerado
    // por um processo externo como p. ex. "kill", e não devido a uma exceção
	// real de ponto flutuante provocada por qualquer thread deste programa),
	// não estará bloqueado e provocará o encerramento do programa como um todo
    // visto que esta é a sua disposição "default". Para tal, devemos cumprir
	// dois passos:
	//
	// 1. Desbloquear este sinal da máscara de sinais desta thread, lembrando
	//    que esta máscara foi herdada da thread primária, a qual bloqueou
	//    todos os sinais:

	sigemptyset(&thread_sigset);
	sigaddset(&thread_sigset, SIGFPE);
	status = pthread_sigmask(SIG_UNBLOCK, &thread_sigset, &thread_sigset);
	if (status != 0) {
		printf("ThreadDedicadaSinais- Erro em pthread_sigmask: %d\n", status);
		exit(-1);
	}
	
	// (continuação do comentário anterior)
	// 
	// 2. Na chamada a "sigwait", temos de especificar que o conjunto de sinais
	// 	  a serem entregues a esta thread dedicada corresponde a todos os
	//    sinais exceto SIGFPE. Temos assim duas opções:
	// 	  - recuperar a máscara de sinais corrente, que tem esta exata
	// 	    configuração, via pthread_sigmask();
	// 	  - retirar este sinal da máscara antiga ("thread_sigset").
	//    Usaremos aqui esta segunda opção por ser mais direta:
	sigdelset(&thread_sigset, SIGFPE);

	// Aguarda chegada de sinais
	do {
		printf("ThreadDedicadaSinais - aguardando sinal...\n");

		// Define os sinais bloqueados que serão entregues a esta thread
		// (com exceção, conforme mencionado acima, de SIGFPE)
		status  = sigwait(&thread_sigset, &signum);
		if (status == -1) {
			printf("ThreadDedicadaSinais - erro em sigwait(): %d\n", errno);
			exit(-1);
		}

		// Testa o sinal recebido e age de acordo
		switch (signum) {
			case SIGUSR1:
				printf("ThreadDedicadaSinais - recebido SIGUSR1\n");
				status = sem_post(&Sem1);
				if (status != 0) {
					printf("Erro em sem_post() [SIGUSR1]: %d\n", errno);
					exit(-1);
				}
				break;
			case SIGUSR2:
				printf("ThreadDedicadaSinais - recebido SIGUSR2\n");
				status = sem_post(&Sem2);
				if (status != 0) {
					printf("Erro em sem_post() [SIGUSR1]: %d\n", errno);
					exit(-1);
				}
				break;
			case SIGINT:
				printf("ThreadDedicadaSinais - recebido SIGINT\n");
				flag_termino = 1;
				status = sem_post(&Sem1);
				if (status != 0) {
					printf("Erro em sem_post(&Sem1) [SIGINT]: %d\n", errno);
					exit(-1);
				}
				status = sem_post(&Sem2);
				if (status != 0) {
					printf("Erro em sem_post(&Sem2) [SIGINT]: %d\n", errno);
					exit(-1);
				}
				break;
			default:
				printf("ThreadDedicadaSinais - recebido sinal %d\n", signum);
		}
	} while (signum != SIGINT);	// Esc foi escolhido

	printf("ThreadDedicadaSinais terminando...\n");
	pthread_exit((void *) pthread_ret);

} // ThreaddedicadaSinais

//====================================================================
// Thread secundária normal
//
// Executa as seguintes ações:
// 1. Laço:
//    1.1 Aguarda sinalização do semáforo "Sem2"
//    1.2 Imprime mensagem;
//    1.3 Se variável flag_termino = 1 abandona laço.
// 2.Encerra sua execução.
//
//====================================================================
void *ThreadNormal(void *arg) {

	int status;
	char *pthread_ret = (char *)"OK";
	do {
		status = sem_wait(&Sem2);
		if (status != 0) {
			printf("Thread normal - erro em sem_wait(&Sem2): %d\n", errno);
			exit(-1);
		}
		if (!flag_termino) printf ("Thread normal - simula tratamento de SIGUSR2...\n");
	}
	while (!flag_termino);
	printf("Thread normal terminando...\n");
	pthread_exit((void *) pthread_ret);
}





