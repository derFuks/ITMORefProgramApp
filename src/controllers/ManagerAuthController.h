#pragma once

#include <drogon/HttpController.h>

class ManagerAuthController : public drogon::HttpController<ManagerAuthController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ManagerAuthController::loginPage, "/manager/login", drogon::Get);
    ADD_METHOD_TO(ManagerAuthController::loginPost, "/manager/login", drogon::Post);
    ADD_METHOD_TO(ManagerAuthController::logout,    "/manager/logout", drogon::Post);
    METHOD_LIST_END

    void loginPage(const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& cb);

    void loginPost(const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& cb);

    void logout(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& cb);
};