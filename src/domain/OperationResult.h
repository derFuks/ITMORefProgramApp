#pragma once
#include <string>

struct OperationResult {
    bool ok{false};
    std::string message;

    static OperationResult Success(std::string msg = {}) {
        return {true, std::move(msg)};
    }
    static OperationResult Fail(std::string msg) {
        return {false, std::move(msg)};
    }
};
