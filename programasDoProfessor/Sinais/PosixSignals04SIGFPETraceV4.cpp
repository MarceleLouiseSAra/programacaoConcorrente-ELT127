//**************************************************************************************
//	Programa de exemplo de "signals" POSIX: Divisão por zero
//
//	Versão: 1.0	  Data: 04/04/2020
//                      Testado no Ubuntu 20.04 sob WSL 1
// 
//          1.1   Data: 29/04/2022
//                      Testado no Ubuntu 20.04 sob WSL 2
// 
//                      Corrigido o procedimento de extração dos endereços-base
//                      do programa executável. Acrescentado este mesmo
//                      procedimento porém quanto à biblioteca "libc"
//                      (libc-2.31.so)
//
//  Autor: Prof. Luiz T. S. Mendes (DELT/EE-UFMG)
//
//  ATENÇÃO: 1) Utilizar a opção "-g" no GCC (geração de informações de depuração)
//              para que a recuperação de símbolos via comando "addr2line" funcione
//              corretamente.
//           2) O Linux kernel emprega a proteção de ASLR (Address Space Layout
//              Randomization), e portanto com ela os endereços de "stack trace" não
//              serão válidos. Assim, é necessário desabilitar (para fins de teste)
//              esta proteção, usando os comandos:
//
//                cat /proc/sys/kernel/randomize_va_space   [verificar ASLR]
//                   0 - No randomization. Everything is static.
//                   1 – Conservative randomization. Shared libraries, stack, mmap(),
//                       VDSO and heap are randomized.
//                   2 – Full randomization 
//
//                sudo sysctl kernel.randomize_va_space=0   [desabilita ASLR]
//                sudo sysctl kernel.randomize_va_space=2   [habilita ASLR]
//
//           3) Para verificar o mapeamento de memória de um processo, usar
//
//                pmap <PID>
//****************************************************************************************

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include "conio.h"      // kbhit() e _getch()
#include <ucontext.h>
#include <execinfo.h>

#define  ESC 0x1B

// Protótipos das funções empregadas
void SignalHandlerDiv0(int signo, siginfo_t *si, void *data);
void SignalHandlerCTRLC (int signo);
void posix_print_stack_trace();
int addr2line(char const * const program_name, void const * const addr);

//====================================================================
// Thread primária
//
// Executa as seguintes ações:
// 1. Define um "signal handler" para SIGFPE
// 2. Executa um "loop" de leitura do teclado e, se a tecla "0" for
//    digitada, provoca uma divisão por zero.
// 3. Se CTRL-C for digitado, é interceptado e descartado.
// 4. Se ESC for digitado, encerra o programa normalmente.
// 
//====================================================================
char nomeprog[256];

int main(int argc, char** argv)
{
	//char buff[1];
	sigset_t sigset;            // Variável para definir máscara de sinais a bloquear
    struct sigaction sa_div0;   // Estrutura necessária para "signal handler" de divisão por zero
    struct sigaction sa_kbd;    // Estrutura necessária para "signal handler" de CTRL-C
	int status;                 // Status de retorno das funções chamadas
    int signum;                 // Numero do sinal
	int div = 0, tecla, n = 1;

	strncpy(nomeprog, argv[0], strlen(argv[0]));

	// Define handler para SIGFPE
	memset(&sa_div0, 0, sizeof(sa_kbd)); //IMPORTANTE - SA_KBD PODE CONTER LIXO E ISTO
	                                     // PROVOCAR MAU FUNCIONAMENTO
	sa_div0.sa_sigaction = SignalHandlerDiv0;
	sa_div0.sa_flags = SA_SIGINFO;
	status = sigaction(SIGFPE, &sa_div0, NULL);
	if (status != 0) {
		printf("Erro em sigaction: valor = %d\n", errno);
		exit (-1);
	}

 	// Define handler para SIGINT
	memset(&sa_kbd, 0, sizeof(sa_kbd)); //IMPORTANTE - SA_KBD PODE CONTER LIXO E ISTO
	                                    // PROVOCAR MAU FUNCIONAMENTO
	sa_kbd.sa_handler = SignalHandlerCTRLC;
	status = sigaction(SIGINT, &sa_kbd, NULL);
	if (status != 0) {
		printf("Erro em sigaction: valor = %d\n", errno);
		exit (-1);
	}

	// Laço de tratamento do teclado
	do {
		printf("\nDigite uma tecla qualquer:");
		if (kbhit()) {
			tecla = _getch();
			if (tecla == '0') {
				printf ("Executando divisao de 1 por zero ... %d\n", n/div);
			}
		}
		sleep (1);
	} while (tecla != ESC);

	//printf("\nAcione uma tecla para terminar\n");
	//_getch();

	return EXIT_SUCCESS;

}  // main

//====================================================================
// "Signal handler" para SIGFPE
//====================================================================

void SignalHandlerDiv0(int signo, siginfo_t *si, void *data) {
	ucontext_t *uc;
	uc = (ucontext_t *) data;

	if (signo == SIGFPE){
	   printf("\nSIGNAL HANDLER: Capturado sinal %d [SIGFPE]\n", signo);
	   printf("Endereco da falha = %p, 'Instruction Pointer' = %p\n",
		   si->si_addr, (void *)uc->uc_mcontext.gregs[REG_RIP]);
	}
	else
      printf("\nSIGNAL HANDLER: Capturado sinal %d", signo);
	posix_print_stack_trace();
    exit(0);
}

//====================================================================
// "Signal handler" para CTRL-C
//====================================================================

void SignalHandlerCTRLC(int signo) {
	if (signo == SIGINT)
      printf("\nSIGNAL HANDLER: Capturado sinal %d [CTRL-C]", signo);
	else
      printf("\nSIGNAL HANDLER: Capturado sinal %d", signo);
}

//========================================================================
// Rotina para imprimir "stack trace" e identificar as linhas
// de código correspondentes
//
// Adaptada de 
//     https://spin.atomicobject.com/2013/01/13/exceptions-stack-traces-c
// 
//========================================================================

void posix_print_stack_trace()
{
  #include <stdint.h>

  #define MAX_STACK_FRAMES 64

  #define RED         "\033[0;31m"
  #define BOLDRED     "\033[1;31m"
  #define GREEN       "\033[0;32m"
  #define BOLDGREEN   "\033[1;32m"
  #define YELLOW      "\033[0;33m"
  #define BOLDYELLOW  "\033[1;33m"
  #define BLUE        "\033[0;34m"
  #define BOLDBLUE    "\033[1;34m"
  #define MAGENTA     "\033[0;35m"
  #define BOLDMAGENTA "\033[1;35m"
  #define CYAN	      "\033[0;36m"
  #define BOLDCYAN    "\033[1;36m"
  #define RESET       "\033[0m"
  
  int i, trace_size = 0;
  char **messages = (char **)NULL;
  static void *stack_traces[MAX_STACK_FRAMES];
  void *stack_addr_prog, *stack_addr_lib;
  uintptr_t temp;
  char cmdprog[256], cmdlib[256], buf[17], * ptr;
  char cmdaux[] = " | grep \"r----\" | grep -m 1 ";
  FILE* cmd;
  long base_address_prog, base_address_lib;
  int status;

  // Nesta rotina, os endereços passados ao comando 'addr2line' no laço a seguir
  // devem ser offsets (deslocamentos), e não os endereços virtuais alocados ao processo
  // pelo kernel quando de sua execução. Estes endereços virtuais, por sua vez, podem ser
  // consultados na estrutura /proc/<PID>/maps, sendo de interesse aqui os endereços com
  // permissão de execução (campo "perms" da forma 'r-xp'). Exemplo:
  //
  //     $ ps a | grep "[P]"osixSignalsDivZeroTrace  // truque para evitar que o "grep" apareça OU
  //     $ ps a | grep PosixSignalsDivZeroTrace | grep -v grep
  //     1766 tty2     S      0:00 PosixSignalsDivZeroTraceV2
  //
  //     $ cat /proc/1766/maps | grep "r-xp"
  //     (...)
  //     7f167c400000-7f167c401000 r-xp 00000000 00:00 65570  /mnt/c/Users/Luiz/[...]/PosixSignalsDivZeroTraceV2
  //     (...)
  //     
  //     https://stackoverflow.com/questions/1401359/understanding-linux-proc-id-maps
  //
  //     ou, ainda, 'pmap <PID>':
  //
  //     $ pmap 1085 | grep "r-x"
  //     0000555555555000      4K r - x-- PosixSignals04SIGFPETraceV3
  //	   00007ffff7def000   1504K r - x-- libc - 2.31.so
  //	   00007ffff7fce000      4K r - x--[anon]
  //	   00007ffff7fd0000    140K r - x-- ld - 2.31.so
  //
  // Portanto, se p. ex. o endereço da instrução de falha retornada no campo "si_addr" da
  // estrutura "sigaction" for  0x555555555910, devemos subtrair o endereço-base virtual
  // 0000555555555000 para obter o "offset" da instrução de falha. Para tal vamos executar
  // o comando "pmap" e extrair o endereço-base de sua saída.
  //

  // Monta linha de comando 'pmap <PID> | grep "r----" | grep -m 1 <nome do programa>'
  snprintf(cmdprog, sizeof(cmdprog), "pmap %d", getpid());
  strncat(cmdprog, cmdaux, strlen(cmdaux));
  strncpy(cmdlib, cmdprog, sizeof(cmdprog));
  strncat(cmdprog, nomeprog, strlen(nomeprog));
  //printf("cmdprog = %s\n", cmdprog);

  // Executa o comando e extrai o endereço base de seu resultado
  cmd = popen(cmdprog, "r");
  if (cmd == NULL) {
	  printf("Erro em popen()!\n");
	  _exit(0);
  }
  fgets(buf, sizeof(buf), cmd);
  pclose(cmd);
  base_address_prog = strtol(buf, &ptr, 16);
  //printf("base address prog = %lx\n", base_address_prog);
  
  // Monta linha de comando 'pmap <PID> | grep "r----" | grep -m 1 libc'
  strncat(cmdlib, "libc", sizeof("libc"));
  //printf("cmdlib = %s\n", cmdlib);

  // Executa o comando e extrai o endereço base de seu resultado
  cmd = popen(cmdlib, "r");
  if (cmd == NULL) {
	  printf("Erro em popen()!\n");
	  _exit(0);
  }
  fgets(buf, sizeof(buf), cmd);
  pclose(cmd);
  base_address_lib = strtol(buf, &ptr, 16);
  //printf("base address lib = %lx\n", base_address_lib);

  trace_size = backtrace(stack_traces, MAX_STACK_FRAMES);
  messages = backtrace_symbols(stack_traces, trace_size);

  printf("\n'Stack trace' e linha correspondente no programa-fonte '%s':\n", nomeprog);

  for (i = 0; i < trace_size; ++i) {

    // A linha a seguir provocará "warning" no gcc referente ao uso de
    // apontadores "void *" em aritmética:
    // 	   stack_addr = stack_traces[i] - base_address_prog;
    // A solução é fazer um "cast" temporário para um tipo de dados compatível
    // com aritmética:
    // http://computer-programming-forum.com/47-c-language/c8a6c16a52df9154.htm
	stack_addr_prog = (void*)((char*)stack_traces[i] - base_address_prog);
	stack_addr_lib = (void*)((char*)stack_traces[i] - base_address_lib);

	//printf(RESET);
	//printf("\n\nstack_traces[i] %p stack_addr_prog %p stack_addr_lib %p\n", stack_traces[i], stack_addr_prog, stack_addr_lib);
	
	printf(RESET);
	printf("--------------------------------------------------------------------------------\n");
	printf(BOLDBLUE);
	printf("%s\n", messages[i]);
	printf(BOLDYELLOW);
	fflush(stdout); // Para forçar o "flush" da cor amarela (pois não há um "\n")
	// Verifica se o endereço contido no presente stack_trace corresponde ao
	// programa executável ou à biblioteca libc-2.31.so. De acordo com a saída
	// do comando "pmap <PID>", endereços menores que base_address_lib pertencem
	// ao programa principal, ao passo que os maiores ou iguais pertencem a libc-2.31.so
	if ((long) stack_traces[i] < base_address_lib)
		status = addr2line(nomeprog, stack_addr_prog);
	else
		// ATENÇÃO: Naturalmente, para o comando "addr2line" funcionar em uma biblioteca
		// qualquer, a mesma deve ter sido compilada com símbolos de depuração. Assim
		// temos de usar a versão "debug" de libc.2.31.so
		status = addr2line("/usr/lib/debug/lib/x86_64-linux-gnu/libc-2.31.so", stack_addr_lib);
	if (status != 0)
		printf("Erro na determinacao do numero de linha para %s\n", messages[i]);
  }// end for
  printf(RESET);
  if (messages)
	free(messages);
}// end posix_print_stack_frame

/* Resolve symbol name and source location given the path to the executable 
   and an address */
int addr2line(char const * const program_name, void const * const addr)
{
  char addr2line_cmd[512] = {0};
 
  /* have addr2line map the address to the relent line in the code */
  #ifdef __APPLE__
    /* apple does things differently... */
  #else
    sprintf(addr2line_cmd,"addr2line -f -p -e %.256s %p", program_name, addr);
  #endif
 
  /* This will print a nicely formatted string specifying the
     function and source line of the address */
  printf("\033[0;33m"); // Força cor amarela
  return system(addr2line_cmd);
}
