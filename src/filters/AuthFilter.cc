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

static drogon::HttpResponsePtr redirectTo(const std::string& url) {
    return drogon::HttpResponse::newRedirectionResponse(url);
}

static bool pathStartsWith(const drogon::HttpRequestPtr& req, const std::string& prefix) {
    const auto& p = req->path();
    return p.rfind(prefix, 0) == 0; // starts_with по-быстрому
}

void AuthFilter::doFilter(const drogon::HttpRequestPtr& req,
                          drogon::FilterCallback&& fcb,
                          drogon::FilterChainCallback&& fccb) {
    auto db = drogon::app().getDbClient("default");
    if (!db) {
        return fcb(jsonErr(drogon::k500InternalServerError, "Ошибка 500. клиент БД не сконфигурирован"));
    }

    // 1 — SSR: куки сессия для менеджера
    const std::string sid = req->getCookie("itmorefprog_manager_session");
    if (!sid.empty()) {
        db->execSqlAsync(
            R"SQL(
                SELECT u.id, u.role
                FROM manager_sessions s
                JOIN users u ON u.id = s.user_id
                WHERE s.session_id = $1
                  AND s.is_active = TRUE
                  AND s.expires_at > now()
                  AND u.is_deleted = FALSE
                LIMIT 1
            )SQL",
            [req, fccb, fcb](const drogon::orm::Result& r) {
                if (r.empty()) {
                    // на форму логина
                    if (pathStartsWith(req, "/manager/")) {
                        return fcb(redirectTo("/manager/login?err=Сессия%20истекла"));
                    }
                    return fcb(jsonErr(drogon::k401Unauthorized, "Ошибка 401. Сессия истекла"));
                }

                const auto userId = r[0]["id"].as<long long>();
                const auto role = r[0]["role"].as<std::string>();
                if (role != "manager") {
                    return fcb(jsonErr(drogon::k403Forbidden, "Ошибка 403. Доступ закрыт"));
                }

                req->attributes()->insert("user_id", userId);
                req->attributes()->insert("role", role);
                fccb();
            },
            [fcb](const drogon::orm::DrogonDbException& e) {
                fcb(jsonErr(drogon::k500InternalServerError, std::string("Ошибка БД: ") + e.base().what()));
            },
            sid
        );
        return; // не идём дальше, т.к. запрос в async
    }

    // // выпилить, когда на куки перестрою
    // std::string apiKey = req->getHeader("x-api-key");
    // if (apiKey.empty()) {
    //     return fcb(jsonErr(drogon::k401Unauthorized, "Missing X-API-Key"));
    // }

    // 2. SSR: куки сессия для партнера
    const std::string psid = req->getCookie("itmorefprog_partner_session");
    if (!psid.empty()) {
        db->execSqlAsync(
            R"SQL(
                SELECT u.id, u.role
                FROM partner_sessions s
                JOIN users u ON u.id = s.user_id
                WHERE s.session_id = $1
                  AND s.is_active = TRUE
                  AND s.expires_at > now()
                  AND u.is_deleted = FALSE
                LIMIT 1
            )SQL",
            [req, fccb, fcb](const drogon::orm::Result& r) {
                if (r.empty()) {
                    if (pathStartsWith(req, "/partner/")) {
                        return fcb(redirectTo("/partner/login?err=Сессия%20истекла"));
                    }
                    return fcb(jsonErr(drogon::k401Unauthorized, "Ошибка 401. Сессия партнера истекла"));
                }

                const auto userId = r[0]["id"].as<long long>();
                const auto role = r[0]["role"].as<std::string>();
                if (role != "partner") {
                    return fcb(jsonErr(drogon::k403Forbidden, "Ошибка 403. Доступ закрыт"));
                }

                req->attributes()->insert("user_id", userId);
                req->attributes()->insert("role", role);
                fccb();
            },
            [req, fcb](const drogon::orm::DrogonDbException& e) {
                if (pathStartsWith(req, "/partner/")) {
                    return fcb(redirectTo("/partner/login?err=Ошибка%20БД"));
                }
                fcb(jsonErr(drogon::k500InternalServerError, std::string("Ошибка БД: ") + e.base().what()));
            },
            psid
        );
        return;
    }

    std::string apiKey = req->getHeader("x-api-key");
    if (apiKey.empty()) {
        // Если это SSR кабинет — отправляю на login.
        if (pathStartsWith(req, "/manager/")) return fcb(redirectTo("/manager/login"));
        if (pathStartsWith(req, "/partner/")) return fcb(redirectTo("/partner/login"));
        return fcb(jsonErr(drogon::k401Unauthorized, "Missing X-API-Key"));
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
