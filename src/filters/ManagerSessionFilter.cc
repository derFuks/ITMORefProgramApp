#include "ManagerSessionFilter.h"
#include <drogon/drogon.h>

static std::string getCookieVal(const drogon::HttpRequestPtr& req, const std::string& name) {
    // Drogon может парсить cookie через getCookie()
    return req->getCookie(name);
}

void ManagerSessionFilter::doFilter(const drogon::HttpRequestPtr& req,
                                    drogon::FilterCallback&& fcb,
                                    drogon::FilterChainCallback&& fccb) {
    const auto sid = getCookieVal(req, "itmorefprog_manager_session");
    if (sid.empty()) {
        return fcb(drogon::HttpResponse::newRedirectionResponse("/manager/login"));
    }

    auto db = drogon::app().getDbClient("default");
    if (!db) {
        // потом показать красивую html-ошибку, пока MVP.
        return fcb(drogon::HttpResponse::newRedirectionResponse("/manager/login?err=Ошибка%20БД"));
    }

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
                return fcb(drogon::HttpResponse::newRedirectionResponse("/manager/login?err=Сессия%20истекла"));
            }

            const auto userId = r[0]["id"].as<long long>();
            const auto role = r[0]["role"].as<std::string>();

            req->attributes()->insert("user_id", userId);
            req->attributes()->insert("role", role);

            fccb();
        },
        [fcb](const drogon::orm::DrogonDbException& e) {
            // Тоже MVP: не рисую красиво — 500 текстом.
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k500InternalServerError);
            resp->setContentTypeCode(drogon::CT_TEXT_PLAIN);
            resp->setBody(std::string("Ошибка БД: ") + e.base().what());
            fcb(resp);
        },
        sid
    );
}
