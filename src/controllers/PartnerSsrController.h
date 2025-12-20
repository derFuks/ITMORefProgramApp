#pragma once

#include <drogon/HttpController.h>

class PartnerSsrController : public drogon::HttpController<PartnerSsrController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(PartnerSsrController::requestsPage, "/partner/requests", drogon::Get, "AuthFilter", "PartnerOnlyFilter");
    ADD_METHOD_TO(PartnerSsrController::rewardsPage,  "/partner/rewards",  drogon::Get, "AuthFilter", "PartnerOnlyFilter");
    METHOD_LIST_END

    void requestsPage(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& cb);

    void rewardsPage(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& cb);
};
