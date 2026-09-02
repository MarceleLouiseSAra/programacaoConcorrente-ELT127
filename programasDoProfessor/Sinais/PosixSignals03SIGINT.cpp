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
//  Compile com "gcc PosixSignals03SIGINT.cpp -o PosixSignals03SIGINT"
//
//**********************************************************************

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include "conio.h"      // kbhit() e _getch()

#define  ESC 0x1B

void SignalHandlerCTRLC(int signo);

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
    struct sigaction sa_kbd;    // Estrutura necessária para "signal handler" de CTRL-C
	int status;                 // Status de retorno das funções chamadas
	int tecla;
	int vez = 0;

	// Laço de tratamento do teclado
	do {
		printf("\nDigite uma tecla qualquer (ESC para encerrar):");
		if (kbhit()) {
			tecla = _getch();
			if (tecla == '0' && vez == 0) {
				// Define handler para SIGINT
				printf ("\nDefinindo 'signal handler' para SIGINT...\n");
				memset(&sa_kbd, 0, sizeof(sa_kbd)); //IMPORTANTE - SA_KBD PODE CONTER LIXO E ISTO
	                                                 // PROVOCAR MAU FUNCIONAMENTO
				sa_kbd.sa_handler = SignalHandlerCTRLC;
				status = sigaction(SIGINT, &sa_kbd, NULL);
				if (status != 0) {
					printf("Erro em sigaction: valor = %d\n", errno);
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
// "Signal handler" para CTRL-C
//====================================================================

void SignalHandlerCTRLC(int signo) {
	if (signo == SIGINT)
      printf("\nSIGNAL HANDLER: Capturado sinal %d [CTRL-C]", signo);
	else
      printf("\nSIGNAL HANDLER: Capturado sinal %d", signo);
}

