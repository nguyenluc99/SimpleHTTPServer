#ifndef SERVER_H_
#define SERVER_H_

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/un.h>

#include <string>
#include <utility>

#define handle_error(msg) \
           do { perror(msg); exit(EXIT_FAILURE); } while (0)


#define SOCKET_PATH         "/tmp/my.sock"
#define MAX_CONCURRENT_REQ  100000
#define MAXEVENTS           1000000
#define HTTP_PORT           1999



namespace my_http_server
{
    char* getIP();
    class HttpServer{
        private:
            std::string host;
            int port;
            int init_and_bind(int port);
            int setnonblocking(int socket_fd);
            int setupServer(struct sockaddr_in& servaddr);
            void handleConnection(int socket_fd, struct epoll_event& event, int epollfd);
            void handleData(struct epoll_event event);


        public:
            HttpServer(const std::string& _host, int _port){
                host = _host; port=_port;
            }

            HttpServer() = default;
            ~HttpServer() = default;
            HttpServer(HttpServer&&) = default;
            HttpServer& operator=(HttpServer&&) = default;
            void openSocket();

            std::string getHost() const {return host;}
            int getPort() {return port;}
    };
};

#endif
