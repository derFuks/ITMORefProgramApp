#pragma once

#include <drogon/HttpController.h>

// LandingController
// "Разводящая" посадочная страница:
//  - выбор как авторизоваться (партнёр/менеджер)
//  - переход к публичной форме /request
//  - базовые заглушки: /about и /contact
//
// MVP-реализация: HTML собираем строками. Потом можно вынести в шаблоны.

class LandingController : public drogon::HttpController<LandingController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(LandingController::index, "/", drogon::Get);
    // удобно иметь отдельный URL под кнопку "Личный кабинет", но страница та же.
    ADD_METHOD_TO(LandingController::cabinet, "/cabinet", drogon::Get);
    ADD_METHOD_TO(LandingController::about, "/about", drogon::Get);
    ADD_METHOD_TO(LandingController::contact, "/contact", drogon::Get);
    ADD_METHOD_TO(LandingController::stub, "/stub", drogon::Get);
    METHOD_LIST_END

    void index(const drogon::HttpRequestPtr &req,
               std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void cabinet(const drogon::HttpRequestPtr &req,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);


    void about(const drogon::HttpRequestPtr &req,
               std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void contact(const drogon::HttpRequestPtr &req,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void stub(const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};