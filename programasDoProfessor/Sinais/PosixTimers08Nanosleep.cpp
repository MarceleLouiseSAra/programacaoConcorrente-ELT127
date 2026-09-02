//**********************************************************************
//	Programa de exemplo da utilização da função POSIX nanosleep()
//
//  Baseado quase inteiramente no exemplo 23.3, capítulo 23 do livro
//  "The Linux Programming Interface", Michael Kerrisck, 2010, com
//  as seguintes alterações:
//
//  - A função gettimeofday() foi substituída por clock_gettime();
//  - Comentários em português foram inseridos ou traduzidos do inglês;
//  - Substituição da função de erro por "printf";
//  - Pequenas modificações visuais de estilo.
//
//	Data: 17/04/2020
//
//  Para compilar:
// 
//    gcc PosixTimersNanosleep.cpp get_num.cpp -o PosixTimersNanosleep
//
//  Autor: Prof. Luiz T. S. Mendes (DELT/EE-UFMG)
//
//**********************************************************************

#define _POSIX_C_SOURCE 199309
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include "tlpi_hdr.h"

// "Signal handler" para SIGINT. O handler não faz nada, e serve
// apenas para interromper a função nanosleep().
static void sigintHandler(int sig) {
	return;
}

int main(int argc, char *argv[]) {
	//struct timeval start, finish;
	struct timespec start, finish;
	struct timespec request, remain;
	struct sigaction sa;
	int s;

	// Testa parâmetros de linha de comando. O usuário deve especificar
	// o número de segundos e nanosegundos desejados.
	if (argc != 3 || strcmp(argv[1], "--help") == 0){
		printf("%s secs nanosecs\n", argv[0]);
		exit(-1);
	}
	request.tv_sec = getLong(argv[1], 0, "secs");
	request.tv_nsec = getLong(argv[2], 0, "nanosecs");

	// Define um signal handler para SIGINT (CTRL-C)
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = sigintHandler;
	if (sigaction(SIGINT, &sa, NULL) == -1) {
	   printf("Erro na chamada a sigaction() - codigo do erro = %d\n", errno);
	   exit(-1);
	}

	// Obtém a hora corrente e a deposita na estrutura "start"
	//if (gettimeofday(&start, NULL) == -1) {
	if (clock_gettime(CLOCK_REALTIME, &start) == -1) {
	   printf ("Erro na chamada a clock_gettime() [1] - codigo do erro = %d\n", errno);
	   exit (-1);
	}

	// Laço de utilizção da função nanosleep()
	for (;;) {
		s = nanosleep(&request, &remain);
		if (s == -1 && errno != EINTR) {
			printf ("Erro na chamada a nanosleep - codigo do erro = %d\n", errno);
			exit (-1);
		}
		// obtém a hora corrente e a deposita na estrutura "finish"
		//if (gettimeofday(&finish, NULL) == -1) {
	    if (clock_gettime(CLOCK_REALTIME, &finish) == -1) {
			printf("Erro na chamada a clock_gettime() [2] - codigo do erro = %d\n", errno);
			exit(-1);
		}
		printf("Dormiu por %9.6f segundos\n", finish.tv_sec - start.tv_sec +
		       (finish.tv_nsec - start.tv_nsec) / 1000000000.0);
		if (s == 0)
		  break; /* nanosleep() completou */
		printf("Tempo restante: %2ld.%09ld\n", (long) remain.tv_sec,
		       remain.tv_nsec);
		request = remain; /* A próxima "dormida" será com o restante do tempo */
	}
	printf("Sleep completo\n");
	exit(EXIT_SUCCESS);
}