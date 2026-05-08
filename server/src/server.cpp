#ifdef _WIN32
  #ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0600
  #endif
#endif
#include "server.h"
#include "parser.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <cstdint>
#include <algorithm>

#ifdef _MSC_VER
  #pragma comment(lib, "ws2_32.lib")
#endif

// ─── Platform init/cleanup ────────────────────────────────────────────────────

static void platform_init() {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0)
        throw std::runtime_error("WSAStartup failed");
#endif
}

static void platform_cleanup() {
#ifdef _WIN32
    WSACleanup();
#endif
}

// ─── Constructor / Destructor ─────────────────────────────────────────────────

Server::Server(int port, int max_threads)
    : port_(port), max_threads_(max_threads) {
    platform_init();
}

Server::~Server() {
    stop();
    platform_cleanup();
}

// ─── Start ────────────────────────────────────────────────────────────────────

void Server::start() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ == INVALID_SOCK)
        throw std::runtime_error("Failed to create socket");

    // Allow port reuse so restarts are instant
    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&opt), sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(static_cast<uint16_t>(port_));

    if (bind(server_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
        throw std::runtime_error("Bind failed on port " + std::to_string(port_));

    if (listen(server_fd_, 128) < 0)
        throw std::runtime_error("Listen failed");

    running_ = true;
    std::cout << "[MemDB] Server listening on port " << port_ << "\n";
    std::cout << "[MemDB] Commands: SET <key> <value> | GET <key> | DEL <key> | PING | SIZE\n";
    std::cout << "[MemDB] Press Ctrl+C to stop.\n";

    accept_loop();
}

void Server::stop() {
    running_ = false;
    if (server_fd_ != INVALID_SOCK) {
        CLOSE_SOCK(server_fd_);
        server_fd_ = INVALID_SOCK;
    }
    // Join all worker threads
    std::lock_guard<std::mutex> lk(workers_mutex_);
    for (auto& t : workers_)
        if (t.joinable()) t.join();
    workers_.clear();
}

// ─── Accept Loop ─────────────────────────────────────────────────────────────

void Server::accept_loop() {
    while (running_) {
        sockaddr_in client_addr{};
        socklen_t len = sizeof(client_addr);
        socket_t client_fd = accept(server_fd_,
                                    reinterpret_cast<sockaddr*>(&client_addr),
                                    &len);
        if (client_fd == INVALID_SOCK) {
            if (!running_) break; // Server was stopped
            std::cerr << "[MemDB] Accept error\n";
            continue;
        }

        // Spin up a thread per connection (capped at max_threads_)
        std::lock_guard<std::mutex> lk(workers_mutex_);

        // Reap finished threads
        workers_.erase(
            std::remove_if(workers_.begin(), workers_.end(),
                [](std::thread& t) {
                    if (t.joinable()) {
                        // Non-blocking check — just leave joinable ones running
                        return false;
                    }
                    return true;
                }),
            workers_.end()
        );

        if ((int)workers_.size() >= max_threads_) {
            // Reject connection gracefully
            const char* msg = "-ERR server busy\n";
            send(client_fd, msg, (int)strlen(msg), 0);
            CLOSE_SOCK(client_fd);
            continue;
        }

        workers_.emplace_back([this, client_fd]() {
            handle_client(client_fd);
        });
        workers_.back().detach(); // Let them run independently
    }
}

// ─── Client Handler ───────────────────────────────────────────────────────────

void Server::handle_client(socket_t client_fd) {
    char buf[4096];
    std::string accumulator;

    while (true) {
        int bytes = recv(client_fd, buf, sizeof(buf) - 1, 0);
        if (bytes <= 0) break; // Client disconnected

        buf[bytes] = '\0';
        accumulator += buf;

        // Process every complete line (newline-delimited protocol)
        size_t pos;
        while ((pos = accumulator.find('\n')) != std::string::npos) {
            std::string line = accumulator.substr(0, pos);
            accumulator.erase(0, pos + 1);

            // Strip \r if present (Windows clients send \r\n)
            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            if (line.empty()) continue;

            std::string response = process_command(line);
            response += "\n";
            send(client_fd, response.c_str(), (int)response.size(), 0);
        }
    }

    CLOSE_SOCK(client_fd);
}

// ─── Command Dispatch ─────────────────────────────────────────────────────────

std::string Server::process_command(const std::string& line) {
    Command cmd = Parser::parse(line);

    switch (cmd.type) {
        case CmdType::SET:
            store_.set(cmd.key, cmd.value);
            return "+OK";

        case CmdType::GET: {
            auto val = store_.get(cmd.key);
            if (val) return "+" + *val;
            return "-ERR key not found";
        }

        case CmdType::DEL:
            if (store_.del(cmd.key)) return "+OK";
            return "-ERR key not found";

        case CmdType::PING:
            return "+PONG";

        case CmdType::SIZE:
            return "+" + std::to_string(store_.size());

        default:
            return "-ERR " + cmd.raw_error;
    }
}