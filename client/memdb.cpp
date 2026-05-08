#ifdef _WIN32
  #ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0600
  #endif
#endif
#include "memdb.h"
#include <cstring>
#include <cstdint>
#include <sstream>

#ifdef _MSC_VER
  #pragma comment(lib, "ws2_32.lib")
#endif

// ─── Platform init ────────────────────────────────────────────────────────────

void MemDB::platform_init_() {
#ifdef _WIN32
    static bool initialised = false;
    if (!initialised) {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2,2), &wsa) != 0)
            throw MemDBException("WSAStartup failed");
        initialised = true;
    }
#endif
}

void MemDB::platform_cleanup_() {
#ifdef _WIN32
    WSACleanup();
#endif
}

// ─── Constructor / Destructor ─────────────────────────────────────────────────

MemDB::MemDB(const std::string& host, int port)
    : host_(host), port_(port) {
    platform_init_();
    connect_();
}

MemDB::~MemDB() {
    disconnect_();
}

MemDB::MemDB(MemDB&& other) noexcept
    : host_(std::move(other.host_)), port_(other.port_), sock_(other.sock_) {
    other.sock_ = INVALID_SOCK;
}

MemDB& MemDB::operator=(MemDB&& other) noexcept {
    if (this != &other) {
        disconnect_();
        host_ = std::move(other.host_);
        port_ = other.port_;
        sock_ = other.sock_;
        other.sock_ = INVALID_SOCK;
    }
    return *this;
}

// ─── Connect / Disconnect ─────────────────────────────────────────────────────

void MemDB::connect_() {
    sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_ == INVALID_SOCK)
        throw MemDBException("Failed to create socket");

    // Use getaddrinfo — works for IPs and hostnames ("localhost", "127.0.0.1")
    // and does NOT require inet_pton, so it compiles on all MinGW versions.
    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    std::string port_str = std::to_string(port_);

    if (getaddrinfo(host_.c_str(), port_str.c_str(), &hints, &res) != 0 || !res) {
        CLOSE_SOCK(sock_);
        sock_ = INVALID_SOCK;
        throw MemDBException("Cannot resolve host: " + host_);
    }

    if (::connect(sock_, res->ai_addr, (int)res->ai_addrlen) < 0) {
        freeaddrinfo(res);
        CLOSE_SOCK(sock_);
        sock_ = INVALID_SOCK;
        throw MemDBException("Cannot connect to " + host_ + ":" + port_str);
    }
    freeaddrinfo(res);
}

void MemDB::disconnect_() {
    if (sock_ != INVALID_SOCK) {
        CLOSE_SOCK(sock_);
        sock_ = INVALID_SOCK;
    }
}

void MemDB::reconnect() {
    disconnect_();
    connect_();
}

// ─── Send / Receive ───────────────────────────────────────────────────────────

std::string MemDB::send_command_(const std::string& cmd) {
    std::string request = cmd + "\n";
    if (send(sock_, request.c_str(), (int)request.size(), 0) < 0)
        throw MemDBException("Send failed — is the server running?");

    // Read until newline
    std::string response;
    char ch;
    while (true) {
        int n = recv(sock_, &ch, 1, 0);
        if (n <= 0) throw MemDBException("Connection closed by server");
        if (ch == '\n') break;
        if (ch != '\r') response += ch;
    }
    return response;
}

// ─── Public API ───────────────────────────────────────────────────────────────

void MemDB::set(const std::string& key, const std::string& value) {
    auto resp = send_command_("SET " + key + " " + value);
    if (resp.empty() || resp[0] != '+')
        throw MemDBException("SET failed: " + resp);
}

std::string MemDB::get(const std::string& key) {
    auto resp = send_command_("GET " + key);
    if (resp.empty()) return "";
    if (resp[0] == '+') return resp.substr(1);
    return "";
}

bool MemDB::get(const std::string& key, std::string& out) {
    auto resp = send_command_("GET " + key);
    if (!resp.empty() && resp[0] == '+') {
        out = resp.substr(1);
        return true;
    }
    return false;
}

bool MemDB::del(const std::string& key) {
    auto resp = send_command_("DEL " + key);
    return (!resp.empty() && resp[0] == '+');
}

bool MemDB::ping() {
    try {
        auto resp = send_command_("PING");
        return resp == "+PONG";
    } catch (...) {
        return false;
    }
}

size_t MemDB::size() {
    auto resp = send_command_("SIZE");
    if (!resp.empty() && resp[0] == '+') {
        try { return std::stoul(resp.substr(1)); } catch (...) {}
    }
    return 0;
}