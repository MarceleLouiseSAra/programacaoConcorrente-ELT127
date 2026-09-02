//**********************************************************************
//
//	Programa 3 de exemplo de sinais POSIX
//  =====================================
//
//  "Signal handler" para SIGINT (CTRL-C)
//
//	Versão: 1.0	  Data: 12/04/2020
//
//  Autor: Prof. Luiz T. S. Mendes (DELT/EE-UFMG)
//
//**********************************************************************

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include "conio.h"      // kbhit() e _getch()
#include <time.h>

#define  ESC 0x1B

void SignalHandlerCTRLC(int signo);
void SignalHandlerUSR1(int signo);

//====================================================================
// Thread primária
//
// Executa as seguintes ações:
// 1. Executa um "loop" de leitura do teclado e, se a tecla "0" for
//    digitada, muda a disposição do sinal SIGINT para que seja
//    capturado e tratado por um "handler"
// 3. Se ESC for digitado, encerra o programa normalmente.
// 
//====================================================================

int main()
{
	//char buff[1];
    struct sigaction sa_kbd;    // Estrutura necessária para "signal handler" de SIGINT
    struct sigaction sa_usr;    // Idem, SIGUSR1
	int status;                 // Status de retorno das funções chamadas
	int tecla;
	int vez = 0;

	// Laço de tratamento do teclado
	do {
		printf("Digite uma tecla qualquer (ESC para encerrar):\n");
		if (kbhit()) {
			tecla = _getch();
			if (tecla == '0' && vez == 0) {
				// Define handler para SIGINT
				printf ("Definindo 'signal handler' para SIGINT...\n");
				memset(&sa_kbd, 0, sizeof(sa_kbd)); //IMPORTANTE - SA_KBD PODE CONTER LIXO E ISTO
	                                                 // PROVOCAR MAU FUNCIONAMENTO
				sa_kbd.sa_handler = SignalHandlerCTRLC;
				status = sigaction(SIGINT, &sa_kbd, NULL);
				if (status != 0) {
					printf("Erro em sigaction [1]: valor = %d\n", errno);
					exit (-1);
				}

				// Define handler para SIGUSR1
				printf ("Definindo 'signal handler' para SIGUSR1...\n");
				memset(&sa_usr, 0, sizeof(sa_usr)); 
				sa_usr.sa_handler = SignalHandlerUSR1;
				status = sigaction(SIGUSR1, &sa_usr, NULL);
				if (status != 0) {
					printf("Erro em sigaction [2]: valor = %d\n", errno);
					exit (-1);
				}
				vez = 1;
			}
		}
		sleep (1);
	} while (tecla != ESC);

	//printf("\nAcione uma tecla para terminar\n");
	//_getch();

	return EXIT_SUCCESS;

}  // main

//====================================================================
// "Signal handler" para SIGINT (CTRL-C)
//
// ATENÇÃO: Este handler emprega a função sleep(), que não pode ser
// retomada (continuada) caso seja interrrompida pela entrega de um
// outro sinal para o qual exista um respectivo signal handler.
//
//====================================================================

void SignalHandlerCTRLC(int signo) {

	int tempo_restante;
	time_t hora;

	if (signo == SIGINT)
      printf("\nSIGNAL HANDLER: Capturado sinal %d [CTRL-C]\n", signo);
	else
      printf("\nSIGNAL HANDLER: Capturado sinal %d\n", signo);
	
	hora = time(NULL);
    printf ("SIGINT signal handler: dormindo por 5 segundos. Hora local = %s", ctime(&hora));
	tempo_restante = sleep(5);
	if (tempo_restante != 0)
		printf ("SIGINT signal handler: sleep() interrompida, %d segundos restantes\n", tempo_restante);
	else {
		hora = time(NULL);
		printf("SIGINT signal handler: acordando. Hora local = %s", ctime(&hora));
	}
}

//====================================================================
// "Signal handler" para SIGFPE
//====================================================================

void SignalHandlerUSR1(int signo) {
	if (signo == SIGUSR1)
      printf("SIGNAL HANDLER: Capturado sinal %d [SIGUSR1]\n", signo);
	else
      printf("SIGNAL HANDLER: Capturado sinal %d\n", signo);
}


