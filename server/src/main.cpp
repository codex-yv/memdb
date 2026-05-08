#include "server.h"
#include <iostream>
#include <stdexcept>
#include <csignal>
#include <cstdlib>

static Server* g_server = nullptr;

static void handle_signal(int) {
    std::cout << "\n[MemDB] Shutting down...\n";
    if (g_server) g_server->stop();
    std::exit(0);
}

int main(int argc, char* argv[]) {
    int port = 7379;
    if (argc >= 2) {
        try { port = std::stoi(argv[1]); }
        catch (...) {
            std::cerr << "Usage: memdb-server [port]\n";
            return 1;
        }
    }

    // Respect $PORT env var for cloud hosting (Render, Railway, etc.)
    if (const char* env_port = std::getenv("PORT")) {
        try { port = std::stoi(env_port); }
        catch (...) {}
    }

    std::signal(SIGINT,  handle_signal);
    std::signal(SIGTERM, handle_signal);

    try {
        Server server(port);
        g_server = &server;
        server.start(); // Blocks until stop()
    } catch (const std::exception& e) {
        std::cerr << "[MemDB] Fatal: " << e.what() << "\n";
        return 1;
    }
    return 0;
}