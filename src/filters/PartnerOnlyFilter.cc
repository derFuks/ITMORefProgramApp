#include "PartnerOnlyFilter.h"

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

static bool pathStartsWith(const drogon::HttpRequestPtr& req, const std::string& prefix) {
    const auto& p = req->path();
    return p.rfind(prefix, 0) == 0;
}

void PartnerOnlyFilter::doFilter(const drogon::HttpRequestPtr& req,
                                 drogon::FilterCallback&& fcb,
                                 drogon::FilterChainCallback&& fccb) {
    try {
        const auto role = req->attributes()->get<std::string>("role");
        if (role != "partner") {
            // для SSR partner я лучше отправлю на /partner/login
            if (pathStartsWith(req, "/partner/")) {
                return fcb(drogon::HttpResponse::newRedirectionResponse("/partner/login"));
            }
            return fcb(jsonErr(drogon::k403Forbidden, "Нужна роль Partner"));
        }
        fccb();
    } catch (...) {
        if (pathStartsWith(req, "/partner/")) {
            return fcb(drogon::HttpResponse::newRedirectionResponse("/partner/login"));
        }
        fcb(jsonErr(drogon::k401Unauthorized, "Not authorized"));
    }
}
