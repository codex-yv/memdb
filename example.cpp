/*
 * example.cpp — shows how to use the MemDB client library
 *
 * Compile & link against the memdb-client static library:
 *   (CMake handles this automatically — see README.md)
 *
 * Or manually (MSYS2/MinGW):
 *   g++ example.cpp ../client/memdb.cpp -o example -lws2_32 -std=c++17
 */

#include "client/memdb.h"
#include <iostream>

int main() {
    try {
        // Connect to the running MemDB server
        MemDB db("127.0.0.1", 7379);

        // ── Basic SET / GET / DEL ─────────────────────────────────────────────

        db.set("name", "Alice");
        db.set("city", "Mumbai");
        db.set("score", "42");

        std::cout << "name  = " << db.get("name")  << "\n"; // Alice
        std::cout << "city  = " << db.get("city")  << "\n"; // Mumbai
        std::cout << "score = " << db.get("score") << "\n"; // 42
        std::cout << "keys  = " << db.size()        << "\n"; // 3

        // ── Check key existence ───────────────────────────────────────────────

        std::string val;
        if (db.get("name", val))
            std::cout << "Found: " << val << "\n";
        else
            std::cout << "Key not found\n";

        // ── Delete ────────────────────────────────────────────────────────────

        bool deleted = db.del("city");
        std::cout << "Deleted city: " << (deleted ? "yes" : "no") << "\n";
        std::cout << "After delete, size = " << db.size() << "\n"; // 2

        // ── Overwrite ─────────────────────────────────────────────────────────

        db.set("name", "Bob");
        std::cout << "Updated name = " << db.get("name") << "\n"; // Bob

        // ── Missing key returns empty string ──────────────────────────────────

        std::cout << "Missing key = '" << db.get("doesnt_exist") << "'\n"; // ''

        // ── Ping ──────────────────────────────────────────────────────────────

        std::cout << "Server alive: " << (db.ping() ? "yes" : "no") << "\n";

    } catch (const MemDBException& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    return 0;
}