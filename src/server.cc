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

#include "server.h"
#include <fcntl.h>

namespace my_http_server
{
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

    std::string* getTmpHTTPResponse()
    {
        std::string *msg = new std::string("HTTP/1.1 200 OK\nServer: Hello\nContent-Length: 13\nContent-Type: text/plain\n\nHello, world\n");
        return msg;
    };

    int HttpServer::init_and_bind(int port)
    {
        struct addrinfo hints, *result, *rp;
        int s;
        int socket_fd;

        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;
        s = getaddrinfo (NULL, std::to_string(port).c_str(), &hints, &result);
        if (s != 0)
            handle_error("getaddrinfo");
        for (rp = result; rp != NULL; rp = rp->ai_next)
        {
            socket_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
            if (socket_fd < 0)
                continue;
            s = bind(socket_fd, rp->ai_addr, rp->ai_addrlen);
            if (s == 0) // OK
            {
                break;  
            }
            else
            {
                /* temporary skip to next socket_fd */   
                // handle_error("bind");
            }
        }
        if (rp == NULL)
        {
            freeaddrinfo(result);
            return -1;
        }
        freeaddrinfo(result);
        return socket_fd;
    }

    int HttpServer::setnonblocking(int socket_fd) { // https://stackoverflow.com/questions/27266346/how-to-set-file-descriptor-non-blocking
        int opt;

        opt = fcntl(socket_fd, F_GETFD);
        if (opt < 0) {
            printf("fcntl(F_GETFD) fail.");
            return -1;
        }
        opt |= O_NONBLOCK;
        if (fcntl(socket_fd, F_SETFD, opt) < 0) {
            printf("fcntl(F_SETFD) fail.");
            return -1;
        }
        return 0;
    }
    int HttpServer::setupServer(struct sockaddr_in& servaddr){
        int server_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        int opt = 1;

        if (server_fd == -1)
            handle_error("open\n");

        // Forcefully attaching socket to the port SOCKET_PORT
        if (setsockopt(server_fd, SOL_SOCKET,
                    //    SO_REUSEADDR | SO_REUSEPORT, &opt,
                    SO_REUSEADDR, &opt,
                    sizeof(opt)))
            handle_error("setsockopt");


        if (bind(server_fd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1)
            handle_error("bind");

        if (listen(server_fd, 10) == -1)
            handle_error("listen");
        return server_fd;
    }
    
    void HttpServer::handleConnection(int socket_fd, struct epoll_event& event, int epollfd)
    {
        // while(1)
        // {
            struct sockaddr in_addr;
            socklen_t in_len;
            int infd, s;
            char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

            in_len = sizeof(in_addr);
            infd = accept(socket_fd, &in_addr, &in_len);
            if (infd == -1) // error
            {
                if ((errno == EAGAIN) ||
                    (errno == EWOULDBLOCK))
                {
                    /* We have processed all incoming
                        connections. */
                    // break;
                    return;
                }
                else
                {
                    perror ("accept");
                    // break;
                    return;
                }
            }
            /* Get information of client */
            s = getnameinfo(&in_addr, in_len, hbuf, NI_MAXHOST, sbuf, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);
            if (s < 0)
                handle_error("getnameinfo");
            else if (s == 0){};
                // printf("Accept connection on decriptor %d, from %s:%s\n", infd, hbuf, sbuf);
            s = setnonblocking(infd);
            if (s == -1)
                handle_error("setnonblocking new connection");
            
            event.data.fd = infd;
            event.events = EPOLLIN | EPOLLET;
            s = epoll_ctl (epollfd, EPOLL_CTL_ADD, infd, &event);
            if (s < 0)
                handle_error("epoll_ctl new connection");
        // }
    }

    void HttpServer::handleData(struct epoll_event event)
    {
        bool done = false;
        while(!done)
        {
            int size;
            char buffer[1024] = {0};

            size = read(event.data.fd, buffer, 1024); // stuck after reading => keep waiting for request.
            if (size == -1) // 0 means done, -1 means error
            {
                if (errno != EAGAIN)
                {
                    done = true;
                    /* we just read all data, no more data to read */
                }
                break;
            }
            else if (size == 0)
            {
                done = 1;
                break;
            }
            else
            {
                // printf("Receive: %s\n.", buffer);
                // usleep(1e6 / MAXEVENTS); // max 10req / sec
                std::string *res = getTmpHTTPResponse();
                write(event.data.fd, res->c_str(), res->size());
            }
            
        }
        /* Close conenction after reading */
        close(event.data.fd);
    }

    void HttpServer::openSocket()
    {
        int connection_fd, varlend;
        struct epoll_event event;
        // struct epoll_event *events;
        struct sockaddr servaddr;
        int listener_fd, s;
        // std::string buffer[1024] = {0};
        // memset(buffer, 0, 1024);

        listener_fd = init_and_bind(getPort());
        // listener_fd = setupServer(servaddr);
        int len = sizeof(servaddr);

        s = setnonblocking(listener_fd);
        if (s == -1)
            handle_error("setnonblocking");

        s = listen(listener_fd, MAX_CONCURRENT_REQ);
        if (s == -1)
            handle_error("lister");

        int  epollfd = epoll_create1(0);
        if (epollfd < 0)
            handle_error("epoll create");
        event.data.fd = listener_fd;
        event.events = EPOLLIN | EPOLLET;
        s = epoll_ctl(epollfd, EPOLL_CTL_ADD, listener_fd, &event);
        if (s < 0)
            handle_error("epoll_ctl");

        struct epoll_event ev, events[MAXEVENTS];
        

          /* The event loop */
        while (1)
        {
            int n, idx;

            n = epoll_wait (epollfd, events, MAXEVENTS, -1);
            for (idx = 0; idx < n; idx ++)
            {
                if ((events[idx].events & EPOLLERR) ||
                    (events[idx].events & EPOLLHUP) ||
                    (!(events[idx].events & EPOLLIN)))
                {
                    // erro happens
                    printf("ERROR HAPPENED\n");
                }
	            else if (listener_fd == events[idx].data.fd)
                {
                    handleConnection(listener_fd, event, epollfd);
                }
                else
                {
                    handleData(events[idx]);
                }
            }
        }
    };
}
