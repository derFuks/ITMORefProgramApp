#include "PartnerSsrController.h"

#include <drogon/drogon.h>

#include <iomanip>
#include <sstream>

#include "service/PartnerService.h"
#include "utils/LayoutHelper.h"

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

    // Профиль партнёра (kev_id + телефон). Если вдруг не нашли — просто показываем пусто.
    // туду: вынести это в отдельную страницу "Инструменты".
    Json::Value profile;
    try {
        profile = svc.getMyProfile(userId);
    } catch (...) {
        profile["ok"] = false;
    }

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
    html << layout::renderHeader("partner");
    html << "<div class='card'>";
    html << "<h1 class='h1'>Кабинет партнера</h1>";
    renderTopNav(html, "requests");
    html << "</div>";


    // === Инструменты партнёра (ссылка + QR) ===
    if (profile.get("ok", false).asBool()) {
        const auto kevId = htmlEscape(profile.get("kev_id", "").asString());
        const auto phone = htmlEscape(profile.get("phone", "").asString());

        // NOTE: домен пока захардкожен, потом лучше вынести в config.
        const std::string refLink = std::string("https://cmrt.ru/?kev_id=") + kevId;

        // Для QR используем внешний генератор (быстро для MVP).
        // туду: сделать генерацию QR на нашей стороне и отдавать svg/png без внешнего сервиса.
        const std::string qrPngUrl = std::string("https://api.qrserver.com/v1/create-qr-code/?size=120x120&data=") + drogon::utils::urlEncode(refLink);
        const std::string qrSvgUrl = std::string("https://api.qrserver.com/v1/create-qr-code/?format=svg&size=220x220&data=") + drogon::utils::urlEncode(refLink);

        html << "<div class='card'>";
        html << "<div style='display:flex;gap:18px;align-items:flex-start;flex-wrap:wrap'>";

        html << "<div style='flex:1;min-width:320px'>";
        html << "<h2 style='margin:0 0 8px'>Моя реферальная ссылка</h2>";
        html << "<div class='muted' style='margin-bottom:10px'>"
             << "kev_id: <b>" << kevId << "</b>";
        if (!phone.empty()) {
            html << "&nbsp;&nbsp;•&nbsp;&nbsp;Телефон: <b>" << phone << "</b>";
        }
        html << "</div>";

        html << "<div style='display:flex;gap:10px;align-items:center;flex-wrap:wrap'>";
        html << "<input id='refLink' type='text' readonly value='" << htmlEscape(refLink) << "'/>";
        html << "<button class='btn' type='button' onclick='copyRefLink()'>Скопировать</button>";
        html << "</div>";
        html << "<div class='muted' style='margin-top:10px'>"
             << "QR можно скачать как SVG или PNG и использовать в полиграфии/на сайте.";
        html << "</div>";
        html << "</div>"; // left

        html << "<div style='min-width:240px'>";
        html << "<img alt='QR' src='" << htmlEscape(qrPngUrl) << "' width='220' height='220' style='border:1px solid #eee;border-radius:12px'/>";
        html << "<div style='display:flex;gap:10px;margin-top:10px'>";
        html << "<a class='btn' href='" << htmlEscape(qrSvgUrl) << "' target='_blank' rel='noopener'>SVG</a>";
        html << "<a class='btn' href='" << htmlEscape(qrPngUrl) << "' target='_blank' rel='noopener'>PNG</a>";
        html << "</div>";
        html << "</div>"; // right

        html << "</div>";

        html << "<script>\n"
             << "function copyRefLink(){\n"
             << "  var el=document.getElementById('refLink');\n"
             << "  el.select();\n"
             << "  el.setSelectionRange(0,99999);\n"
             << "  if (navigator.clipboard && navigator.clipboard.writeText) {\n"
             << "    navigator.clipboard.writeText(el.value).then(function(){alert('Ссылка скопирована');});\n"
             << "  } else {\n"
             << "    document.execCommand('copy');\n"
             << "    alert('Ссылка скопирована');\n"
             << "  }\n"
             << "}\n"
             << "</script>";

        html << "</div>"; // card
    }
    
    html << "<div class='card'>";
    html << "<h2 class='h2'>Список заявок</h2>";
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
    html << "<p class='p muted' style='margin-top:14px'>Пагинацию и фильтры добавлю в следующих итерациях.</p>";
    html << "</div>";

    html << layout::renderFooter();

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
    html << layout::renderHeader("partner");
    html << "<div class='card'>";
    html << "<h1 class='h1'>Кабинет партнёра</h1>";
    renderTopNav(html, "rewards");
    html << "</div>";

    html << "<div class='card'>";
    html << "<h2 class='h2'>Мои начисления</h2>";
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
    html << "<p class='p muted' style='margin-top:14px'>Тут тоже можно будет добавить сумму итого и экспорт в CSV, но позже.</p>";
    html << "</div>";
    html << layout::renderFooter();

    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setContentTypeCode(drogon::CT_TEXT_HTML);
    resp->setBody(html.str());
    cb(resp);
}
