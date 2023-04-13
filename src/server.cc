#include <iostream>
#include <string>
#include <stdio.h>

#ifdef __WIN32__
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/un.h>

#define SOCKET_PORT     1999

#define handle_error(msg) \
           do { perror(msg); exit(EXIT_FAILURE); } while (0)


#define SOCKET_PATH "/tmp/my.sock"


char* getIP()
{
    char hostname[1024];
    hostname[1023] = '\0';
    gethostname(hostname, 1023);
    // printf("Hostname: %s\n", hostname);
    struct hostent* host_entry;
    host_entry = gethostbyname(hostname);
    // printf("h_name: %s\n", host_entry->h_name);
    char* IP = inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0])); //Convert into IP string
    char* IP_cp = (char*) malloc(30);
    memcpy(IP_cp, IP, strlen(IP));
    return IP_cp;
}

char* getHTTPResponse()
{
    return "HTTP/1.1 200 OK\nServer: Hello\nContent-Length: 13\nContent-Type: text/plain\n\nHello, world";
}


void openSocket()
{
    int connection_fd, varlend;
    struct sockaddr_in servaddr;
    socklen_t len;
    int opt = 1;
    int server_fd;
    char buffer[1024] = {0};

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    len = sizeof(servaddr);

    if (server_fd == -1)
        handle_error("SOCKET ERROR\n");

    // Forcefully attaching socket to the port SOCKET_PORT
    if (setsockopt(server_fd, SOL_SOCKET,
                //    SO_REUSEADDR | SO_REUSEPORT, &opt,
                   SO_REUSEADDR, &opt,
                   sizeof(opt)))
        handle_error("setsockopt");

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(SOCKET_PORT);

    if (bind(server_fd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1)
        handle_error("bind");

    if (listen(server_fd, 10) == -1)
        handle_error("listen");

    printf("IP IS '%s', socket fd is %d, len is %d.\n", getIP(), server_fd, len);

    while(1)
    {
        connection_fd = accept(server_fd, (struct sockaddr *) &servaddr, &len);
        if (connection_fd == -1)
            handle_error("accept");

        varlend = read(connection_fd, buffer, 1024);
        printf("RECEIVE: %s\n", buffer);
        printf("LENGTH: %ld\n", strlen(buffer));
        char* ret;
        ret = getHTTPResponse();
        send(connection_fd, ret, strlen(ret), 0);
        unlink(SOCKET_PATH);
        close(connection_fd);
    }
    exit(EXIT_SUCCESS);
}

int main()
{
    char* IP = getIP();
    openSocket();
    return 0;
}