#include <iostream>
#include <string>
#ifdef __WIN32__
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/un.h>

#define NUMBER_OF_REQUEST   1
#define PORT                1999

#define handle_error(msg) \
           do { perror(msg); exit(EXIT_FAILURE); } while (0)

void sendRequest(char* host, int port)
{
    char buffer[1024] = {0};
    struct sockaddr_in server_addr;
    char* msg;
    int status, varlend, client_fd;

    sprintf(msg, "GET /hello.html HTTP/1.1\nHost: %s:%d\nAccept-Language: en, vi\n\nasdasdsa", host, port);
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0)
        handle_error("create");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, host, &server_addr.sin_addr) <= 0)
        handle_error("converting");
    status = connect(client_fd, (sockaddr*) &server_addr, sizeof(server_addr));
    if (status < 0)
        handle_error("connect");
    send(client_fd, msg, strlen(msg), 0);
    varlend = read(client_fd, buffer, 1024);
    printf("message from server: %s.\n", buffer);
}

int main(int argc, char** argv)
{
    int idx;
    char* host = argv[1];
    int port = atoi(argv[2]);
    for (idx = 0; idx < NUMBER_OF_REQUEST; idx ++)
    {
        // send request asynchronously
        sendRequest(host, port);
    }

    return 0;
}
