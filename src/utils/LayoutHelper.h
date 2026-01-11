#pragma once
#include <string>
#include <optional>

namespace layout {

// роли: nullopt / менеджер / партнер
std::string renderHeader(const std::optional<std::string>& role);

// общий футер
std::string renderFooter();

}
