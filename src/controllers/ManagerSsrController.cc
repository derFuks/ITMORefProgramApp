#include "ManagerSsrController.h"
#include <drogon/drogon.h>
#include <json/json.h>
#include <sstream>
#include <iomanip>

#include "service/RequestService.h"
#include "domain/RequestFilter.h"
#include "utils/LayoutHelper.h"

static int getIntQ(const drogon::HttpRequestPtr& req, const std::string& k, int def) {
    auto qp = req->getParameters();
    auto it = qp.find(k);
    if (it == qp.end()) return def;
    try { return std::stoi(it->second); } catch (...) { return def; }
}

static std::string getStrQ(const drogon::HttpRequestPtr& req, const std::string& k) {
    auto qp = req->getParameters();
    auto it = qp.find(k);
    return (it == qp.end()) ? "" : it->second;
}

void ManagerSsrController::requestsPage(const drogon::HttpRequestPtr& req,
                                        std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    RequestFilter filter;
    const auto status = getStrQ(req, "status");
    if (!status.empty()) filter.status = status;

    filter.limit = std::min(100, getIntQ(req, "limit", 20));
    filter.offset = std::max(0, getIntQ(req, "offset", 0));

    auto db = drogon::app().getDbClient("default");
    RequestService svc(db);
    auto items = svc.list(filter); // Json::arrayValue

    std::ostringstream html;
    html << layout::renderHeader("manager");

    html << "<div class='card'>";
    html << "<h1 class='h1'>Валидация заявок и их оплата</h1>";
    html << "<p class='p'>На этой странице реализована ручная валидация заявок от пациентов. В следующих релизах планируется прямая интеграция с МИС.</p>";

    // Фильтр по статусу
    html << "<form class='formRow' method='GET' action='/manager/requests'>"
         << "<label class='labelInline'>Фильтр по статусу:</label>"
         << "<select name='status'>"
         << "<option value='' " << (status.empty() ? "selected" : "") << ">Все</option>"
         << "<option value='new' " << (status=="new" ? "selected" : "") << ">Новые</option>"
         << "<option value='paid' " << (status=="paid" ? "selected" : "") << ">Оплаченные</option>"
         << "<option value='cancelled' " << (status=="cancelled" ? "selected" : "") << ">Отклоненные</option>"
         << "</select> "
         << "<input type='hidden' name='limit' value='" << filter.limit << "'/>"
         << "<button class='btn btn-outline' type='submit'>Применить</button>"
         << "</form>";

    html << "<p class='p muted'>Авторизация менеджера сделана через cookie-сессию (MVP). Потом можно заменить вход на email/password + bcrypt.</p>";

    html << "<table><thead><tr>"
         << "<th>ID</th><th>Телефон</th><th>kev_id</th><th>Услуга</th><th>Статус</th><th>Цена, руб.</th><th>Дата создания заявки</th><th>Дата оплаты партнеру</th><th>Действия</th>"
         << "</tr></thead><tbody>";

    for (const auto& it : items) {
        const auto id = it["id"].asInt64();
        const auto phone = it["phone"].asString();
        const auto kevId = it["kev_id"].isNull() ? "" : it["kev_id"].asString();
        const auto service = it["service"].asString();
        const auto st = it["status"].asString();
        const auto createdAt = it["created_at"].asString();
        const auto paidAt = it["paid_at"].isNull() ? "" : it["paid_at"].asString();

        // std::to_string печатает 6 знаков после запятой, до 2 знаков сокращаю.
        std::string priceStr;
        if (it["price"].isNull()) {
            priceStr = "";
        } else {
            std::ostringstream ps;
            ps.setf(std::ios::fixed);
            ps << std::setprecision(2) << it["price"].asDouble();
            priceStr = ps.str();
        }

        html << "<tr>";
        html << "<td>" << id << "</td>";
        html << "<td>" << phone << "</td>";
        if (kevId.empty()) {
            html << "<td><span class='muted'>нет</span></td>";
        } else {
            html << "<td>" << kevId << "</td>";
        }
        html << "<td>" << service << "</td>";
        html << "<td>" << st << "</td>";
        html << "<td>" << priceStr << "</td>";
        html << "<td>" << createdAt << "</td>";
        html << "<td>" << paidAt << "</td>";
        html << "<td>";

        if (st == "new") {
                html << "<form class='inline' method='POST' action='/manager/requests/" << id
                     << "/mark-paid" << "'>"
                     << "<button type='submit'>Отметить как оплачено</button>"
                     << "</form>";
                html << "<form class='inline' method='POST' action='/manager/requests/" << id
                     << "/reject'"
                     << ">"
                     << "<button class='btn-danger' type='submit'>Отклонить оплату</button>"
                     << "</form>";
                     } else {
            html << "<span class='muted'>—</span>";
        }

        html << "</td>";
        html << "</tr>";
    }

    html << "</tbody></table>";

    html << layout::renderFooter();
    html << "</body></html>";

    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setContentTypeCode(drogon::CT_TEXT_HTML);
    resp->setBody(html.str());
    cb(resp);
}



void ManagerSsrController::markPaidPost(const drogon::HttpRequestPtr& req,
                                        std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                                        long long requestId) {
    auto db = drogon::app().getDbClient("default");
    RequestService svc(db);

    // SSR-форма без перерисовывания цены, берем base_price на момент заявки
    (void)svc.markPaid(static_cast<long>(requestId), std::nullopt);

    auto resp = drogon::HttpResponse::newRedirectionResponse("/manager/requests");
    cb(resp);
}

void ManagerSsrController::rejectPaidPost(const drogon::HttpRequestPtr& req,
                                         std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                                         long long requestId) {
    auto db = drogon::app().getDbClient("default");
    RequestService svc(db);

    // Пока просто статус >> cancelled. Потом добавить причину отказа и в БД.
    (void)svc.markCancelled(static_cast<long>(requestId));

    auto resp = drogon::HttpResponse::newRedirectionResponse("/manager/requests");
    cb(resp);
}