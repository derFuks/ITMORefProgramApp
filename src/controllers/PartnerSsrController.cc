#include "PartnerSsrController.h"

#include <drogon/drogon.h>

#include <iomanip>
#include <sstream>

#include "service/PartnerService.h"

static std::string fmtMoney(const Json::Value& v) {
    if (v.isNull()) return "";
    // туду: в идеале хранить деньги в NUMERIC и форматировать через локаль, но пока так
    try {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << v.asDouble();
        return oss.str();
    } catch (...) {
        return "";
    }
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

static void renderTopNav(std::ostringstream& html, const std::string& active) {
    html << "<div style='display:flex;gap:10px;align-items:center;margin-bottom:14px'>";

    auto linkStyle = [&](const std::string& name) {
        if (name == active) return std::string("font-weight:bold;text-decoration:none;color:#111");
        return std::string("text-decoration:none;color:#555");
    };

    html << "<a href='/partner/requests' style='" << linkStyle("requests") << "'>Заявки</a>";
    html << "<a href='/partner/rewards' style='"  << linkStyle("rewards")  << "'>Начисления</a>";

    html << "<div style='flex:1'></div>";
    html << "<form method='POST' action='/partner/logout' style='margin:0'>"
         << "<button type='submit' style='padding:8px 12px;border:1px solid #ddd;background:#fff;border-radius:8px;cursor:pointer'>Выйти</button>"
         << "</form>";

    html << "</div>";
}

void PartnerSsrController::requestsPage(const drogon::HttpRequestPtr& req,
                                       std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    long long userId{};
    try {
        userId = req->attributes()->get<long long>("user_id");
    } catch (...) {
        return cb(drogon::HttpResponse::newRedirectionResponse("/partner/login"));
    }

    auto db = drogon::app().getDbClient("default");
    PartnerService svc(db);

    Json::Value items;
    try {
        items = svc.getMyRequests(userId, 50, 0);
    } catch (const std::exception& e) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->setContentTypeCode(drogon::CT_TEXT_PLAIN);
        resp->setBody(std::string("Ошибка: ") + e.what());
        return cb(resp);
    }

    std::ostringstream html;
    html << "<!doctype html><html><head><meta charset='utf-8'>"
         << "<title>Кабинет партнера ЦМРТ - заявки</title>"
         << "<style>"
         << "body{font-family:Arial,sans-serif;padding:24px;max-width:1100px;margin:0 auto;}"
         << "table{border-collapse:collapse;width:100%;}"
         << "th,td{border-bottom:1px solid #eee;padding:10px;text-align:left;font-size:14px;}"
         << "th{color:#666;font-weight:600;}"
         << ".pill{display:inline-block;padding:4px 8px;border:1px solid #ddd;border-radius:999px;font-size:12px;color:#444;background:#fafafa;}"
         << "</style></head><body>";

    html << "<h1 style='margin:0 0 10px'>Кабинет партнера</h1>";
    renderTopNav(html, "requests");

    html << "<h2 style='margin:16px 0 10px'>Спосок заявок</h2>";
    html << "<table><thead><tr>"
         << "<th>ID</th><th>Телефон</th><th>Услуга</th><th>Статус</th><th>Цена</th><th>Создано</th><th>Оплачено</th>"
         << "</tr></thead><tbody>";

    for (const auto& it : items) {
        const auto id = it["id"].asInt64();
        const auto phone = htmlEscape(it.get("phone", "").asString());
        const auto service = htmlEscape(it.get("service", "").asString());
        const auto status = htmlEscape(it.get("status", "").asString());
        const auto price = fmtMoney(it["price"]);
        const auto createdAt = htmlEscape(it.get("created_at", "").asString());
        const auto paidAt = it["paid_at"].isNull() ? "" : htmlEscape(it["paid_at"].asString());

        html << "<tr>"
             << "<td>" << id << "</td>"
             << "<td>" << phone << "</td>"
             << "<td>" << service << "</td>"
             << "<td><span class='pill'>" << status << "</span></td>"
             << "<td>" << price << "</td>"
             << "<td>" << createdAt << "</td>"
             << "<td>" << paidAt << "</td>"
             << "</tr>";
    }

    html << "</tbody></table>";
    html << "<p style='color:#777;margin-top:14px'>Пагинацию и фильтры добавлю в следующих итерациях.</p>";

    html << "</body></html>";

    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setContentTypeCode(drogon::CT_TEXT_HTML);
    resp->setBody(html.str());
    cb(resp);
}

void PartnerSsrController::rewardsPage(const drogon::HttpRequestPtr& req,
                                      std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    long long userId{};
    try {
        userId = req->attributes()->get<long long>("user_id");
    } catch (...) {
        return cb(drogon::HttpResponse::newRedirectionResponse("/partner/login"));
    }

    auto db = drogon::app().getDbClient("default");
    PartnerService svc(db);

    Json::Value items;
    try {
        items = svc.getMyRewards(userId, 50, 0);
    } catch (const std::exception& e) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->setContentTypeCode(drogon::CT_TEXT_PLAIN);
        resp->setBody(std::string("Ошибка: ") + e.what());
        return cb(resp);
    }

    std::ostringstream html;
    html << "<!doctype html><html><head><meta charset='utf-8'>"
         << "<title>Кабинет партнера — начисления</title>"
         << "<style>"
         << "body{font-family:Arial,sans-serif;padding:24px;max-width:1100px;margin:0 auto;}"
         << "table{border-collapse:collapse;width:100%;}"
         << "th,td{border-bottom:1px solid #eee;padding:10px;text-align:left;font-size:14px;}"
         << "th{color:#666;font-weight:600;}"
         << "</style></head><body>";

    html << "<h1 style='margin:0 0 10px'>Кабинет партнёра</h1>";
    renderTopNav(html, "rewards");

    html << "<h2 style='margin:16px 0 10px'>Мои начисления</h2>";
    html << "<table><thead><tr>"
         << "<th>ID</th><th>ID заявки</th><th>Ставка</th><th>Сумма</th><th>Создано</th>"
         << "</tr></thead><tbody>";

    for (const auto& it : items) {
        const auto id = it["id"].asInt64();
        const auto reqId = it["patient_request_id"].asInt64();
        const auto rate = fmtMoney(it["reward_rate"]);
        const auto amount = fmtMoney(it["amount"]);
        const auto createdAt = htmlEscape(it.get("created_at", "").asString());

        html << "<tr>"
             << "<td>" << id << "</td>"
             << "<td>" << reqId << "</td>"
             << "<td>" << rate << "</td>"
             << "<td>" << amount << "</td>"
             << "<td>" << createdAt << "</td>"
             << "</tr>";
    }

    html << "</tbody></table>";
    html << "<p style='color:#777;margin-top:14px'>Тут тоже можно будет добавить сумму итого и экспорт в CSV, но позже.</p>";
    html << "</body></html>";

    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setContentTypeCode(drogon::CT_TEXT_HTML);
    resp->setBody(html.str());
    cb(resp);
}
