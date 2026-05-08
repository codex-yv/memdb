#pragma once
/*
 * MemDB Client Library
 * --------------------
 * Usage:
 *   #include "memdb.h"
 *
 *   MemDB db("127.0.0.1", 7379);
 *   db.set("name", "Alice");
 *   std::string val = db.get("name");   // returns "" if key missing
 *   bool found = db.get("name", val);   // returns false if key missing
 *   db.del("name");
 *   db.ping();                          // returns true if server alive
 *   int n = db.size();                  // number of keys
 */

#include <string>
#include <stdexcept>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  using socket_t = SOCKET;
  #define INVALID_SOCK INVALID_SOCKET
  #define CLOSE_SOCK(s) closesocket(s)
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <netdb.h>
  #include <unistd.h>
  using socket_t = int;
  #define INVALID_SOCK (-1)
  #define CLOSE_SOCK(s) close(s)
#endif

class MemDBException : public std::runtime_error {
public:
    explicit MemDBException(const std::string& msg)
        : std::runtime_error("[MemDB] " + msg) {}
};

class MemDB {
public:
    // Connect to MemDB server at host:port
    explicit MemDB(const std::string& host = "127.0.0.1", int port = 7379);
    ~MemDB();

    // Not copyable — socket is a unique resource
    MemDB(const MemDB&) = delete;
    MemDB& operator=(const MemDB&) = delete;
    MemDB(MemDB&&) noexcept;
    MemDB& operator=(MemDB&&) noexcept;

    // Store a key-value pair
    void set(const std::string& key, const std::string& value);

    // Get value for key. Returns empty string if key doesn't exist.
    std::string get(const std::string& key);

    // Get value into out param. Returns false if key doesn't exist.
    bool get(const std::string& key, std::string& out);

    // Delete a key. Returns true if key existed.
    bool del(const std::string& key);

    // Returns true if server is reachable.
    bool ping();

    // Returns number of keys in the store.
    size_t size();

    // Reconnect (useful after network hiccup)
    void reconnect();

private:
    std::string host_;
    int port_;
    socket_t sock_ = INVALID_SOCK;

    void connect_();
    void disconnect_();
    std::string send_command_(const std::string& cmd);
    static void platform_init_();
    static void platform_cleanup_();
};