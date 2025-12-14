#include <drogon/drogon.h>
#include <filesystem>
#include <iostream>

int main() {
    using namespace drogon;

    if (std::filesystem::exists("config.local.json")) {
        app().loadConfigFile("config.local.json");
        std::cout << "[BOOT] Loaded config.local.json\n";
    } else {
        app().loadConfigFile("config.json");
        std::cout << "[BOOT] Loaded config.json\n";
    }

    app().run();
}
