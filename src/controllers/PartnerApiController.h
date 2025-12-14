#pragma once
#include <drogon/HttpController.h>

class PartnerApiController : public drogon::HttpController<PartnerApiController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(PartnerApiController::myRequests, "/api/partner/requests", drogon::Get, "AuthFilter");
    ADD_METHOD_TO(PartnerApiController::myRewards,  "/api/partner/rewards",  drogon::Get, "AuthFilter");
    METHOD_LIST_END

    void myRequests(const drogon::HttpRequestPtr& req,
                    std::function<void(const drogon::HttpResponsePtr&)>&& cb);

    void myRewards(const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& cb);
};