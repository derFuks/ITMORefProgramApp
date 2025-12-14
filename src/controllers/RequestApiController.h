#pragma once
#include <drogon/HttpController.h>

class RequestApiController : public drogon::HttpController<RequestApiController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(RequestApiController::create, "/api/requests", drogon::Post); // пока публичным оставляю
    // ADD_METHOD_TO(RequestApiController::markPaid, "/api/requests/{1}/mark-paid", drogon::Post);
    // ADD_METHOD_TO(RequestApiController::list, "/api/requests", drogon::Get);
    ADD_METHOD_TO(RequestApiController::markPaid, "/api/requests/{1}/mark-paid", drogon::Post, "AuthFilter", "ManagerOnlyFilter");
    ADD_METHOD_TO(RequestApiController::list, "/api/requests", drogon::Get, "AuthFilter", "ManagerOnlyFilter");

    METHOD_LIST_END

    void markPaid(const drogon::HttpRequestPtr& req,
              std::function<void (const drogon::HttpResponsePtr&)>&& callback,
              long long requestId);

    void create(const drogon::HttpRequestPtr& req,
                std::function<void (const drogon::HttpResponsePtr&)>&& callback);
    
                void list(const drogon::HttpRequestPtr& req,
          std::function<void (const drogon::HttpResponsePtr&)>&& callback);

};
