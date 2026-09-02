//
//	Programa de exemplo de "signals" e temporizadores POSIX
//
//  Adaptação do exemplo de temporizações empregando "waitable timers" da plataforma
//  Win32 (exemplo 4, capítulo 4 do livro de "Programação Multithreaded em ambiente
//  Windows NT© - uma visão de  Automação", de Constantino Seixas Filho e Marcelo
//  Szuster)
//
//	Versão: 1.0	  Data: 23/03/2020
// 
// Compile com
// 
//    gcc -pthread PosixTimers09MalhaPID.cpp -o PosixTimers09MalhaPID -lrt
// 
//    Observação: a posição do switch "-lrt" no comando acima faz diferença
//    em algumas distribuições Linux. A forma dada é como funciona no Ubuntu.
//
//  Autor: Prof. Luiz T. S. Mendes (DELT/EE-UFMG)

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include "bGetFloat.h"      // Função para ler um valor em ponto flutuante

// Protótipos das funções empregadas
void *PidControlFunc(void *arg);  // Declaração da função correspondente à thread secundária
void Pid(double);			      // Função PID
void SignalHandler(int signo);

// Variáveis globais para comunicação entre threads
double SetPoint= 0.0;
double dInput;

//====================================================================
// Thread primária
//
// Executa as seguintes ações:
// 1. Define um "signal handler" para a tecla CTRL-C
// 2. Define a máscara de sinais a serem bloqueados pelas threads
//    (SIGUSR1, SIGALRM e SIGQUIT)
// 3. Cria a thread secundária
// 4. Inicializa a estrutura de temporização, cria o temporizador e
//    dispara o mesmo
// 5. Executa um "loop" de leitura do teclado e sinaliza a thread
//    secundária em conformidade com o valor digitado
// 6. Em caso de indicação de término do programa, abandona o "loop"
//    anterior e aguarda o término da thread secundária.
//====================================================================

int main()
{
	//char buff[1];
	pthread_t hThread;          // Identificador da thread secundária
	struct itimerspec timer;    // Estrutura necessária para uso de temporizadores
	struct sigevent sigev;      // Idem
	sigset_t sigset;            // Variável para definir máscara de sinais a bloquear
    struct sigaction sa_kbd;    // Estrutura necessária para "signal handler" de CTRL-C
	timer_t timer_id;           // Identificador do temporizador criado
	void *thread_ret;           // Status de retorno da thread secundária
	int status;                 // Status de retorno das funções chamadas
    int signum;                 // Numero do sinal

	BOOL bStatus;               // Status de retorno da função "bGetFloat()". Observe que
	                            //   o tipo de dados BOOL (em maiúsculas) é específico da
	                            //   plataforma Win32 e é simulado aqui apenas para fins de
	                            //   compatibilidade (vide arquivo "bGetFloat.h")

	// Define handler para CTRL-C
	memset(&sa_kbd, 0, sizeof(sa_kbd)); //IMPORTANTE - SA_KBD PODE CONTER LIXO E ISTO
	                                    // PROVOCAR MAU FUNCIONAMENTO
	sa_kbd.sa_handler = SignalHandler;
	status = sigaction(SIGINT, &sa_kbd, NULL);
	if (status != 0) {
		printf("Erro em sigaction: valor = %d\n", errno);
		exit (-1);
	}

    // Bloqueia os sinais SIGUSR1, SIGQUIT e SIGALRM. Threads secundárias
	// herdam uma cópia desta máscara de sinais.
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGQUIT);
    sigaddset(&sigset, SIGUSR1);
	sigaddset(&sigset, SIGALRM);
	status = pthread_sigmask(SIG_BLOCK, &sigset, NULL);
	if (status != 0) {
		printf("Erro em pthread_sigmask: %d\n", status);
		exit(-1);
	}

	// Cria a thread secundária
	status = pthread_create(&hThread, NULL, PidControlFunc, (void *) &sigset);
	if (status == 0) printf("Thread PidControlFunc criada com Id= %#x \n", (int) hThread);
	else {
			printf ("Erro na criacao da thread PidControlFunc! Codigo = %d\n", status);
			exit(-1);
	}

	// Prepara estrutura de dados necessária à criação do temporizador
    memset(&sigev, 0, sizeof(struct sigevent));
    sigev.sigev_notify = SIGEV_SIGNAL;
    sigev.sigev_signo = SIGALRM;
	
	// Cria temporizador
    status = timer_create(CLOCK_MONOTONIC, &sigev, &timer_id);
    if (status < 0) {
       printf("Erro na criacao do temporizador = %d\n", errno);
       exit(-1);
    }

	// Prepara estrutura de dados tal que a primeira temporização ocorra 2 segundos
    // depois do disparo do temporizador, e depois a cada 500ms
	timer.it_value.tv_sec = 2;
	timer.it_value.tv_nsec = 0;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_nsec = 500000000;

    // Dispara o temporizador
    status = timer_settime(timer_id, 0 /*TIMER_ABSTIME*/, &timer, NULL);
    if (status != 0) {
       printf("Erro no disparo da temporizacao = %d\n", errno);
       exit(-1);
    }

	// Laço de tratamento do teclado. A função "bGetFloat()" está definida no
	// cabeçalho "bGetFloat.h"
	do {
		printf("\nEscreva novo valor de SetPoint:");
		// le string ou ESC
		bStatus = bGetFloat(&dInput, 6);
		if (bStatus) {
			// Novo valor de set-point: acorda a thread que controla a malha PID
			status = pthread_kill(hThread, SIGUSR1);  // SetPoint mudou
			if (status != 0) {
				printf("Erro em pthread_kill [SIGUSR1]: %d\n", status);
				exit(-1);
			}
		}
		else {
			// Valor de set-point inválido: pressupõe término do programa
			// e acorda a thread que controla a malha PID
			status = pthread_kill(hThread, SIGQUIT);  // Fim do programa
			if (status != 0) {
				printf("Erro em pthread_kill [SIGQUIT]: %d\n", status);
				exit(-1);
			}
		}
	} while (bStatus);

	// Espera thread secundária terminar
	status = pthread_join(hThread, &thread_ret);
	if (status == 0) printf ("Thread %#x: status de retorno = '%s'\n", (int) hThread, (char *) thread_ret);
	else printf ("Erro em pthread_join: %d\n", status);

	//printf("\nAcione uma tecla para terminar\n");
	//_getch();

	return EXIT_SUCCESS;

}  // main

//====================================================================
// Thread secundária - simula o algoritmo de controle de uma malha PID
//====================================================================

void *PidControlFunc(void *arg) {

	int signum;
	// A declaração abaixo de pthread_ret provoca o seguinte warning no gcc:
	// "ISO C++ forbids converting a string constant to ‘char*’"
	// A solução é fazer um cast da cadeia para (char *)
	// https://stackoverflow.com/questions/20944784/why-is-conversion-from-string-constant-to-char-valid-in-c-but-invalid-in-c
	//char *pthread_ret = "OK";
	char *pthread_ret = (char *)"OK";
	int status;

	sigset_t *sigset = (sigset_t *) arg;
	
	do {
		// Aguarda alguma sinalização. Lembramos os sinais possíveis:
		// SIGUSR1 ==> Usuário digitou novo set-point
		// SIGALRM ==> Temporização
		// SIGQUIT ==> Usuário deseja encerrar programa
		status  = sigwait(sigset, &signum);
		if (status != 0) {
			printf("Erro em sigwait(): %d\n", status);
			exit(-1);
		}
		// Testa o sinal recebido e age de acordo
		if (signum == SIGUSR1) {// Novo set-point digitado pelo usuário
			SetPoint = dInput;
			printf("\nNovo valor de set point: %f", SetPoint);
		}
		else if (signum == SIGALRM ) Pid(SetPoint);	// Ocorreu ativação de tempo
	} while (signum != SIGQUIT);	// Esc foi escolhido

	printf("\nThread PidControlFunc terminando...\n");
	pthread_exit((void *) pthread_ret);
} // WaitSignalFunc


void Pid(double SetPoint)
{// Algoritmo Pid;

	struct tm *tempo;
	time_t ticks;
	
	ticks = time(NULL);
	tempo = localtime(&ticks);
	printf("\nSP=%6.2f  %02d:%02d:%02d", SetPoint, tempo->tm_hour,
		   tempo->tm_min, tempo->tm_sec);

}// Pid

//====================================================================
// "Signal handler" para CTRL-C
//====================================================================

void SignalHandler(int signo) {
	if (signo == SIGINT)
      printf("\nSIGNAL HANDLER: Capturado sinal %d [CTRL-C]", signo);
	else
      printf("\nSIGNAL HANDLER: Capturado sinal %d", signo);
}

