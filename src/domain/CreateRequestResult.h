#pragma once
#include <optional>
#include <string>

struct CreateRequestResult {
    bool ok{false};
    std::optional<long long> requestId;
    std::string message;

    static CreateRequestResult Success(long long id) {
        return {true, id, "Request created"};
    }
    static CreateRequestResult Fail(std::string msg) {
        return {false, std::nullopt, std::move(msg)};
    }
};
