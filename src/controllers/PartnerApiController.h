#pragma once
#include <drogon/HttpController.h>

class PartnerApiController : public drogon::HttpController<PartnerApiController> {
public:
    METHOD_LIST_BEGIN
    // Защита авторизацией и ролью.
    // пока менеджер может ходить с партнерским ключом, потом на логин/пароль переделаю
    ADD_METHOD_TO(PartnerApiController::myRequests, "/api/partner/requests", drogon::Get, "AuthFilter", "PartnerOnlyFilter");
    ADD_METHOD_TO(PartnerApiController::myRewards,  "/api/partner/rewards",  drogon::Get, "AuthFilter", "PartnerOnlyFilter");

    METHOD_LIST_END

    void myRequests(const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& cb);

    void myRewards(const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& cb);
};