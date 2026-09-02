//***************************************************************************************
//	Programa de exemplo do emprego de "signals" para sincronismo entre threads
//
//  Adaptação do exemplo de sincronização empregando "eventos" da plataforma
//  Win32 (exemplo 2, capítulo 4 do livro de "Programação Multithreaded em ambiente
//  Windows NT© - uma visão de  Automação", de Constantino Seixas Filho e Marcelo
//  Szuster)
//
//	Versão: 1.0	  Data: 14/04/2020
//          1.1   Data: 01/05/2022
//                Modificação para que SIGUSR1 acorde alternadamente
//                as threads secundárias A e B.
//
// Compile o programa com
// 
//     gcc -pthread PosixSignals07ThreadsSincronismoV2.cpp -o PosixSignals07ThreadsSincronismoV2
//
//  Autor: Prof. Luiz T. S. Mendes (DELT/EE-UFMG)
//
//***************************************************************************************

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include "conio.h"      // kbhit() e _getch()

#define SP			0x20
#define	ESC			0x1B
#define NUM_THREADS	2

// Protótipos das funções empregadas
void *WaitSignalFunc(void *arg);  // Declaração da função correspondente às threads secundárias

// variável para implementar fila sequencial das threads secundárias
int vez = 0;

int main()
{
	pthread_t hThread[NUM_THREADS];
	sigset_t sigset;            // Variável para definir máscara de sinais a bloquear
    struct sigaction sa_kbd;    // Estrutura necessária para redefinir a disposição de SIGINT (CTRL-C)
	void *thread_ret;           // Status de retorno da thread secundária
	int status;                 // Status de retorno das funções chamadas
    int signum;                 // Numero do sinal
	int i, nTecla, vez_thread;

	// Define máscara de sinais bloqueando os sinais de sincronização SIGUSR1,
	// SIGUSR2 e SIGQUIT. Threads secundárias herdam uma cópia desta máscara de sinais.
	sigemptyset(&sigset);
    sigaddset(&sigset, SIGUSR1);
    sigaddset(&sigset, SIGUSR2);

	// EXPERIMENTO EM CLASSE I: Adicione o sinal SIGINT (CTRL-C) à máscara e veja
	// o efeito no funcionamento da aplicação.
	//sigaddset(&sigset, SIGINT);

    status = pthread_sigmask(SIG_BLOCK, &sigset, NULL);
	if (status != 0) {
		printf("Erro em pthread_sigmask: %d\n", status);
		exit(-1);
	}

    // Cria as threads secundárias. O valor corrente da variável de iteração é passado
	// como parâmetro para cada thread criada.
	for (i = 0; i < NUM_THREADS; i++){
		// GCC causa warning "cast to pointer from integer of different size" na conversão
		// abaixo de (void *) i. Temos antes de converter para tipo intptr_t:
		//   https://stackoverflow.com/questions/9251102/warning-cast-to-pointer-from-integer-of-different-size-wint-to-pointer-cast
		//status = pthread_create(&hThread[i], NULL, WaitSignalFunc, (void *) i);
		status = pthread_create(&hThread[i], NULL, WaitSignalFunc, (void *) (intptr_t) i);
		if (status == 0) printf("Thread WaitSignalFunc #%d criada com Id= %#lx \n", i, (long unsigned) hThread[i]);
		else {
			  printf ("Erro na criacao da thread WaitSignalFunc #%d! Codigo = %d\n", i, status);
			  exit(-1);
		}
	}

	// EXPERIMENTO EM CLASSE II: Cancele o experimento I acima, se este estiver em vigor,
	// e ajuste a máscara de sinais somente da **thread primária** acrescentando o
	// sinal SIGINT. Veja o que acontece com a aplicação. Lembre-se que bloquer um
	// sinal é diferente de ignorar o sinal, e que a disposição de um sinal vale para
	// o processo como um todo.
	/*
	sigaddset(&sigset, SIGINT);
	status = pthread_sigmask(SIG_BLOCK, &sigset, NULL);
	if (status != 0) {
		printf("Erro em pthread_sigmask: %d\n", status);
		exit(-1);
	}
	*/

    // EXPERIMENTO EM CLASSE III: Cancele os experimentos I e II acima, se estiverem em vigor,
	// e defina a disposição do sinal SIGINT para SIG_IGN. Veja o efeito na aplicação.
	// Lembre-e novamente que  a disposição de um sinal vale para o processo como um todo.
	/*
	memset(&sa_kbd, 0, sizeof(sa_kbd)); //IMPORTANTE - SA_KBD PODE CONTER LIXO E ISTO
	                                    // PROVOCAR MAU FUNCIONAMENTO
	sa_kbd.sa_handler = SIG_IGN;
	status = sigaction(SIGINT, &sa_kbd, NULL);
	if (status != 0) {
		printf("Erro em sigaction: valor = %d\n", errno);
		exit (-1);
	}
	*/
	
	// Laço de leitura do teclado
	vez_thread = 0;
	do {
		printf("Tecle <SP> para enviar sinal ou <Esc> para terminar\n");
		nTecla = _getch();
		if (nTecla == SP) {
			status = pthread_kill(hThread[vez_thread], SIGUSR1);     // Gera sinal
			if (status != 0) {
				if (status == 3)
					printf("Erro em pthread_kill: thread tid = %#lx inexistente ou ja' encerrada\n", (long unsigned)hThread[i]);
				else
					printf("Erro em pthread_kill: i=%d tid = %#lx erro = %d\n", i, (long unsigned)hThread[i], status);
				exit(-1);
			}
			vez_thread = 1 - vez_thread;
		}
		else if (nTecla == ESC) {
			for (i = 0; i < NUM_THREADS; i++) {
				status = pthread_kill(hThread[i], SIGUSR2);     // Gera sinal
				if (status != 0) {
					if (status == 3)
						printf("Erro em pthread_kill: thread tid = %#lx inexistente ou ja' encerrada\n", (long unsigned)hThread[i]);
					else
						printf("Erro em pthread_kill: i=%d tid = %#lx erro = %d\n", i, (long unsigned)hThread[i], status);
					exit(-1);
				}
			}
		}
	} while (nTecla != ESC);

	// Espera todas as threads terminarem
	for (i = 0; i < NUM_THREADS; i++){
		status = pthread_join(hThread[i], &thread_ret);
		// A linha [1] abaixo provocará warning do gcc:
		//    "format ‘%d’ expects argument of type ‘int’, but argument 3 has type ‘void ’"
		// Inicialmente tento um casting para int (linha [2]), mas agora aparece erro:
		//    "cast from ‘void*’ to ‘int’ loses precision".
		// A solução final é fazer um casting precedente para intptr_t.
		// https://stackoverflow.com/questions/1640423/error-cast-from-void-to-int-loses-precision
		// [1] if (status == 0) printf ("Thread %#lx: status de retorno = %d\n", (long unsigned) hThread[i], thread_ret);
		// [2] if (status == 0) printf ("Thread %#lx: status de retorno = %d\n", (long unsigned) hThread[i], (int) thread_ret);
		if (status == 0) printf ("Thread %#lx: status de retorno = %d\n", (long unsigned) hThread[i], (int) (intptr_t) thread_ret);
		else printf ("Erro em pthread_join! i = %d status = %d\n", i, status);
	}

	//printf("\nAcione uma tecla para terminar\n");
	//_getch(); // // Pare aqui, caso não esteja executando no ambiente MDS

	return EXIT_SUCCESS;
}  // main


void *WaitSignalFunc(void *arg)
{	
	int signum;
	int status;
	int id;
	sigset_t thread_sigset;

	//id = (int) arg;
	id = (int) (intptr_t) arg;

	//sleep(1);

	// Recupera sua máscara de sinais corrente, lembrando que a mesma é herdada da
	// thread criadora. Outra forma de fazer isto seria declarar a máscara como uma
	// variável global, mas aqui escolhemos o uso de pthread_sigmask para exemplificar
	// seu uso nesta situação. Lembre que, se o segundo parâmetro for NULL, o primeiro
	// parâmetro é desconsiderado.
	status = pthread_sigmask(-1, NULL, &thread_sigset);
	if (status != 0) {
		printf("WAITSIGNALFUNC- Erro em pthread_sigmask: i=%d erro = %d\n", id, status);
		exit(-1);
	}
	do {
		// Aguarda a sinalização proveniente da thread primária.  sinalização. Lembramos os sinais possíveis:
		// SIGUSR1 ==> Usuário digitou tecla de espaço
		// SIGUSR2 ==> Usuário digitou ESC
		// No caso de tecla de espaço, as threads devem imprimir mensagem na console em regime de "fila"
		// dequencial começando pela thread de menor ID (o parâmetro recebido da thread primária). Assim,
		// a thread selecionada deve atualizar a variável global "vez" para passar a vez para a próxima
		// thread. Este recurso foi usado aqui para simular o funcionamento do objeto "evento" no
		// ambiente Windows.
		status  = sigwait(&thread_sigset, &signum);
		if (status == -1) {
			printf("Erro em sigwait()! id = %d erro = %d\n", id, errno);
			exit(-1);
		}
		// Testa o sinal recebido e age de acordo
		if (signum == SIGUSR1){//Tecla de espaço digitada
			if (vez == id) {//Se for a vez da thread
				printf("Thread %d: Sinal SIGUSR1 recebido\n", id);
				vez = ++vez % NUM_THREADS;
			}
		}
		else if (signum == SIGUSR2) {
			printf("Thread %d: Sinal SIGUSR2 recebido... encerrando\n", id);
			break;	// Abandona laço e encerra
		}
		else {
			printf("Thread %d: Sinal inesperado %d recebido... encerrando\n", id, signum);
			break;	// Abandona laço e encerra
		}

	} while (signum == SIGUSR1);	// Esc foi escolhido

	printf("Thread WaitSignalFunc %d terminando...\n", id);
	// A linha abaixo provocará warning do gcc "cast to pointer from integer of different size"
	// Para eliminar o warning faço um "cast" para o tipo intptr_t
	//pthread_exit((void *)id);
	pthread_exit((void *)(intptr_t)id);
} // WaitSignalFunc



