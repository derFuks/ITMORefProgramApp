#include "PublicRequestController.h"

#include <drogon/drogon.h>

#include <algorithm>
#include <cctype>
#include <sstream>
#include "service/RequestService.h"
#include "utils/LayoutHelper.h"

#include <string>

// MVP: простая экранизация HTML, чтобы не получить XSS через поля формы.
// Потом можно вынести в отдельный util-файл.
static std::string htmlEscape(const std::string &s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
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

static std::string getQp(const drogon::HttpRequestPtr& req, const std::string& key) {
    auto qp = req->getParameters();
    auto it = qp.find(key);
    return (it == qp.end()) ? "" : it->second;
}

static std::string stripNonDigits(std::string s) {
    // MVP: просто выкидываем всё, кроме цифр ("+7 (...)" -> "7...")
    s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c) {
        return !std::isdigit(c);
    }), s.end());
    return s;
}

static drogon::HttpResponsePtr htmlResp(const std::string& body) {
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setContentTypeCode(drogon::CT_TEXT_HTML);
    resp->setBody(body);
    return resp;
}

struct LookupItem {
    long long id;
    std::string title;
};

static std::vector<LookupItem> loadServices(const drogon::orm::DbClientPtr& db) {
    std::vector<LookupItem> items;
    auto r = db->execSqlSync(
        R"SQL(
            SELECT id, name
            FROM services
            WHERE is_deleted = FALSE AND is_active = TRUE
            ORDER BY id ASC
        )SQL"
    );
    for (const auto& row : r) {
        items.push_back(LookupItem{row["id"].as<long long>(), row["name"].as<std::string>()});
    }
    return items;
}

static std::vector<LookupItem> loadDoctors(const drogon::orm::DbClientPtr& db) {
    std::vector<LookupItem> items;
    auto r = db->execSqlSync(
        R"SQL(
            SELECT id, first_name, last_name
            FROM doctors
            WHERE is_deleted = FALSE AND is_active = TRUE
            ORDER BY rating DESC, last_name ASC
        )SQL"
    );
    for (const auto& row : r) {
        const auto id = row["id"].as<long long>();
        const auto fn = row["first_name"].as<std::string>();
        const auto ln = row["last_name"].as<std::string>();
        items.push_back(LookupItem{id, ln + " " + fn});
    }
    return items;
}

void PublicRequestController::formPage(const drogon::HttpRequestPtr& req,
                                       std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto db = drogon::app().getDbClient("default");
    if (!db) {
        callback(htmlResp("<h1>500</h1><p>DB client is not configured</p>"));
        return;
    }

    const std::string kevId = getQp(req, "kev_id");
    const bool hasKev = !kevId.empty();

    // Для MVP всё строю прямо строками HTML, потом можно вынести в шаблоны.
    auto services = loadServices(db);
    auto doctors = loadDoctors(db);

    std::ostringstream html;
    html << layout::renderHeader(std::nullopt);

    html << "<div class='card'>";
    html << "<h1 class='h1'>Онлайн-запись пациента</h1>";

    if (!hasKev) {
        // Как поступаем, если нет kev_id:
        // 1) Заявку всё равно принимаем.
        // 2) partner_id будет NULL, kev_id будет NULL.
        // 3) Менеджер на своей странице видит "нет kev_id" и может обработать вручную.
        html << "<div class='alert alert-warn'>"
             << "<b>Ссылка без партнёрского кода.</b> "
             << "Если вы пришли не по реферальной ссылке, заявка будет создана без привязки к партнёру. "
             << "Менеджер увидит такие заявки отдельно и обработает вручную."
             << "</div>";
    } else {
        html << "<div class='alert alert-ok'>"
             << "Заявка будет привязана к партнёру (kev_id: <b>" << htmlEscape(kevId) << "</b>)."
             << "</div>";
    }

    html << "<form class='form' method='POST' action='/request'>";
    // kev_id передаём скрытым полем
    html << "<input type='hidden' name='kev_id' value='" << htmlEscape(kevId) << "'/>";

    html << "<div class='field'>"
         << "<label>ФИО</label>"
         << "<input name='full_name' type='text' placeholder='Иванов Иван Иванович'/>"
         << "</div>";

    html << "<div class='field'>"
         << "<label>Телефон</label>"
         << "<input name='phone' type='tel' placeholder='+7 999 000 11 22' required/>"
         << "<div class='hint'>Можно вводить с пробелами/скобками — я оставлю только цифры.</div>"
         << "</div>";

    html << "<div class='field'>"
         << "<label>Услуга</label>"
         << "<select name='service_id' required>";
    html << "<option value='' selected disabled>Выберите услугу</option>";
    for (const auto& s : services) {
        html << "<option value='" << s.id << "'>" << htmlEscape(s.title) << "</option>";
    }
    html << "</select></div>";

    html << "<div class='field'>"
         << "<label>Дата и время посещения</label>"
         << "<input name='visit_at' type='datetime-local' required/>"
         << "<div class='hint'>В MVP время не проверяем на рабочие часы — это можно добавить позже.</div>"
         << "</div>";

    html << "<div class='field'>"
         << "<label>Врач</label>"
         << "<select name='doctor_id' required>";
    html << "<option value='' selected disabled>Выберите врача</option>";
    for (const auto& d : doctors) {
        html << "<option value='" << d.id << "'>" << htmlEscape(d.title) << "</option>";
    }
    html << "</select></div>";

    html << "<div class='ctaRow'>"
         << "<button class='btn' type='submit'>Отправить заявку</button>"
         << "<a class='btn btn-outline' href='/'>На главную</a>"
         << "</div>";
    html << "</form>";

    html << "<div class='divider'></div>";
    html << "<div class='hint'>Технически: форма создаёт запись в таблице patient_requests со статусом <b>new</b>. "
         << "Оплата подтверждается менеджером вручную в его кабинете.</div>";

    html << "</div>"; // card
    html << layout::renderFooter();
    callback(htmlResp(html.str()));
}

void PublicRequestController::submitForm(const drogon::HttpRequestPtr& req,
                                         std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    auto db = drogon::app().getDbClient("default");
    if (!db) {
        callback(htmlResp("<h1>500</h1><p>DB client is not configured</p>"));
        return;
    }

    // Читаем параметры формы
    const std::string fullNameRaw = req->getParameter("full_name");
    std::string phone = req->getParameter("phone");
    const std::string serviceIdStr = req->getParameter("service_id");
    const std::string doctorIdStr = req->getParameter("doctor_id");
    const std::string visitAtLocal = req->getParameter("visit_at");
    const std::string kevIdRaw = req->getParameter("kev_id");

    phone = stripNonDigits(phone);

    if (phone.empty() || serviceIdStr.empty() || doctorIdStr.empty() || visitAtLocal.empty()) {
        callback(htmlResp("<h1>400</h1><p>Не хватает обязательных полей.</p><p><a href='/request'>Назад</a></p>"));
        return;
    }

    long serviceId = 0;
    long doctorId = 0;
    try {
        serviceId = std::stol(serviceIdStr);
        doctorId = std::stol(doctorIdStr);
    } catch (...) {
        callback(htmlResp("<h1>400</h1><p>Некорректные значения service_id/doctor_id.</p>"));
        return;
    }

    // HTML input datetime-local отдаёт формат "YYYY-MM-DDTHH:MM" (без таймзоны)
    // Для MVP добавлю ":00+03:00" (у нас часовой пояс Europe/Tallinn/МСК-подобный).
    // Потом лучше брать таймзону из конфигурации.
    std::string visitAtIso = visitAtLocal;
    if (visitAtIso.size() == 16) {
        visitAtIso += ":00+03:00";
    }

    std::optional<std::string> fullName;
    if (!fullNameRaw.empty()) fullName = fullNameRaw;

    std::optional<std::string> kevId;
    if (!kevIdRaw.empty()) kevId = kevIdRaw;

    RequestService svc(db);
    auto res = svc.createRequest(
        phone,
        serviceId,
        std::optional<long>(doctorId),
        visitAtIso,
        fullName,
        kevId,
        std::nullopt,
        std::nullopt
    );

    std::ostringstream html;
    html << layout::renderHeader(std::nullopt);

    html << "<div class='card'>";
    html << "<h1 class='h1'>Заявка отправлена</h1>";

    if (res.ok && res.requestId.has_value()) {
        html << "<div class='alert alert-ok'>"
             << "Спасибо! Заявка создана. Номер заявки: <b>" << *res.requestId << "</b>."
             << "</div>";
        if (kevId.has_value()) {
            html << "<p class='p'>Заявка привязана к партнёру (kev_id: <b>" << htmlEscape(*kevId) << "</b>).</p>";
        } else {
            html << "<p class='p muted'>Заявка создана без партнёра (kev_id отсутствовал в URL).</p>";
        }
        html << "<div class='ctaRow'>"
             << "<a class='btn' href='/request'>Создать ещё одну заявку</a>"
             << "<a class='btn btn-outline' href='/'>На главную</a>"
             << "</div>";
    } else {
        html << "<div class='alert alert-err'>"
             << "Не получилось создать заявку: <b>" << htmlEscape(res.message) << "</b>."
             << "</div>";
        html << "<div class='ctaRow'>"
             << "<a class='btn' href='/request'>Вернуться к форме</a>"
             << "<a class='btn btn-outline' href='/'>На главную</a>"
             << "</div>";
    }

    html << "</div>"; // card
    html << layout::renderFooter();
    callback(htmlResp(html.str()));
}
