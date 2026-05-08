#include "parser.h"
#include <sstream>
#include <algorithm>
#include <cctype>

std::vector<std::string> Parser::tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string token;
    // First two tokens split by space, rest goes into value (to allow spaces in values)
    if (iss >> token) tokens.push_back(token); // CMD
    if (iss >> token) tokens.push_back(token); // KEY
    std::string remainder;
    if (std::getline(iss, remainder)) {
        // Trim leading space
        size_t start = remainder.find_first_not_of(' ');
        if (start != std::string::npos)
            tokens.push_back(remainder.substr(start));
    }
    return tokens;
}

CmdType Parser::toType(const std::string& token) {
    std::string upper = token;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    if (upper == "SET")  return CmdType::SET;
    if (upper == "GET")  return CmdType::GET;
    if (upper == "DEL")  return CmdType::DEL;
    if (upper == "PING") return CmdType::PING;
    if (upper == "SIZE") return CmdType::SIZE;
    return CmdType::UNKNOWN;
}

Command Parser::parse(const std::string& line) {
    Command cmd;
    auto tokens = tokenize(line);
    if (tokens.empty()) {
        cmd.raw_error = "Empty command";
        return cmd;
    }

    cmd.type = toType(tokens[0]);

    switch (cmd.type) {
        case CmdType::SET:
            if (tokens.size() < 3) {
                cmd.type = CmdType::UNKNOWN;
                cmd.raw_error = "SET requires KEY and VALUE";
            } else {
                cmd.key   = tokens[1];
                cmd.value = tokens[2];
            }
            break;
        case CmdType::GET:
        case CmdType::DEL:
            if (tokens.size() < 2) {
                cmd.type = CmdType::UNKNOWN;
                cmd.raw_error = tokens[0] + " requires KEY";
            } else {
                cmd.key = tokens[1];
            }
            break;
        case CmdType::PING:
        case CmdType::SIZE:
            break;
        default:
            cmd.raw_error = "Unknown command: " + tokens[0];
            break;
    }
    return cmd;
}