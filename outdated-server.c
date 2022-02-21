#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <errno.h>
#include <poll.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define guard(result, msg, sockfd) \
    if (result == -1)              \
    {                              \
        perror(msg);               \
        if (sockfd != 0)           \
            close(sockfd);         \
        exit(EXIT_FAILURE);        \
    }

#define SERVER_PORT 80
#define LISTEN_BACKLOG 12

#define TIME_IN_MILLISECONDS 3 * 60 * 1000
#define BUFFER_SIZE 2048

#define FALSE 0
#define TRUE 1

int POLL_SIZE = 200;

char *responseWrite(int code, int length)
{
    char *response = (char *)calloc(BUFFER_SIZE, sizeof(char));
    if (code == 404)
    {
        sprintf(response, "%s\r\n%s\r\n\n", "HTTP/1.0 404 Not Found", "Connection: Closed");
    }
    else if (code == 200)
    {
        sprintf(response, "%s\r\n%s%d\r\n%s\r\n\n", 
        "HTTP/1.1 200 OK", 
        "Content-Length: ", length, 
        "Connection: Closed");
    }

    return response;
}

void getFilename(char buffer[], char data[])
{
    char line[100];
    int start = strcspn(buffer, "/");
    int limit = strcspn(buffer, "\r");
    int j = 0;
    for (int i = start + 1; i < limit - 9; i++)
    {
        line[j++] = buffer[i];
    }
    line[j] = '\0';

    strcat(data, line);
}

void setHttpResponse(char *filename, int sockfd)
{
    char buffer[BUFFER_SIZE];
    char *headers = NULL;
    FILE *pf;
    unsigned long fsize;

    pf = fopen(filename, "rb");
    if (pf == NULL)
    {
        perror("[-] File not found.\n");
        headers = responseWrite(404, 0);
        printf("%s", headers);
        int bytes_written = send(sockfd, headers, strlen(headers), 0);
        guard(bytes_written <= 0 ? -1 : 0, "[-] ERROR writing to socket\n", 0);
        return;
    }

    printf("[+] Found file %s\n", filename);

    fseek(pf, 0, SEEK_END);
    fsize = ftell(pf);
    rewind(pf);

    printf("[+] File contains %ld bytes!\n", fsize);
    printf("[+] Sending the file now\n");

    headers = responseWrite(200, fsize);
    printf("%s", headers);
    int bytes_written = send(sockfd, headers, strlen(headers), 0);
    guard(bytes_written <= 0 ? -1 : 0, "[-] ERROR writing to socket\n", 0);

    while (1)
    {
        int bytes_read = fread(buffer, sizeof(char), sizeof(buffer), pf);
        if (bytes_read == 0) // We're done reading from the file
            break;

        guard(bytes_read < 0 ? -1 : 0, "[-] ERROR reading from file\n", 0);

        void *p = buffer;
        while (bytes_read > 0)
        {
            int bytes_written = send(sockfd, buffer, bytes_read, 0);
            guard(bytes_written <= 0 ? -1 : 0, "[-] ERROR writing to socket\n", 0);

            bytes_read -= bytes_written;
            p += bytes_written;
        }
    }

    printf("[+] Done Sending the File!\n");
    printf("[+] Now Closing Connection.\n");

    fclose(pf);
    printf("[+] Successful sent data.\n");
}

struct pollfd resetClientFd(struct pollfd fd)
{
    close(fd.fd);
    fd.fd = 0;
    fd.events = 0;
    fd.revents = 0;
}

int main(int argc, char *argv[])
{
    int i, data_length, res, on = 1;
    int server_sockfd, client_sockfd;
    struct sockaddr_in server_addr;
    int nfds = 1, current_size = 0;
    int end_server = FALSE, close_conn;
    char buffer[BUFFER_SIZE];

    struct pollfd *poll_fds = malloc(sizeof(struct pollfd) * POLL_SIZE);
    guard(poll_fds == NULL ? -1 : 0, "[-] Failed to allocate memory for struct poll.\n", 0);

    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    guard(server_sockfd, "[-] Failed to create a socket.\n", 0);

    printf("[+] TCP socket created successfully.\n");

    guard(setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)),
          "[-] Failed to set socket options.\n",
          server_sockfd);

    printf("[+] Socket descriptor allowed to be reused.\n");

    guard(ioctl(server_sockfd, FIONBIO, (char *)&on),
          "[-] Failed to set socket to be nonblocking.\n",
          server_sockfd);

    printf("[+] Socket descriptor set to be nonblocking.\n");

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);
    guard(bind(server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)),
          "[-] Failed to bind the socket to the address and port number.\n",
          server_sockfd);

    printf("[+] Success to bind the socket to the address and port number.\n");

    guard(listen(server_sockfd, LISTEN_BACKLOG),
          "[-] Failed to listen requests.\n",
          server_sockfd);

    printf("[+] Listening for connections...\n");

    memset(poll_fds, 0, sizeof(poll_fds));
    poll_fds[0].fd = server_sockfd;
    poll_fds[0].events = POLLIN;

    do
    {
        res = poll(poll_fds, nfds, TIME_IN_MILLISECONDS);
        guard(res, "[-] Poll failed.\n", server_sockfd);

        printf("[+] Waiting on poll...\n");

        if (res == 0)
        {
            printf("[-] The poll timeout expired.\n");
            break;
        }

        current_size = nfds;
        for (i = 0; i < current_size; i++)
        {
            if (poll_fds[i].revents == 0 || poll_fds[i].fd == -1)
                continue;

            if (poll_fds[i].revents != POLLIN)
            {
                printf("[*] Unexpected result.");
                end_server = TRUE;
                break;
            }

            if (poll_fds[i].fd == server_sockfd)
            {
                printf("[+] Listening socket is readable.\n");

                do
                {
                    if ((client_sockfd = accept(server_sockfd, NULL, NULL)) == -1)
                    {
                        if (errno != EWOULDBLOCK)
                        {
                            perror("[-] Failed to accept connection request.\n");
                            end_server = TRUE;
                        }
                        break;
                    }

                    if (nfds >= POLL_SIZE)
                    {
                        POLL_SIZE = POLL_SIZE * 2;
                        poll_fds = realloc(poll_fds, sizeof(struct pollfd) * POLL_SIZE);
                        if (poll_fds == NULL)
                        {
                            perror("[-] Failed to increase poll memory.");
                            end_server = TRUE;
                            break;
                        }
                    }

                    printf("[+] New incoming connection - %d\n", client_sockfd);
                    poll_fds[nfds].fd = client_sockfd;
                    poll_fds[nfds].events = POLLIN;
                    nfds++;

                } while (client_sockfd != -1);
            }
            else
            {
                printf("[+] Descriptor %d is readable\n", poll_fds[i].fd);

                data_length = recv(poll_fds[i].fd, buffer, sizeof(buffer), 0);
                if (data_length == -1)
                {
                    if (errno != EWOULDBLOCK)
                    {
                        perror("[-] Failed to recieve data.\n");
                        poll_fds[i] = resetClientFd(poll_fds[i]);
                        nfds--;
                    }
                    break;
                }

                if (data_length == 0)
                {
                    printf("[+] Client closed connection.\n");
                    poll_fds[i] = resetClientFd(poll_fds[i]);
                    nfds--;
                    break;
                }

                printf("[+] Successful received data.\n");

                char filename[BUFFER_SIZE] = {'\0'};
                getFilename(buffer, filename);
                setHttpResponse(filename, poll_fds[i].fd);

                close(poll_fds[i].fd);
                poll_fds[i].fd = -1;
            }
        }
    } while (end_server == FALSE);

    printf("[*] Ending server.\n");

    for (i = 0; i < nfds; i++)
    {
        if (poll_fds[i].fd >= 0)
            close(poll_fds[i].fd);
    }

    free(poll_fds);
    return 0;
}