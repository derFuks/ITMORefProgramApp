#include "ManagerOnlyFilter.h"
#include <drogon/drogon.h>
#include <json/json.h>

static drogon::HttpResponsePtr jsonErr(drogon::HttpStatusCode code, const std::string& msg) {
    Json::Value out;
    out["ok"] = false;
    out["error"] = msg;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(out);
    resp->setStatusCode(code);
    return resp;
}



void ManagerOnlyFilter::doFilter(const drogon::HttpRequestPtr& req,
                                 drogon::FilterCallback&& fcb,
                                 drogon::FilterChainCallback&& fccb) {
    try {
        const auto role = req->attributes()->get<std::string>("role");
        if (role != "manager") {
            return fcb(jsonErr(drogon::k403Forbidden, "Нужна роль Manager"));
        }
        fccb();
    } catch (...) {
        fcb(jsonErr(drogon::k401Unauthorized, "Not authorized"));
    }
}
