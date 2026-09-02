//**********************************************************************
//
//	Programa 1 de exemplo de sinais POSIX
//  =====================================
//
//  Alteração da máscara de sinais e consulta de sinais
//
//	Versão: 1.0	  Data: 25/04/2020
//
//  Autor: Prof. Luiz T. S. Mendes (DELT/EE-UFMG)
// 
//  Compile com "gcc PosixSignals01PENDING.cpp -o PosixSignals02PENDING"
//
//**********************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include "conio.h"      // kbhit() e _getch()

#define  ESC 0x1B
#define  TRUE 1

//====================================================================
// Thread primária
//
// Executa as seguintes ações:
// 
// 1. Executa um "loop" de leitura do teclado e, se a tecla "0" for
//    digitada, altera a máscara de sinais para bloquear o sinal
//    SIGINT (CTRL-C)
// 3. Se ESC for digitado, encerra o programa normalmente.
// 
//====================================================================

int main()
{
	int status;                  // Status de retorno das funções chamadas
	int tecla;                   // tecla digitada
	int opcao;                   // Opcão escolhida pelo usuário
	int vez = 0;                 // Indicador de "primeira vez"
	sigset_t blockSet, prevMask; // Conjuntos de sinais
	sigset_t pendingSet;

	do {
		printf("\nEscolha se quer bloquear SIGINT [a] ou nao [b]:");
		tecla = _getch();
		if (tecla == 'a' || tecla == 'b') break;
		printf("\nTecla invalida!\n");
	} while (TRUE);
	opcao = tecla;

	// Laço de tratamento do teclado
	do {
		printf("\nDigite uma tecla qualquer (ESC para encerrar):");
		if (kbhit()) {
			tecla = _getch();
			if (tecla == '0' && vez == 0) {
				// Muda a máscara de sinais do processo para bloquear
				// SIGINT (CTRL-C)
				sigemptyset(&blockSet);
				sigaddset(&blockSet, SIGINT);

				// Bloqueia SIGINT e obtém a máscara de sinais anterior
				if (opcao == 'a') {
					printf("Bloqueando SIGINT...\n");
					if (sigprocmask(SIG_BLOCK, &blockSet, &prevMask) == -1) {
						printf("Erro na chamada a sigprocmask [1]: valor = %d\n", errno);
						exit(-1);
					}
				}
				vez = 1;
			}
		}
		sleep (1);

		// Testa se há algum sinal SIGINT pendente
		status = sigpending(&pendingSet);
		if (status != 0) {
			printf ("Erro em sigpending(): %d\n", errno);
			exit(-1);
		}
		if (sigismember(&pendingSet, SIGINT) != 0)
			printf ("\nSIGINT pendente!\n");

	} while (tecla != ESC);

	// Restaura a máscara de sinais original (na realidade desnecessário
	// aqui, visto que o processo será encerrado)
	if (sigprocmask(SIG_SETMASK, &prevMask, NULL) == -1) {
		printf("Erro na chamada a sigprocmask {2]: valor = %d\n", errno);
		exit(-1);
	}

	//printf("\nAcione uma tecla para terminar\n");
	//_getch();
	
	return EXIT_SUCCESS;

}  // main
