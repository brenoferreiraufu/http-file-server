#include <sys/socket.h> // socket API
#include <pthread.h>    // thread API
#include <stdlib.h>     // EXIT_FAILURE
#include <netinet/in.h> // struct sockaddr_in
#include <string.h>     // memset()
#include <stdio.h>      // printf()
#include <unistd.h>     // close()
#include <signal.h>     // signal()

#define ERROR -1
#define SUCCESS 0
#define TRUE 1
#define FALSE 0

#define PORT 80
#define LISTEN_BACKLOG 4096
#define REQUEST_BUFFER_SIZE 8192
#define FILENAME_SIZE 256
#define DATA_LENGTH 65535 * 32
#define NUMBER_THREADS 32

#define HTTP_STATUS_404 "HTTP/1.0 404 Not Found\r\nServer: Cervidor-0.2\r\nConnection: Closed\r\n\n"
#define HTTP_STATUS_200 "HTTP/1.0 200 OK\r\nServer: Cervidor-0.2\r\nConnection: Closed\r\n\n"

int status, server_sock;

void guard(char *fail, char *success)
{
    if (status == ERROR)
    {
        perror(fail);
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf(success);
}

void *connection_handler(void *arg)
{
    char buffer[REQUEST_BUFFER_SIZE] = {'\0'};
    char filename[FILENAME_SIZE] = {'\0'};
    char rbuffer[DATA_LENGTH];
    int client_sock;
    int data_length, bytes_written;
    int file_size, bytes_read;
    long thread_n = (long) arg + 1;
    FILE *file;

    do
    {
        /******************************************************/
        /* Aguarda até que uma nova conexão seja aceita.      */
        /******************************************************/

        client_sock = accept(server_sock, NULL, NULL);

        if (client_sock == ERROR)
        {
            perror("[-] Failed to accept connection request.\n");
            close(server_sock);
            exit(EXIT_FAILURE);
        }

        printf("[+] Thread %ld: Successful connection.\n", thread_n);

        /******************************************************/
        /* Recebe os dados da requisição e guarda no buffer.  */
        /******************************************************/

        data_length = recv(client_sock, buffer, sizeof(buffer), 0);

        if (data_length == ERROR)
        {
            perror("Failed to recieve data.\n");
            close(client_sock);
            continue;
        }

        if (data_length == 0)
        {
            printf("[+] Thread %ld: Closed connection.\n", thread_n);
            close(client_sock);
            continue;
        }

        printf("[+] Thread %ld: Successful received data.\n", thread_n);

        /******************************************************/
        /* Lê a requisição para pegar o nome do arquivo.      */
        /******************************************************/

        int start = strcspn(buffer, "/") + 1;
        int end = strcspn(buffer, "\r") - 14;
        strncpy(filename, buffer + start, end);

        if (strlen(filename) == 0)
            strcat(filename, "index.html");

        /******************************************************/
        /* Tenta abrir o arquivo para ler os dados.           */
        /******************************************************/

        // printf("[*] Client %d: Trying to open file.\n", client_sock);
        file = fopen(filename, "rb");

        if (file == NULL)
        {
            perror("[-] File not found.\n");
            bytes_written = send(client_sock, HTTP_STATUS_404, strlen(HTTP_STATUS_404), 0);

            printf("[*] Thread %ld: Sending not found response.\n", thread_n);
            if (bytes_written == ERROR)
            {
                perror("[-] Failed to send headers.\n");
                close(client_sock);
                continue;
            }

            close(client_sock);
            continue;
        }

        printf("[+] Thread %ld: File %s was found.\n", thread_n, filename);

        /******************************************************/
        /* Envia o arquivo para o cliente.                    */
        /******************************************************/

        bytes_written = send(client_sock, HTTP_STATUS_200, strlen(HTTP_STATUS_200), 0);

        if (bytes_written == ERROR)
        {
            perror("[-] Failed to send headers.\n");
            fclose(file);
            close(client_sock);
            continue;
        }

        do
        {
            bytes_read = fread(rbuffer, sizeof(char), DATA_LENGTH, file);

            if (ferror(file))
            {
                perror("[-] ERROR reading from file\n");
                fclose(file);
                close(client_sock);
                pthread_exit(NULL);
            }

            if (bytes_read == 0)
            {
                fclose(file);
                break;
            }

            bytes_written = send(client_sock, rbuffer, bytes_read, 0);

            if (bytes_written == ERROR)
            {
                perror("[-] Failed to send data.\n");
                fclose(file);
                close(client_sock);
                pthread_exit(NULL);
            }

        } while (TRUE);

        //printf("[+] Thread %ld: Successful sent data.\n", thread_n);

        close(client_sock);

    } while (TRUE);

    pthread_exit(NULL);
}

int main(int argc, char const *argv[])
{
    int on = 1;
    int status, addr_size;
    struct sockaddr_in server_addr;
    pthread_t thread_id[NUMBER_THREADS];
    long itr = 0;

    /******************************************************/
    /* Cria um socket TCP (SOCK_STREAM).                  */
    /******************************************************/

    server_sock = socket(AF_INET, SOCK_STREAM, 0);

    if (server_sock == ERROR)
    {
        perror("[-] Failed to create a socket.\n");
        exit(EXIT_FAILURE);
    }

    printf("[+] TCP socket created successfully.\n");

    /******************************************************/
    /* Permite que o socket descriptor seja reutilizado.  */
    /******************************************************/

    status = setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
    guard("[-] Failed to set socket options.\n",
          "[+] Socket descriptor allowed to be reused.\n");

    /******************************************************/
    /* Configurando o socket com I/O não bloqueante.      */
    /******************************************************/

    // status = ioctl(server_sock, FIONBIO, (char *)&on);
    // guard("[-] Failed to set socket to be nonblocking.\n",
    //     "[+] Socket descriptor set to be nonblocking.\n");

    /******************************************************/
    /* Inicializando estrutura do socket address.         */
    /******************************************************/

    addr_size = sizeof(server_addr);
    memset(&server_addr, 0, addr_size);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    /******************************************************/
    /* Bind do socket com o endereço em server_addr.      */
    /******************************************************/

    status = bind(server_sock, (struct sockaddr *)&server_addr, addr_size);
    guard("[-] Failed to bind the socket to the address and port number.\n",
          "[+] Success to bind the socket to the address and port number.\n");

    /******************************************************/
    /* Passa a escutar conexões com o socket.             */
    /******************************************************/

    status = listen(server_sock, LISTEN_BACKLOG);
    guard("[-] Failed to listen requests.\n", "[+] Listening...\n");

    /******************************************************/
    /* Loop infinito enquanto permite conexões.           */
    /******************************************************/

    do
    {
        /******************************************************/
        /* Cria uma nova thread para lidar com a requisição.  */
        /******************************************************/

        status = pthread_create(&thread_id[itr], NULL, connection_handler, (void *)itr);

        if (status != SUCCESS)
        {
            perror("[-] Failed to create a new pthread.\n");
            close(server_sock);
            exit(EXIT_FAILURE);
        }

        /******************************************************/
        /* Para desalocar a memória da thread quando rotina   */
        /* finalizar usa a função pthread_detach.             */
        /******************************************************/

        status = pthread_detach(thread_id[itr]);

        if (status != SUCCESS)
        {
            perror("[-] Failed to detach thread.\n");
            close(server_sock);
            exit(EXIT_FAILURE);
        }

        itr++;
        // printf("[+] New detached thread created for connection %d.\n", client_sock);
    } while (itr < NUMBER_THREADS);

    while (TRUE)
        ;

    return 0;
}
