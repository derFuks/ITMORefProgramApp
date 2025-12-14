#include "ManagerSsrController.h"
#include <drogon/drogon.h>
#include <json/json.h>
#include <sstream>

#include "service/RequestService.h"
#include "domain/RequestFilter.h"

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
    const auto apiKey = getStrQ(req, "api_key"); // потом убрать, когда ключ через куки доставать буду
    RequestFilter filter;
    const auto status = getStrQ(req, "status");
    if (!status.empty()) filter.status = status;

    filter.limit = std::min(100, getIntQ(req, "limit", 20));
    filter.offset = std::max(0, getIntQ(req, "offset", 0));

    auto db = drogon::app().getDbClient("default");
    RequestService svc(db);
    auto items = svc.list(filter); // Json::arrayValue

    std::ostringstream html;
    html << "<!doctype html><html><head><meta charset='utf-8'>"
         << "<title>Менеджер: список з</title>"
         << "<style>"
         << "body{font-family:Arial, sans-serif; padding:20px;}"
         << "table{border-collapse:collapse; width:100%;}"
         << "th,td{border:1px solid #ddd; padding:8px;}"
         << "th{background:#f5f5f5; text-align:left;}"
         << "form.inline{display:inline; margin:0;}"
         << "button{padding:6px 10px;}"
         << ".muted{color:#777;}"
         << "</style></head><body>";

    html << "<h1>Валидация заявок и их оплата</h1>";
    html << "<p class=''>На этой странице реализована ручная валидация заявок от пациентов. В следующих релизах планируется прямая интеграция с МИС.</p>";

    // Фильтр по статусу
    html << "<form method='GET' action='/manager/requests'>"
         << "<input type='hidden' name='api_key' value='" << apiKey << "'/>"
         << "<label>Фильтр по статусу: </label>"
    
         << "<select name='status'>"
         << "<option value='' " << (status.empty() ? "selected" : "") << ">Все</option>"
         << "<option value='new' " << (status=="new" ? "selected" : "") << ">Новые</option>"
         << "<option value='paid' " << (status=="paid" ? "selected" : "") << ">Оплаченные</option>"
         << "<option value='cancelled' " << (status=="cancelled" ? "selected" : "") << ">Отклоненные</option>"
         << "</select> "
         << "<input type='hidden' name='limit' value='" << filter.limit << "'/>"
         << "<button type='submit'>Применить</button>"
         << "</form>";

    html << "<p class='muted'>Временный режим разработки: ключ API перекидываю через параметр <b>api_key</b> в URL. В следующих релизах переделаю через cookie.</p>";

    html << "<table><thead><tr>"
         << "<th>ID</th><th>Телефон</th><th>Услуга</th><th>Статус</th><th>Цена, руб.</th><th>Дата создания заявки</th><th>Дата оплаты партнеру</th><th>Действия</th>"
         << "</tr></thead><tbody>";

    for (const auto& it : items) {
        const auto id = it["id"].asInt64();
        const auto phone = it["phone"].asString();
        const auto service = it["service"].asString();
        const auto st = it["status"].asString();
        const auto createdAt = it["created_at"].asString();
        const auto paidAt = it["paid_at"].isNull() ? "" : it["paid_at"].asString();

        std::string priceStr = it["price"].isNull() ? "" : std::to_string(it["price"].asDouble());

        html << "<tr>";
        html << "<td>" << id << "</td>";
        html << "<td>" << phone << "</td>";
        html << "<td>" << service << "</td>";
        html << "<td>" << st << "</td>";
        html << "<td>" << priceStr << "</td>";
        html << "<td>" << createdAt << "</td>";
        html << "<td>" << paidAt << "</td>";
        html << "<td>";

        if (st == "new") {
                html << "<form class='inline' method='POST' action='/manager/requests/" << id
                     << "/mark-paid?api_key=" << apiKey << "'>"
                     << "<button type='submit'>Отметить как оплачено</button>"
                     << "</form>";
                     } else {
            html << "<span class='muted'>—</span>";
        }

        html << "</td>";
        html << "</tr>";
    }

    html << "</tbody></table>";
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

    // SSR-форма без перерисовывания цены, берём base_price на момент заявки
    (void)svc.markPaid(static_cast<long>(requestId), std::nullopt);

    // Возвращаемся обратно сохраняя api_key. временный костыль)
    const auto apiKey = getStrQ(req, "api_key");
    auto url = std::string("/manager/requests?api_key=") + apiKey;
    auto resp = drogon::HttpResponse::newRedirectionResponse(url);
    cb(resp);
}
