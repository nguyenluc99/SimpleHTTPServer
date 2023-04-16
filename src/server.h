#ifndef SERVER_H_
#define SERVER_H_

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <ev.h>

#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <queue>

#include <ulimit.h>
#include <condition_variable>
#include <iostream>
#include <mutex>

#include <string>
#include <utility>

#define handle_error(msg) \
           do { perror(msg); exit(EXIT_FAILURE); } while (0)


// #define SOCKET_PATH         "/tmp/my.sock"
#define MAX_CONCURRENT_REQ  10000
#define MAXEVENTS           100000
#define HTTP_PORT           3000
#define MAX_THREADS         1
#define NUM_SOCKET          1



namespace my_http_server
{

        // Thread-safe queue
    template <typename T>
    class TSQueue {
    private:
        // Underlying queue
        std::queue<T> m_queue;
    
        // mutex for thread synchronization
        std::mutex m_mutex;
    
        // Condition variable for signaling
        std::condition_variable m_cond;

    
    public:
        int counter;
        // Pushes an element to the queue
        void push(T item)
        {
    
            // Acquire lock
            std::unique_lock<std::mutex> lock(m_mutex);
    
            // Add item
            m_queue.push(item);
    
            // Notify one thread that
            // is waiting
            m_cond.notify_one();
        }
    
        // Pops an element off the queue
        T pop()
        {
    
            // acquire lock
            std::unique_lock<std::mutex> lock(m_mutex);
    
            // wait until queue is not empty
            m_cond.wait(lock,
                        [this]() { return !m_queue.empty(); });
    
            // retrieve item
            T item = m_queue.front();
            m_queue.pop();
    
            // return item
            return item;
        }

        int size()
        {
            // acquire lock
            std::unique_lock<std::mutex> lock(m_mutex);
            int size = m_queue.size();
            return size;
        }

        bool empty()
        {
            // acquire lock
            std::unique_lock<std::mutex> lock(m_mutex);
            int size = m_queue.size();
            return (size == 0);
        }
        
        // void popCount()
        // {
        //     // Acquire lock
        //     std::unique_lock<std::mutex> lock(m_mutex);
        //     counter --;
        // }
        int getCount()
        {
            // Acquire lock
            std::unique_lock<std::mutex> lock(m_mutex);
            int c = counter;
            return c;
        }
    };
    
    typedef enum ThreadExeState
    {
        THREAD_FREE,        /* not started */
        THREAD_WAITING,     /* started, and waiting for file descriptor to read */
        THREAD_RUNNING,     /* busy in reading and writing response   */
        THREAD_FINISHED     /* final state => to be joined            */
    } ThreadExeState;

    typedef struct SharedThread
    {
        // int                         event_fd;       /* file discriptor that this thread is handling */
        int                         epollfd;        /* the epoll file discriptor */
        // int                         thread_idx;     /* index of this thread, to revtrieve back */
        // ThreadExeState              state;          /* execution state */
        // pthread_t                   thread_id;
        SharedThread(){epollfd = -1;};
        // SharedThread(){event_fd = -1; epollfd = -1; thread_idx = -1; state = THREAD_FREE;};
    } SharedThread;
    typedef struct my_io        /* used for interaction with ev */
    {
        ev_io io;
        // int epollfd;
        SharedThread* shared;
        // void *somedata;
        // struct whatever *mostinteresting;
    } my_io;

    extern SharedThread thread_infos[MAX_THREADS];
    extern pthread_mutex_t mutex;
    
    char* getIP();
    class HttpServer{
        private:
            std::string host;
            int port;
            int init_and_bind(int port);
            int setnonblocking(int socket_fd);
            int setupServer(struct sockaddr_in& servaddr);
            void handleConnection(int socket_fd, int epollfd);
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
            void openEV();

            std::string getHost() const {return host;}
            int getPort() {return port;}
    };
};

#endif
