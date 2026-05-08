/*
 * memdb-cli — command-line interface for MemDB
 *
 * One-shot:    memdb-cli SET name Alice
 *              memdb-cli GET name
 *              memdb-cli DEL name
 *              memdb-cli PING
 *              memdb-cli SIZE
 *
 * Interactive: memdb-cli              (starts a REPL)
 *              memdb-cli --host 192.168.1.10 --port 7379
 */

#include "../client/memdb.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

static void print_help() {
    std::cout <<
        "memdb-cli [--host HOST] [--port PORT] [COMMAND [ARGS...]]\n"
        "\n"
        "Commands:\n"
        "  SET <key> <value>   Store a value\n"
        "  GET <key>           Retrieve a value\n"
        "  DEL <key>           Delete a key\n"
        "  PING                Check server liveness\n"
        "  SIZE                Number of keys stored\n"
        "  HELP                Show this help\n"
        "  EXIT / QUIT         Exit interactive mode\n"
        "\n"
        "Examples:\n"
        "  memdb-cli SET user:1 Alice\n"
        "  memdb-cli GET user:1\n"
        "  memdb-cli --port 7380 PING\n";
}

static std::string run_command(MemDB& db, const std::vector<std::string>& args) {
    if (args.empty()) return "";
    std::string cmd = args[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

    if (cmd == "SET") {
        if (args.size() < 3) return "Usage: SET <key> <value>";
        // Join remaining args as value (supports spaces in value)
        std::string value;
        for (size_t i = 2; i < args.size(); ++i) {
            if (i > 2) value += ' ';
            value += args[i];
        }
        db.set(args[1], value);
        return "OK";
    }
    if (cmd == "GET") {
        if (args.size() < 2) return "Usage: GET <key>";
        std::string val;
        if (db.get(args[1], val)) return val;
        return "(nil)";
    }
    if (cmd == "DEL") {
        if (args.size() < 2) return "Usage: DEL <key>";
        bool ok = db.del(args[1]);
        return ok ? "OK" : "(key not found)";
    }
    if (cmd == "PING")   return db.ping() ? "PONG" : "(no response)";
    if (cmd == "SIZE")   return std::to_string(db.size()) + " key(s)";
    if (cmd == "HELP")   { print_help(); return ""; }
    return "Unknown command: " + args[0];
}

static std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string tok;
    while (iss >> tok) tokens.push_back(tok);
    return tokens;
}

int main(int argc, char* argv[]) {
    std::string host = "127.0.0.1";
    int port = 7379;
    std::vector<std::string> cmd_args;

    // Parse args
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if ((a == "--host" || a == "-h") && i + 1 < argc) {
            host = argv[++i];
        } else if ((a == "--port" || a == "-p") && i + 1 < argc) {
            try { port = std::stoi(argv[++i]); }
            catch (...) { std::cerr << "Invalid port\n"; return 1; }
        } else if (a == "--help") {
            print_help(); return 0;
        } else {
            cmd_args.push_back(a);
        }
    }

    // Connect
    MemDB* db = nullptr;
    try {
        db = new MemDB(host, port);
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    // One-shot mode
    if (!cmd_args.empty()) {
        try {
            std::string result = run_command(*db, cmd_args);
            if (!result.empty()) std::cout << result << "\n";
        } catch (const std::exception& e) {
            std::cerr << e.what() << "\n";
            delete db;
            return 1;
        }
        delete db;
        return 0;
    }

    // Interactive REPL mode
    std::cout << "MemDB CLI — connected to " << host << ":" << port << "\n";
    std::cout << "Type HELP for commands, EXIT to quit.\n";

    std::string line;
    while (true) {
        std::cout << "memdb> ";
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;

        auto tokens = tokenize(line);
        if (tokens.empty()) continue;

        std::string first = tokens[0];
        std::transform(first.begin(), first.end(), first.begin(), ::toupper);
        if (first == "EXIT" || first == "QUIT") break;

        try {
            std::string result = run_command(*db, tokens);
            if (!result.empty()) std::cout << result << "\n";
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
            // Try to reconnect once
            try { db->reconnect(); std::cerr << "Reconnected.\n"; }
            catch (...) { std::cerr << "Could not reconnect.\n"; break; }
        }
    }

    delete db;
    return 0;
}