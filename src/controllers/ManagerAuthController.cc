#include "ManagerAuthController.h"
#include <drogon/drogon.h>
#include <sstream>
#include <random>


static std::string genSessionId() {
    static const char* chars = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, 61);
    std::string s;
    s.reserve(48);
    for (int i = 0; i < 48; i++) s.push_back(chars[dist(gen)]);
    return s;
}

static std::string htmlEscape(const std::string& in) {
    std::string out;
    out.reserve(in.size());
    for (char c : in) {
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            case '\'': out += "&#39;"; break;
            default: out += c; break;
        }
    }
    return out;
}

void ManagerAuthController::loginPage(const drogon::HttpRequestPtr& req,
                                      std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    const auto err = req->getParameter("err");

    std::ostringstream html;
    html << "<!doctype html><html><head><meta charset='utf-8'>"
         << "<title>Менеджерский вход</title>"
         << "<style>"
         << "body{font-family:Arial,sans-serif;padding:24px;max-width:520px;margin:0 auto;}"
         << "input{width:100%;padding:10px;margin:8px 0;}"
         << "button{padding:10px 14px;}"
         << ".err{color:#b00020;margin:10px 0;}"
         << "</style></head><body>";

    html << "<h1>Вход для менеджеров</h1>";
    if (!err.empty()) html << "<div class='err'>" << htmlEscape(err) << "</div>";

    html << "<form method='POST' action='/manager/login'>"
         << "<label>Ключ доступа (API key)</label>"
         << "<input name='api_key' type='password' placeholder='Введите ключ'/>"
         << "<button type='submit'>Войти</button>"
         << "</form>";

    html << "<p style='color:#777'>!Это пока временная форма входа. Дальше надо заменить на логин/пас.</p>";

    html << "</body></html>";

    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setContentTypeCode(drogon::CT_TEXT_HTML);
    resp->setBody(html.str());
    cb(resp);
}

void ManagerAuthController::loginPost(const drogon::HttpRequestPtr& req,
                                      std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    const auto apiKey = req->getParameter("api_key");
    if (apiKey.empty()) {
        return cb(drogon::HttpResponse::newRedirectionResponse("/manager/login?err=Введите%20ключ"));
    }

    auto db = drogon::app().getDbClient("default");
    if (!db) {
        return cb(drogon::HttpResponse::newRedirectionResponse("/manager/login?err=Ошибка%20БД"));
    }

    // Проверка, что ключ активен и роль manager
    try {
        auto r = db->execSqlSync(
            R"SQL(
                SELECT u.id, u.role
                FROM user_api_keys k
                JOIN users u ON u.id = k.user_id
                WHERE k.api_key=$1 AND k.is_active=TRUE AND u.is_deleted=FALSE
                LIMIT 1
            )SQL",
            apiKey
        );

        if (r.empty() || r[0]["role"].as<std::string>() != "manager") {
            return cb(drogon::HttpResponse::newRedirectionResponse("/manager/login?err=Неверный%20ключ"));
        }

        const auto userId = r[0]["id"].as<long long>();
        const auto sid = genSessionId();

        try {
            db->execSqlSync(
                R"SQL(
                    INSERT INTO manager_sessions(user_id, session_id, expires_at)
                    VALUES ($1, $2, now() + interval '7 days')
                )SQL",
                userId, sid
            );
        } catch (...) {
            return cb(drogon::HttpResponse::newRedirectionResponse("/manager/login?err=Не%20удалось%20создать%20сессию"));
        }

        auto resp = drogon::HttpResponse::newRedirectionResponse("/manager/requests");
        resp->addHeader(
            "Set-Cookie",
            "itmorefprog_manager_session=" + sid + "; Path=/; Max-Age=604800; HttpOnly; SameSite=Lax"
        );
        return cb(resp);

    } catch (...) {
        return cb(drogon::HttpResponse::newRedirectionResponse("/manager/login?err=Ошибка%20проверки%20ключа"));
    }
}


void ManagerAuthController::logout(const drogon::HttpRequestPtr& req,
                                   std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    auto sid = req->getCookie("itmorefprog_manager_session");
    if (!sid.empty()) {
        try {
            auto db = drogon::app().getDbClient("default");
            if (db) {
                db->execSqlSync(
                    "UPDATE manager_sessions SET is_active=FALSE WHERE session_id=$1",
                    sid
                );
            }
        } catch (...) {
            // в этой версии MVP не обрабатываю, потом накидаю
        }
    }

    auto resp = drogon::HttpResponse::newRedirectionResponse("/manager/login");
    resp->addHeader(
        "Set-Cookie",
        "itmorefprog_manager_session=; Path=/; Max-Age=0; HttpOnly; SameSite=Lax"
    );
    cb(resp);
}
