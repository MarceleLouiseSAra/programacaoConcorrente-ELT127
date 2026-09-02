//***************************************************************************************
//	Programa de exemplo da medição de intervalos de tempo no padrão POSIX
//
//	Data: 19/04/2020
//
//  Para compilar:
// 
//    gcc PosixTimers10MedicaoIntervaloTempo.cpp -o PosixTimers10MedicaoIntervaloTempo
//
//  Autor: Prof. Luiz T. S. Mendes (DELT/EE-UFMG)
//
//***************************************************************************************

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

int main() {

	long unsigned ctr1 = 0, ctr2 = 0, freq = 0;
	int acc = 0, i = 0;
	struct timespec res, inicio, fim;

	//Obtém a resolução do relógio
	if (clock_getres(CLOCK_REALTIME, &res) == -1) {
	   printf ("Erro na chamada a clock_getres() - codigo do erro = %d\n", errno);
	   exit (-1);
	}
	printf("Resolucao do clock: segundos = %d nanossegundos = %d\n", (int)res.tv_sec, (int)res.tv_nsec);

	// Início da medição de tempo
	if (clock_gettime(CLOCK_REALTIME, &inicio) == -1) {
	   printf ("Erro na chamada a clock_gettime() [1] - codigo do erro = %d\n", errno);
	   exit (-1);
	}

	// Laço de incremento de uma variável inteira 1000 vezes
	for (i=0; i<1000; i++) acc++;

	// Fim da medição de tempo
	if (clock_gettime(CLOCK_REALTIME, &fim) == -1) {
	   printf ("Erro na chamada a clock_gettime() [2] - codigo do erro = %d\n", errno);
	   exit (-1);
	}

	printf("Tempo inicial: %lu seg %lu nseg\n", inicio.tv_sec, inicio.tv_nsec);
	printf("Tempo final:   %lu seg %lu nseg\n", fim.tv_sec, fim.tv_nsec);
	printf("Resolucao de CLOCK_REALTIME: %lu seg %lu nseg\n", res.tv_sec, res.tv_nsec);
	
    printf("Tempo de 1000 incrementos: %e segundos.\n", fim.tv_sec - inicio.tv_sec +
		  (fim.tv_nsec - inicio.tv_nsec) / (1000000000.0/res.tv_nsec));

    return 0;
}
