#pragma once

#include <drogon/HttpController.h>
#include "utils/LayoutHelper.h"

// Итерация B: публичная форма записи пациента.
//
// Важное (MVP):
// - Страница открывается без авторизации.
// - kev_id берём из URL (?kev_id=XXXXXXX) и отправляем как hidden field.
// - Если kev_id нет, заявку всё равно создаём, но она будет "без партнёра".
//   На странице менеджера такие заявки подсвечиваем как "нет kev_id".

class PublicRequestController : public drogon::HttpController<PublicRequestController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(PublicRequestController::formPage, "/request", drogon::Get);
    ADD_METHOD_TO(PublicRequestController::submitForm, "/request", drogon::Post);
    METHOD_LIST_END

    void formPage(const drogon::HttpRequestPtr& req,
                  std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void submitForm(const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};
