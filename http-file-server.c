#include <sys/socket.h>
#include <pthread.h>
#include <stdlib.h>

#define ERROR -1

int main(int argc, char const *argv[]) {
    int server_sock, on = 1;
    int status;

    /******************************************************/
    /* Cria um socket TCP (SOCK_STREAM).                  */
    /******************************************************/

    server_sock = socket(AF_INET, SOCK_STREAM, 0);

    if (server_sock == ERROR) {
        perror("[-] Failed to create a socket.\n");
        exit(EXIT_FAILURE);
    }

    printf("[+] TCP socket created successfully.\n");


    /******************************************************/
    /* Permite que o socket descriptor seja reutilizado.  */
    /******************************************************/

    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on))


    return 0;
}
