#include "AuthFilter.h"
#include <drogon/drogon.h>

#include <json/json.h>

static drogon::HttpResponsePtr jsonErr(drogon::HttpStatusCode code, const std::string& msg) {
    Json::Value out;
    out["ok"] = false;
    out["error"] = msg;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(out);
    resp->setStatusCode(code);
    return resp;
}

void AuthFilter::doFilter(const drogon::HttpRequestPtr& req,
                          drogon::FilterCallback&& fcb,
                          drogon::FilterChainCallback&& fccb) {
    // const auto apiKey = req->getHeader("x-api-key");
    // if (apiKey.empty()) {
    //     return fcb(jsonErr(drogon::k401Unauthorized, "Missing X-API-Key"));
    // }

    std::string apiKey = req->getHeader("x-api-key");
    if (apiKey.empty()) {
        auto qp = req->getParameters();
        auto it = qp.find("api_key");
        if (it != qp.end()) apiKey = it->second;
    }
    // убрать перед сдачей, это пока чтобы смотреть в браузере http://localhost:8080/manager/requests?api_key=manager-key-1


    auto db = drogon::app().getDbClient("default");
    if (!db) {
        return fcb(jsonErr(drogon::k500InternalServerError, "DB client not configured"));
    }



    db->execSqlAsync(
        R"SQL(
            SELECT u.id, u.role
            FROM user_api_keys k
            JOIN users u ON u.id = k.user_id
            WHERE k.api_key = $1 AND k.is_active = TRUE AND u.is_deleted = FALSE
            LIMIT 1
        )SQL",
        [req, fccb, fcb](const drogon::orm::Result& r) {
            if (r.empty()) {
                return fcb(jsonErr(drogon::k401Unauthorized, "Нерабочий ключ API"));
            }
            const auto userId = r[0]["id"].as<long long>();
            const auto role = r[0]["role"].as<std::string>();

        // Сохраняю в контекст запроса чтобы достучаться в контроллерах
            req->attributes()->insert("user_id", userId);
            req->attributes()->insert("role", role);

            fccb();
        },
        [fcb](const drogon::orm::DrogonDbException& e) {
            fcb(jsonErr(drogon::k500InternalServerError, std::string("Ошибка БД: ") + e.base().what()));
        },
        apiKey
    );
}
