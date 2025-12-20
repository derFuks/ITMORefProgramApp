#pragma once

#include <drogon/HttpController.h>

class PartnerAuthController : public drogon::HttpController<PartnerAuthController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(PartnerAuthController::loginPage, "/partner/login", drogon::Get);
    ADD_METHOD_TO(PartnerAuthController::loginPost, "/partner/login", drogon::Post);
    ADD_METHOD_TO(PartnerAuthController::logout,    "/partner/logout", drogon::Post);
    METHOD_LIST_END

    void loginPage(const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& cb);

    void loginPost(const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& cb);

    void logout(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& cb);
};
