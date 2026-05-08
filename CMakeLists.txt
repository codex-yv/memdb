cmake_minimum_required(VERSION 3.16)
project(memdb LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Output all binaries to build/bin ──────────────────────────────────────────
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ── Platform-specific socket libraries ───────────────────────────────────────
if(WIN32)
    set(SOCKET_LIBS ws2_32)
else()
    set(SOCKET_LIBS "")
endif()

# ─────────────────────────────────────────────────────────────────────────────
# Target 1: memdb-client  (static library — link this into your project)
# ─────────────────────────────────────────────────────────────────────────────
add_library(memdb-client STATIC
    client/memdb.cpp
    client/memdb.h
)
target_include_directories(memdb-client PUBLIC client/)
target_link_libraries(memdb-client PUBLIC ${SOCKET_LIBS})

# ─────────────────────────────────────────────────────────────────────────────
# Target 2: memdb-server  (the database server binary)
# ─────────────────────────────────────────────────────────────────────────────
add_executable(memdb-server
    server/src/main.cpp
    server/src/server.cpp
    server/src/store.cpp
    server/src/parser.cpp
)
target_include_directories(memdb-server PRIVATE server/src/)
target_link_libraries(memdb-server PRIVATE ${SOCKET_LIBS})

# Enable extra warnings
if(MSVC)
    target_compile_options(memdb-server PRIVATE /W4)
else()
    target_compile_options(memdb-server PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ─────────────────────────────────────────────────────────────────────────────
# Target 3: memdb-cli  (command-line client)
# ─────────────────────────────────────────────────────────────────────────────
add_executable(memdb-cli
    cli/main.cpp
)
target_link_libraries(memdb-cli PRIVATE memdb-client)

# ─────────────────────────────────────────────────────────────────────────────
# Target 4: example  (usage demo — optional, disable in production)
# ─────────────────────────────────────────────────────────────────────────────
option(BUILD_EXAMPLE "Build the example program" ON)
if(BUILD_EXAMPLE)
    add_executable(example example.cpp)
    target_link_libraries(example PRIVATE memdb-client)
endif()

# ─────────────────────────────────────────────────────────────────────────────
# Install rules (optional — run: cmake --install build/)
# ─────────────────────────────────────────────────────────────────────────────
install(TARGETS memdb-server memdb-cli DESTINATION bin)
install(FILES client/memdb.h DESTINATION include)
install(TARGETS memdb-client DESTINATION lib)

message(STATUS "── MemDB Build Configuration ──────────────────────")
message(STATUS "  C++ standard  : ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build type    : ${CMAKE_BUILD_TYPE}")
message(STATUS "  Output dir    : ${CMAKE_BINARY_DIR}/bin")
message(STATUS "  Socket libs   : ${SOCKET_LIBS}")
message(STATUS "  Build example : ${BUILD_EXAMPLE}")
message(STATUS "───────────────────────────────────────────────────")