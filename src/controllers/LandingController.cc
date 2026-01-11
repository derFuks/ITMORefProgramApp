#include "LandingController.h"

#include <drogon/drogon.h>
#include <sstream>

#include "utils/LayoutHelper.h"

using namespace drogon;

// MVP: простая экранизация HTML.
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

static HttpResponsePtr htmlResp(const std::string &body) {
    auto resp = HttpResponse::newHttpResponse();
    resp->setContentTypeCode(CT_TEXT_HTML);
    resp->setBody(body);
    return resp;
}

static void renderStubPage(const HttpRequestPtr &req,
                           std::function<void(const HttpResponsePtr &)> &&callback,
                           const std::string &title,
                           const std::string &text) {
    (void)req;
    std::ostringstream html;
    html << layout::renderHeader(std::nullopt);
    html << "<div class='card'>";
    html << "<h1 class='h1'>" << htmlEscape(title) << "</h1>";
    html << "<p class='p'>" << htmlEscape(text) << "</p>";
    html << "<div class='ctaRow'><a class='btn btn-outline' href='/'>На главную</a></div>";
    html << "</div>";
    html << layout::renderFooter();
    callback(htmlResp(html.str()));
}

void LandingController::index(const HttpRequestPtr &req,
                             std::function<void(const HttpResponsePtr &)> &&callback) {
    (void)req;

    std::ostringstream html;
    html << layout::renderHeader(std::nullopt);

    html << "<div class='grid2'>";

    // Левая колонка
    html << "<div class='card'>";
    html << "<h1 class='h1'>Реферальная программа ЦМРТ</h1>";
    html << "<p class='p'>Партнёры ЦМРТ получают вознаграждение за привлечение новых пациентов. "
            "Пациент записывается по выделенному номеру телефона или по персональной ссылке партнёра.</p>";

    html << "<ul class='list'>"
            "<li><b>Прозрачно:</b> заявка фиксируется в системе и доступна менеджеру и партнёру.</li>"
            "<li><b>Удобно:</b> партнёр получает ссылку и QR-код, пациент — простую форму записи.</li>"
            "<li><b>Гибко:</b> условия вознаграждения зависят от услуги (МРТ, консультации и т.д.).</li>"
            "</ul>";

    html << "<div class='ctaRow'>"
            "<a class='btn' href='/request'>Записать пациента</a>"
            "<a class='btn btn-outline' href='/cabinet'>Личный кабинет</a>"
            "</div>";
    html << "</div>";

    // Правая колонка
    html << "<div class='card'>";
    html << "<h2 class='h2'>Выберите вход</h2>";
    html << "<p class='p muted'>Для демонстрации MVP — два кабинета: партнёр и менеджер.</p>";
    html << "<div class='stack'>"
            "<a class='btn' href='/partner/login'>Вход партнёра</a>"
            "<a class='btn btn-outline' href='/manager/login'>Вход менеджера</a>"
            "</div>";

    html << "<div class='divider'></div>";
    html << "<p class='p muted'>Если вы обычный пользователь и хотите просто записаться — используйте кнопку ниже.</p>";
    html << "<a class='btn' href='/request'>Перейти к записи</a>";
    html << "</div>";

    html << "</div>"; // grid2

    html << "<div class='grid3'>"
            "<div class='mini'><h3>1. Партнёр делится ссылкой</h3><p>Личная ссылка или QR-код ведут на сайт с параметром kev_id.</p></div>"
            "<div class='mini'><h3>2. Пациент оставляет заявку</h3><p>ФИО, телефон, услуга, дата/время и врач — всё в одном шаге.</p></div>"
            "<div class='mini'><h3>3. Менеджер подтверждает оплату</h3><p>После факта оплаты заявка переводится в paid и начисляется вознаграждение.</p></div>"
            "</div>";

    html << layout::renderFooter();
    callback(htmlResp(html.str()));
}

void LandingController::cabinet(const HttpRequestPtr &req,
                               std::function<void(const HttpResponsePtr &)> &&callback) {
    // Кабинет-выбор — тот же index
    index(req, std::move(callback));
}

void LandingController::about(const HttpRequestPtr &req,
                             std::function<void(const HttpResponsePtr &)> &&callback) {
    renderStubPage(req, std::move(callback), "О реферальной программе",
                   "Страница-заглушка. В следующей итерации тут будет подробное описание условий и правил.");
}

void LandingController::contact(const HttpRequestPtr &req,
                               std::function<void(const HttpResponsePtr &)> &&callback) {
    renderStubPage(req, std::move(callback), "Связаться с менеджером",
                   "Страница-заглушка. В следующей итерации добавим форму обращения или контактный телефон.");
}

void LandingController::stub(const HttpRequestPtr &req,
                            std::function<void(const HttpResponsePtr &)> &&callback) {
    renderStubPage(req, std::move(callback), "Раздел в разработке",
                   "Пока это заглушка. Позже здесь появится реальная страница.");
}
