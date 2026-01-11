#include "PartnerAuthController.h"

#include <drogon/drogon.h>

#include <random>
#include <sstream>

#include "utils/LayoutHelper.h"


// cкопировал с менеджерского входа
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

void PartnerAuthController::loginPage(const drogon::HttpRequestPtr& req,
                                      std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    const auto err = req->getParameter("err");

    std::ostringstream html;
    html << layout::renderHeader(std::nullopt);
    html << "<div class='card' style='max-width:640px;margin:0 auto'>";
    html << "<h1 class='h1'>Вход для партнёров</h1>";
    if (!err.empty()) html << "<div class='alert alert-err'>" << htmlEscape(err) << "</div>";

    html << "<form class='form' method='POST' action='/partner/login'>"
         << "<div class='field'>"
         << "<label>Ключ доступа (API key)</label>"
         << "<input name='api_key' type='password' placeholder='Введите ключ'/>"
         << "<div class='hint'>Это временно. В следующей итерации заменю на логин/пароль.</div>"
         << "</div>"
         << "<div class='ctaRow'>"
         << "<button class='btn' type='submit'>Войти</button>"
         << "<a class='btn btn-outline' href='/'>На главную</a>"
         << "</div>"
         << "</form>";

    html << "<p style='color:#777'>!Это MVP: вход по API key. Потом сделаем логин/пароль и восстановление.</p>";

    html << "</div>";
    html << layout::renderFooter();

    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setContentTypeCode(drogon::CT_TEXT_HTML);
    resp->setBody(html.str());
    cb(resp);
}

void PartnerAuthController::loginPost(const drogon::HttpRequestPtr& req,
                                      std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    const auto apiKey = req->getParameter("api_key");
    if (apiKey.empty()) {
        return cb(drogon::HttpResponse::newRedirectionResponse("/partner/login?err=Введите%20ключ"));
    }

    auto db = drogon::app().getDbClient("default");
    if (!db) {
        return cb(drogon::HttpResponse::newRedirectionResponse("/partner/login?err=Ошибка%20БД"));
    }

    // Проверка, что ключ активен и роль partner
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

        if (r.empty() || r[0]["role"].as<std::string>() != "partner") {
            return cb(drogon::HttpResponse::newRedirectionResponse("/partner/login?err=Неверный%20ключ"));
        }

        const auto userId = r[0]["id"].as<long long>();
        const auto sid = genSessionId();

        try {
            db->execSqlSync(
                R"SQL(
                    INSERT INTO partner_sessions(user_id, session_id, expires_at)
                    VALUES ($1, $2, now() + interval '7 days')
                )SQL",
                userId, sid
            );
        } catch (...) {
            return cb(drogon::HttpResponse::newRedirectionResponse("/partner/login?err=Не%20удалось%20создать%20сессию"));
        }

        auto resp = drogon::HttpResponse::newRedirectionResponse("/partner/requests");
        resp->addHeader(
            "Set-Cookie",
            "itmorefprog_partner_session=" + sid + "; Path=/; Max-Age=604800; HttpOnly; SameSite=Lax"
        );
        return cb(resp);
    } catch (...) {
        return cb(drogon::HttpResponse::newRedirectionResponse("/partner/login?err=Ошибка%20проверки%20ключа"));
    }
}

void PartnerAuthController::logout(const drogon::HttpRequestPtr& req,
                                   std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    auto sid = req->getCookie("itmorefprog_partner_session");
    if (!sid.empty()) {
        try {
            auto db = drogon::app().getDbClient("default");
            if (db) {
                db->execSqlSync(
                    "UPDATE partner_sessions SET is_active=FALSE WHERE session_id=$1",
                    sid
                );
            }
        } catch (...) {
            // MVP: ничего не делаю, главное что cookie сейчас снесём
        }
    }

    auto resp = drogon::HttpResponse::newRedirectionResponse("/partner/login");
    resp->addHeader(
        "Set-Cookie",
        "itmorefprog_partner_session=; Path=/; Max-Age=0; HttpOnly; SameSite=Lax"
    );
    cb(resp);
}
