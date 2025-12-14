#pragma once
#include <drogon/HttpController.h>

class ManagerSsrController : public drogon::HttpController<ManagerSsrController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ManagerSsrController::requestsPage, "/manager/requests", drogon::Get,  "AuthFilter", "ManagerOnlyFilter");
    ADD_METHOD_TO(ManagerSsrController::markPaidPost, "/manager/requests/{1}/mark-paid", drogon::Post, "AuthFilter", "ManagerOnlyFilter");
    METHOD_LIST_END

    void requestsPage(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& cb);

    void markPaidPost(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& cb,
                      long long requestId);

};