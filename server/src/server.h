#pragma once
#include "store.h"
#include <string>
#include <atomic>
#include <thread>
#include <vector>

#ifdef _WIN32
  #ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0600
  #endif
  #include <winsock2.h>
  #include <ws2tcpip.h>
  using socket_t = SOCKET;
  #define INVALID_SOCK INVALID_SOCKET
  #define CLOSE_SOCK(s) closesocket(s)
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <unistd.h>
  using socket_t = int;
  #define INVALID_SOCK (-1)
  #define CLOSE_SOCK(s) close(s)
#endif

class Server {
public:
    explicit Server(int port = 7379, int max_threads = 16);
    ~Server();

    void start();   // Blocking — runs until stop() is called
    void stop();

private:
    void accept_loop();
    void handle_client(socket_t client_fd);
    std::string process_command(const std::string& line);

    int port_;
    int max_threads_;
    socket_t server_fd_ = INVALID_SOCK;
    std::atomic<bool> running_{false};
    Store store_;

    std::vector<std::thread> workers_;
    std::mutex workers_mutex_;
};