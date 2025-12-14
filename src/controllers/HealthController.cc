#include "HealthController.h"
#include <drogon/drogon.h>

void HealthController::health(const drogon::HttpRequestPtr&,
                              std::function<void (const drogon::HttpResponsePtr&)>&& callback) {
    auto db = drogon::app().getDbClient("default");
    if (!db) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->setBody("DB client not configured");
        return callback(resp);
    }

    db->execSqlAsync(
        "SELECT 1",
        [callback](const drogon::orm::Result&) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k200OK);
            resp->setBody("Все OK");
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException& e) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k500InternalServerError);
            resp->setBody(std::string("БД ЗАФЕЙЛИЛАСЬ: ") + e.base().what());
            callback(resp);
        }
    );
}
