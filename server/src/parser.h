#pragma once
#include <string>
#include <vector>

enum class CmdType { SET, GET, DEL, PING, SIZE, UNKNOWN };

struct Command {
    CmdType type = CmdType::UNKNOWN;
    std::string key;
    std::string value;
    std::string raw_error; // set if parse failed
};

class Parser {
public:
    // Parse a single line like "SET mykey myvalue"
    static Command parse(const std::string& line);

private:
    static std::vector<std::string> tokenize(const std::string& line);
    static CmdType toType(const std::string& token);
};