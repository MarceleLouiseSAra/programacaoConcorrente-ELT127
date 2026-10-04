#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

int main()
{
    pid_t pid;
    unsigned int tempo;

    printf("PID do processo: ");
    scanf("%d", &pid);

    printf("Tempo de bloqueio (segundos): ");
    scanf("%u", &tempo);

    kill(pid, SIGSTOP);

    sleep(tempo);

    kill(pid, SIGCONT);

    return 0;
}