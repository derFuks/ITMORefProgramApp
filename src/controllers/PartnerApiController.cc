#include "PartnerApiController.h"
#include <drogon/drogon.h>
#include <json/json.h>
#include "service/PartnerService.h"

static drogon::HttpResponsePtr jsonResp(int code, const Json::Value& body) {
    auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
    resp->setStatusCode(static_cast<drogon::HttpStatusCode>(code));
    return resp;
}

static int getIntQ(const drogon::HttpRequestPtr& req, const std::string& k, int def) {
    auto qp = req->getParameters();
    auto it = qp.find(k);
    if (it == qp.end()) return def;
    try { return std::stoi(it->second); } catch (...) { return def; }
}

void PartnerApiController::myRequests(const drogon::HttpRequestPtr& req,
                                      std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    Json::Value out;

    long long userId{};
    try { userId = req->attributes()->get<long long>("user_id"); }
    catch (...) { out["ok"]=false; out["error"]="Не авторизован"; return cb(jsonResp(401,out)); }

    const int limit = std::min(100, getIntQ(req, "limit", 20));
    const int offset = std::max(0, getIntQ(req, "offset", 0));

    auto db = drogon::app().getDbClient("default");
    PartnerService svc(db);

    try {
        out["ok"] = true;
        out["items"] = svc.getMyRequests(userId, limit, offset);
        cb(jsonResp(200, out));
    } catch (const std::exception& e) {
        out["ok"] = false;
        out["error"] = e.what();
        cb(jsonResp(400, out));
    }
}

void PartnerApiController::myRewards(const drogon::HttpRequestPtr& req,
                                     std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
    Json::Value out;

    long long userId{};
    try { userId = req->attributes()->get<long long>("user_id"); }
        catch (...) { out["ok"]=false; out["error"]="Не авторизован"; 
        return cb(jsonResp(401,out));
    }

    const int limit = std::min(100, getIntQ(req, "limit", 20));
    const int offset = std::max(0, getIntQ(req, "offset", 0));

    auto db = drogon::app().getDbClient("default");
    PartnerService svc(db);

    try {
        out["ok"] = true;
        out["items"] = svc.getMyRewards(userId, limit, offset);
        cb(jsonResp(200, out));
    } catch (const std::exception& e) {
        out["ok"] = false;
        out["error"] = e.what();
        cb(jsonResp(400, out));
    }
}