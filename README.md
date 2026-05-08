# MemDB — In-Memory Key-Value Database

A Redis-inspired in-memory database written in C++17.  
Supports `SET`, `GET`, `DEL`, `PING`, `SIZE` over a TCP text protocol.

---

## Project Structure

```
memdb/
├── server/src/
│   ├── main.cpp        Entry point + signal handling
│   ├── server.cpp/.h   TCP accept loop, thread pool
│   ├── store.cpp/.h    unordered_map + shared_mutex
│   └── parser.cpp/.h   Protocol parser
├── client/
│   ├── memdb.h         ← Include this in your project
│   └── memdb.cpp       TCP socket client implementation
├── cli/
│   └── main.cpp        Interactive REPL + one-shot CLI
├── example.cpp         Usage demo
├── CMakeLists.txt
├── Dockerfile
└── render.yaml
```

---

## Building on MSYS2 (Windows)

### 1. Install dependencies

Open the **MSYS2 MinGW 64-bit** shell (search "MSYS2 MinGW x64" in Start menu) and run:

```bash
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
```

### 2. Clone / copy the project

```bash
cd ~
# If using git:
# git clone https://github.com/yourname/memdb.git
cd memdb
```

### 3. Configure with CMake

```bash
cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
```

### 4. Build

```bash
cmake --build build
```

Binaries are written to `build/bin/`:
- `build/bin/memdb-server.exe`
- `build/bin/memdb-cli.exe`
- `build/bin/example.exe`

### 5. Quick smoke test

Terminal 1 — start the server:
```bash
./build/bin/memdb-server.exe
# [MemDB] Server listening on port 7379
```

Terminal 2 — use the CLI:
```bash
./build/bin/memdb-cli.exe SET name Alice
./build/bin/memdb-cli.exe GET name
./build/bin/memdb-cli.exe DEL name
./build/bin/memdb-cli.exe PING
./build/bin/memdb-cli.exe SIZE
```

Terminal 2 — interactive shell:
```bash
./build/bin/memdb-cli.exe
memdb> SET city Mumbai
OK
memdb> GET city
Mumbai
memdb> SIZE
2 key(s)
memdb> EXIT
```

Run the example:
```bash
./build/bin/example.exe
```

---

## Using the Client Library in Your Code

Copy `client/memdb.h` and `client/memdb.cpp` into your project, then:

```cpp
#include "memdb.h"

int main() {
    MemDB db("127.0.0.1", 7379);

    db.set("user:1", "Alice");
    db.set("user:2", "Bob");

    std::string name = db.get("user:1");   // "Alice"

    std::string val;
    if (db.get("user:2", val))             // returns false if missing
        std::cout << val << "\n";

    db.del("user:1");

    db.ping();                             // true if server alive
    db.size();                             // number of keys
}
```

### CMake integration (recommended)

Add to your own `CMakeLists.txt`:

```cmake
add_subdirectory(memdb)          # path to this project
target_link_libraries(your_app PRIVATE memdb-client)
```

Then just `#include "memdb.h"` — CMake handles linking.

### Manual compile (MSYS2/MinGW)

```bash
g++ your_app.cpp memdb/client/memdb.cpp -o your_app -lws2_32 -std=c++17
```

---

## Wire Protocol

Plain text over TCP, one command per line, `\n` terminated.

| Command          | Response              |
|------------------|-----------------------|
| `SET key value`  | `+OK`                 |
| `GET key`        | `+value` or `-ERR …`  |
| `DEL key`        | `+OK` or `-ERR …`     |
| `PING`           | `+PONG`               |
| `SIZE`           | `+<number>`           |

Responses starting with `+` are success, `-` are errors.  
Values may contain spaces (everything after the key is the value for SET).

You can test with `telnet` or `netcat`:

```bash
# netcat (install via: pacman -S mingw-w64-x86_64-netcat)
echo "SET foo bar" | nc 127.0.0.1 7379
echo "GET foo"     | nc 127.0.0.1 7379
```

---

## Custom Port

```bash
# Server on port 8000
./memdb-server.exe 8000

# CLI pointing to port 8000
./memdb-cli.exe --port 8000 GET mykey

# Client library
MemDB db("127.0.0.1", 8000);
```

---

## Cloud Deployment (Render / Railway / Fly.io)

The server reads the `PORT` environment variable automatically (cloud platforms inject this).

### Render

1. Push to GitHub
2. Create a new **Web Service** → select your repo → choose **Docker**
3. Render sets `$PORT` automatically — no config needed

### Railway / Fly.io

```bash
# Railway
railway up

# Fly.io
fly launch
fly deploy
```

The `Dockerfile` builds a minimal Debian image (~40MB) with just the server binary.

---

## Thread Safety

- The store uses `std::shared_mutex`: multiple `GET`s run concurrently, `SET`/`DEL` takes an exclusive lock.
- Each client connection runs in its own thread (up to `max_threads`, default 16).
- The server rejects new connections with `-ERR server busy` when the thread pool is full.

---

## Build Options

| Option           | Default | Description                     |
|------------------|---------|---------------------------------|
| `BUILD_EXAMPLE`  | `ON`    | Build `example.cpp` demo binary |

```bash
cmake -B build -DBUILD_EXAMPLE=OFF   # skip the example
```