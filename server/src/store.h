#pragma once
#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <optional>

class Store {
public:
    void set(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key);
    bool del(const std::string& key);
    size_t size();

private:
    std::unordered_map<std::string, std::string> data_;
    mutable std::shared_mutex mutex_;
};