


#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <string>
#include <utility>

#include "server.h"

using my_http_server::HttpServer;

int main(int argc, char* argv[])
{
    char* host = my_http_server::getIP();
    printf("HOST IS '%s', port is %d\n", host, HTTP_PORT);
    HttpServer server(std::string(host), HTTP_PORT);
    server.openSocket();
    return 0;
}
