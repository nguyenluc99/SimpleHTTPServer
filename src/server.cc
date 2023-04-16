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

#include <ulimit.h>
#include <sys/resource.h>
#include "server.h"
#include <fcntl.h>
#include <ev.h>


namespace my_http_server
{

    ev_io stdin_watcher;
    ev_signal sig;
    // int sig = SIGINT;
    const char* sampleResponse = "HTTP/1.1 200 OK\nServer: Hello\nContent-Length: 13\nContent-Type: text/plain\n\nHello, world\n";
    const int sampleLength = 88;
    // const char* sampleResponse = "dummy\n";
    // const int sampleLength = 6;
    SharedThread    thread_infos[THREAD_POOL_SIZE];
    int             epoll_list[THREAD_POOL_SIZE];
    epoll_event worker_events_list[THREAD_POOL_SIZE][MAX_EVENTS];
    // pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    // pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
    // int sharedEpollfd = -1;
    // int socketList[NUM_SOCKET];
    // TSQueue<std::pair<int, int> > fd_queue; // first = epollfd, second = event_fd;

    // int countWorkingThread()
    // {
    //     int count = 0, idx;
    //     pthread_mutex_lock(&mutex);
    //     for (idx = 0; idx < THREAD_POOL_SIZE; idx++) 
    //         if (thread_infos[idx].state == THREAD_RUNNING)
    //             count ++;
    //     pthread_mutex_unlock(&mutex);
    //     return count;
    // }

    // void stdin_cb (EV_P_ ev_io *w, int revents)
    // {
    //     puts ("stdin ready");
    //     char buffer[100];
    //     int size = read(0,buffer, 100);
    //     puts("You enter\n");
    //     printf("%s", buffer);
    //     // for one-shot events, one must manually stop the watcher
    //     // with its corresponding stop function.
    //     ev_io_stop (EV_A_ w);
        
    //     // this causes all nested ev_run's to stop iterating
    //     ev_break (EV_A_ EVBREAK_ALL);
    // }

    // void data_cb (EV_P_ ev_io *w_, int revents)
    // {
    //     struct my_io *w = (struct my_io *)w_;

    //     // for one-shot events, one must manually stop the watcher
    //     // with its corresponding stop function.
    //     int size;
    //     int bufferSize = 10;
    //     char buffer[bufferSize] = {0};

    //     // printf("Processing fd %d\n", w_->fd);
    //     // while(1)
    //     // {
    //     size = read(w_->fd, buffer, bufferSize);
    //     // usleep(100000);
    //     send(w_->fd, sampleResponse, strlen(sampleResponse), 0);
    //     // }
    //     ev_io_stop (EV_A_ w_);
    //     // this causes all nested ev_run's to stop iterating
    //     ev_break (EV_A_ EVBREAK_ONE);
    //     // else
    //     //     printf("del %d from poll\n", w_->fd);


    //     // TODO: fix this
    //     // let the parent know that the child is processing the fd, so remove the fd from parent epollfd right now
    //     struct epoll_event event;
    //     event.data.fd = w->io.fd;
    //     event.events = EPOLLIN | EPOLLET; // EPOLLET  = edge-triggered
    //     int s = epoll_ctl (w->shared->epollfd, EPOLL_CTL_DEL, w->io.fd, &event);
    //     // if (s < 0)
    //     //     handle_error("epoll_ctl del connection");
    //     /* Close conenction after reading */
    //     close(w_->fd);
    //     free(w->shared);
    // }

    // another callback, this time for a time-out
    void timeout_cb (EV_P_ ev_timer *w, int revents)
    {
        puts ("timeout");
        ev_timer_stop (EV_A_ w);
        // this causes the innermost ev_run to stop iterating
        ev_break (EV_A_ EVBREAK_ONE);
    }
    // void
    // sigint_cb (struct ev_loop *loop, ev_signal *w, int revents)
    // {
    //     printf("Jump");
    //     ev_unloop (loop, EVUNLOOP_ALL);
    // }


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
        struct sockaddr_in servaddr;
        socklen_t len;
        int opt = 1;
        int server_fd;
        int idx;
    
        server_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        len = sizeof(servaddr);

        if (server_fd == -1)
        handle_error("SOCKET ERROR");
        // Forcefully attaching socket to the port SOCKET_PORT
        if (setsockopt(server_fd, SOL_SOCKET,
                    SO_REUSEADDR | SO_REUSEPORT, &opt,
                    // SO_REUSEADDR, &opt,
                    sizeof(opt)))
            handle_error("setsockopt");

        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = INADDR_ANY;
        servaddr.sin_port = htons(getPort());

        if (bind(server_fd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1)
            handle_error("bind");
            
        return server_fd;
    }

    int HttpServer::setnonblocking(int socket_fd) // https://stackoverflow.com/questions/27266346/how-to-set-file-descriptor-non-blocking
    {
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

    // int getResource()
    // {
    //     int idx;
    //     pthread_mutex_lock(&mutex);
    //     for (idx = 0; idx < THREAD_POOL_SIZE; idx++)
    //         if (thread_infos[idx].state == THREAD_WAITING)
    //         {
    //             thread_infos[idx].state = THREAD_RUNNING; // ask child to start processing on events[idx].data.fd
    //             pthread_mutex_unlock(&mutex);
    //             return idx;
    //         }
    //     pthread_mutex_unlock(&mutex);
    //     return -1;
    // }

    void* threadStart(void* arg)
    {
        SharedThread* shared = (SharedThread*) arg;
        int size;
        int bufferSize = 1024;
        char buffer[bufferSize] = {0};
        int s, conn_fd;
        int workingThr = shared->thread_idx;
        EventData *edata;
        while(1)
        {
            int n, idx;
            n = epoll_wait (epoll_list[workingThr], worker_events_list[workingThr], MAX_EVENTS, -1);
            for (idx = 0; idx < n; idx ++)
            {
                if (worker_events_list[workingThr][idx].events & EPOLLERR)
                    printf("ERROR HAPPENED, EPOLLERR\n");
                else if (worker_events_list[workingThr][idx].events & EPOLLHUP)
                    printf("ERROR HAPPENED, EPOLLHUP\n");
                else if (!(worker_events_list[workingThr][idx].events & EPOLLIN))
                    printf("ERROR HAPPENED, not EPOLLI\n");
                else
                {
                    // process data here
                    edata = (EventData*) worker_events_list[workingThr][idx].data.ptr;
                    conn_fd = edata->fd;
                    bool done = false;

                    size = read(conn_fd, buffer, 1024);
                    if (size == -1) // 0 means done or client closed, -1 means error
                    {
                        if (errno != EAGAIN)
                        {
                            done = true;
                            /* we just read all data, no more data to read */
                        }
                    }
                    else if (size == 0)
                    {
                        done = 1;
                    }
                    write(conn_fd, sampleResponse, sampleLength);

                    /* Close conenction after reading */
                    close(conn_fd);
                    // TODO: Handle more deeply
                    free(edata);// raised?
                }
            }

        }
    }

    void initThreadResource()
    {
        int idx;
        for (idx = 0; idx < THREAD_POOL_SIZE; idx++)
        {
            thread_infos[idx].thread_idx = idx;
            pthread_t pid;
            int err = pthread_create(&pid, NULL, threadStart, (void*)&thread_infos[idx]);
            if (err == 0){}
                // printf("thread %d start successfully\n", idx);
            else 
                printf("thread %d FAILED\n", idx);
        }
    }
    void initEpollList()
    {
        int idx;
        for (idx = 0; idx < THREAD_POOL_SIZE; idx++)
        {
            epoll_list[idx] = epoll_create1(0);
        }
    }
    // void *checkThread(void* arg)
    // {
    //     while(1)
    //     {
    //         int workingThread = countWorkingThread();
    //         // pthread_mutex_lock(&queue_mutex);
    //         int queuesize = fd_queue.size();
    //         // pthread_mutex_unlock(&queue_mutex);
    //         printf("working thread is  %d / %d, queue size is %d\n", workingThread, (int)THREAD_POOL_SIZE, queuesize);
    //         usleep(100000);
    //     }
    // }
    // void startCheckingThread()
    // {
    //     pthread_t pid;
    //     pthread_create(&pid, NULL, checkThread, NULL);
    // }


    // void HttpServer::handleConnection(int socket_fd, int epollfd)
    // {
    //     // printf("Connection arrived on fd %d\n", socket_fd);
    //     // while(1)
    //     // {
    //         // printf("Handling %d requests\n", fd_queue.counter ++);
    //         struct sockaddr in_addr;
    //         socklen_t in_len;
    //         int infd, s;
    //         // char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
    //         struct epoll_event event;

    //         in_len = sizeof(in_addr);
    //         infd = accept4(socket_fd, &in_addr, &in_len, SOCK_NONBLOCK);
    //         if (infd <= 0) // error
    //         {
    //             // if ((errno == EAGAIN) ||
    //             //     (errno == EWOULDBLOCK))
    //             // {
    //             //     /* We have processed all incoming
    //             //         connections. */
    //             //     // break;
    //             printf("ERROR");
    //                 close(infd);
    //                 return;
    //             // }
    //             // else
    //             // {
    //             //     close(infd);
    //             //     perror ("accept");
    //             //     // break;
    //             //     return;
    //             // }
    //         }
    //         // /* Get information of client */
    //         // s = getnameinfo(&in_addr, in_len, hbuf, NI_MAXHOST, sbuf, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);
    //         // if (s < 0)
    //         //     handle_error("getnameinfo");
    //         // else if (s == 0){};
    //             // printf("Accept connection on decriptor %d, from %s:%s\n", infd, hbuf, sbuf);
    //         s = setnonblocking(infd);
    //         if (s == -1)
    //         {
    //             close(infd);
    //             handle_error("setnonblocking new connection");
    //         }
    //         event.data.fd = infd;
    //         event.events = EPOLLIN | EPOLLET;
    //         s = epoll_ctl (epollfd, EPOLL_CTL_ADD, infd, &event);
    //         if (s < 0)
    //         {
    //             close(infd);
    //             handle_error("epoll_ctl new connection");
    //         }
    //         // else 
    //         //     printf("ADD %d to poll\n", infd);
    //     // }
    // }

    void HttpServer::handleData(int conn_fd)
    {
        bool done = false;
        int size;
        char buffer[1024] = {0};

        size = read(conn_fd, buffer, 1024); // stuck after reading => keep waiting for request.
        if (size == -1) // 0 means done, -1 means error
        {
            if (errno != EAGAIN)
            {
                done = true;
                /* we just read all data, no more data to read */
            }
        }
        else if (size == 0)
        {
            done = 1;
        }
        write(conn_fd, sampleResponse, sampleLength);

        /* Close conenction after reading */
        close(conn_fd);
    }


    // now, run only 1 socket with many epoll.
    void startSocket(int socket_fd)
    {
        int idx, s;
        struct epoll_event event;
        
        int opt;

        opt = fcntl(socket_fd, F_GETFD);
        if (opt < 0) {
            printf("fcntl(F_GETFD) fail.");
            handle_error("setnonblocking");
        }
        opt |= O_NONBLOCK;
        if (fcntl(socket_fd, F_SETFD, opt) < 0) {
            printf("fcntl(F_SETFD) fail.");
            handle_error("setnonblocking");
        }
        
        s = listen(socket_fd, BACKLOGSIZE);
        if (s == -1)
            handle_error("listener");        
    }

    // bool checkMatchedSocket(int eventfd)
    // {
    //     int idx;
    //     for (idx = 0; idx < NUM_SOCKET; idx++)
    //     {
    //         if (socketList[idx] == eventfd)
    //             return true;
    //     }
    //     return false;
    // }


    void HttpServer::openSocket()
    {
        int s;
        struct epoll_event events[MAX_EVENTS];
        int currentWorker = 0;
        EventData *client_data;

        struct sockaddr in_addr;
        socklen_t in_len;

        int socket_fd = init_and_bind(getPort());
        startSocket(socket_fd);
        
        initThreadResource();
        // startCheckingThread();
        initEpollList();

          /* The event loop => socket keep listening */
        while (1)
        {
            int client_fd = accept4(socket_fd, &in_addr, &in_len, SOCK_NONBLOCK);
            if (client_fd < 0)
            {
                // usleep(100);
                continue;
            }
            // distribute client fd to a worker
            client_data = new EventData();
            client_data->fd = client_fd;
            struct epoll_event event;
            event.data.ptr = (void*) client_data;
            event.events = EPOLLIN;

            s = epoll_ctl(epoll_list[currentWorker], EPOLL_CTL_ADD, client_fd, &event);
            if (s < 0)
            {
                printf("Error adding task to worker %d.", currentWorker);
                perror("Error adding task to worker");
            }
            else
                currentWorker ++;
            currentWorker %= THREAD_POOL_SIZE;

            // int n, idx;
            // n = epoll_wait (sharedEpollfd, events, MAX_EVENTS, -1);
            // for (idx = 0; idx < n; idx ++)
            // {
            //     if (events[idx].events & EPOLLERR)
            //         printf("ERROR HAPPENED, EPOLLERR, error num = %d, fd = %d\n", events[idx].events, events[idx].data.fd);
            //     else if (events[idx].events & EPOLLHUP)
            //         printf("ERROR HAPPENED, EPOLLHUP, error num = %d, fd = %d\n", events[idx].events, events[idx].data.fd);
            //     else if (!(events[idx].events & EPOLLIN))
            //     {
            //         printf("ERROR HAPPENED, not EPOLLIN error num = %d, fd = %d\n", events[idx].events, events[idx].data.fd);
            //         // close(events[idx].data.fd);
            //         // continue;
            //     }
	        //     else if (checkMatchedSocket(events[idx].data.fd))
            //     {
            //         handleConnection(events[idx].data.fd, sharedEpollfd);
            //     }
            //     else
            //     {
            //         // int thread_idx = getResource();
            //         // while (thread_idx < 0)
            //         // {
            //         //     thread_idx = getResource();
            //         //     // printf("pidx is %d\n", thread_idx);
            //         //     usleep(100);
            //         // }
            //         // pthread_mutex_lock(&mutex);
            //         // // TODO: push events[idx].data.fd to queue
            //         // thread_infos[thread_idx].event_fd = events[idx].data.fd;
            //         // thread_infos[thread_idx].epollfd = sharedEpollfd;
            //         // thread_infos[thread_idx].thread_idx = thread_idx;
            //         // pthread_mutex_unlock(&mutex);
            //         int event_fd = events[idx].data.fd;
                    
            //         // pthread_mutex_unlock(&queue_mutex);
            //         fd_queue.push(std::make_pair(sharedEpollfd, event_fd));
            //         // pthread_mutex_unlock(&queue_mutex);
            //     }
        }
    }

    void HttpServer::openEV() // open 4 thread and ask them to wait on specificed fd?
    {
        while(1)
        {
            // // use the default event loop unless you have special needs
            // struct ev_loop *loop = EV_DEFAULT;
            
            // // initialise an io watcher, then start it
            // // this one will watch for stdin to become readable
            // ev_io_init (&stdin_watcher, stdin_cb, /*STDIN_FILENO*/ 0, EV_READ); // not open a socket. 
            // ev_io_start (loop, &stdin_watcher);
            
            // // initialise a timer watcher, then start it
            // // simple non-repeating 5.5 second timeout
            // ev_timer_init (&timeout_watcher, timeout_cb, 5.5, 0.);
            // ev_timer_start (loop, &timeout_watcher);
            
            // // now wait for events to arrive
            // ev_run (loop, 0);
        }
    }
}
