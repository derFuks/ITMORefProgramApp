#pragma once
#include <optional>
#include <string>

struct RequestFilter {
    std::optional<std::string> status;
    std::optional<std::string> dateFrom; // ISO string
    std::optional<std::string> dateTo;   // ISO string

    int limit{20};
    int offset{0};
};
