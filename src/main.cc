#include <drogon/drogon.h>
#include <iostream>

int main() {
    using namespace drogon;

    app().loadConfigFile("config.json");

    // Проверка БД после старта event loop
    app().getLoop()->queueInLoop([]() {
        auto db = drogon::app().getDbClient("default");
        if (!db) {
            std::cerr << "Database client 'default' not found. Check config.json -> db_clients.\n";
            return;
        }

        db->execSqlAsync(
            "SELECT 1",
            [](const drogon::orm::Result &r) {
                std::cout << "Database connection OK. SELECT 1 returned "
                          << r.size() << " row(s)." << std::endl;
            },
            [](const drogon::orm::DrogonDbException &e) {
                std::cerr << "Database connection FAILED: " << e.base().what() << std::endl;
            }
        );
    });

    app().run();
}
