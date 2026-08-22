#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     // fork, execvp
#include <sys/types.h>  // pid_t
#include <sys/wait.h>   // wait
#include <errno.h>      // errno
#include <pthread.h>
#include <stdint.h>     // intptr_t (para conversão segura de ponteiros)

void* TestFunc(void* arg) {
    int i = 0, index = 0;

    index = (int)(intptr_t) arg; // parsing

    for (i = 0; i < 100; i++) {
        printf("%d", index);
        sleep(0.1);
    }

    printf("\n");

    pthread_exit((void *)(intptr_t) index);
    return ((void *)(intptr_t) index);
}

int flag[3] = {1, 1, 1};

int main() {
    pthread_t hThreads[3];
    int status = 0;
    
    for (int i = 0; i < 3; i++) {
        status = pthread_create(
            &hThreads[i], 
            NULL, 
            TestFunc, 
            (void *)(intptr_t) i
        );
        
        if (!status) {
            printf("Thread %d criada com ID = %ld\n", i, (unsigned long) hThreads[i]);
        } else {
            printf("Erro ao criar a thread %d! Codigo = %d\n", i, status);
        }
    }
    
    int i = 0;
    // while((flag[0] + flag[1] + flag[2]) > 0) {
        //     if (flag[i] == 1) {
    //         status = pthread_cancel(hThreads[i]);
    //         if (status == ESRCH) {
        //             printf("Thread %d encerrada (ID = %d)\n", i, (int) &hThreads[i]);
        //             flag[i] = 0;
        //         }
        //     }
        //     i = (i + 1) % 3;
        // }
    void *tRetStatus;
    for (i = 0; i < 3; i++) {
        printf("Aguardando o término da thread %ld... \n", (unsigned long) &hThreads[i]);
        pthread_join(
            hThreads[i],
            &tRetStatus
        );
        printf("Thread %d: status de retorno %d\n", i, (int)(intptr_t) tRetStatus);
    }

    // return (0);
    pthread_exit(NULL);
}