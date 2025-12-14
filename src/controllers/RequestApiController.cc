#include "RequestApiController.h"
#include <drogon/drogon.h>
#include <json/json.h>

#include "service/RequestService.h"

static drogon::HttpResponsePtr jsonResponse(int code, const Json::Value& body) {
    auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
    resp->setStatusCode(static_cast<drogon::HttpStatusCode>(code));
    return resp;
}

void RequestApiController::create(const drogon::HttpRequestPtr& req,
                                  std::function<void (const drogon::HttpResponsePtr&)>&& callback) {
    auto json = req->getJsonObject();
    if (!json) {
        Json::Value out;
        out["ok"] = false;
        out["error"] = "Битый JSON";
        return callback(jsonResponse(400, out));
    }

    const auto phone = (*json).get("phone", "").asString();
    const auto serviceId = (*json).get("service_id", 0).asInt64();
    const auto visitAt = (*json).get("visit_at", "").asString();

    if (phone.empty() || serviceId <= 0 || visitAt.empty()) {
        Json::Value out;
        out["ok"] = false;
        out["error"] = "Обязательно нужно указать phone, service_id, visit_at";
        return callback(jsonResponse(400, out));
    }

    std::optional<long> doctorId;
    if ((*json).isMember("doctor_id") && !(*json)["doctor_id"].isNull())
        doctorId = (*json)["doctor_id"].asInt64();

    std::optional<std::string> fullName;
    if ((*json).isMember("full_name") && !(*json)["full_name"].isNull())
        fullName = (*json)["full_name"].asString();

    std::optional<long> partnerId;
    if ((*json).isMember("partner_id") && !(*json)["partner_id"].isNull())
        partnerId = (*json)["partner_id"].asInt64();

    std::optional<std::string> partnerCode;
    if ((*json).isMember("partner_code") && !(*json)["partner_code"].isNull())
        partnerCode = (*json)["partner_code"].asString();

    auto db = drogon::app().getDbClient("default");
    if (!db) {
        Json::Value out;
        out["ok"] = false;
        out["error"] = "Клиент БД не сконфигурирован.";
        return callback(jsonResponse(500, out));
    }

    RequestService svc(db);
    auto res = svc.createRequest(phone, serviceId, doctorId, visitAt, fullName, partnerId, partnerCode);

    Json::Value out;
    out["ok"] = res.ok;
    if (res.ok && res.requestId.has_value()) {
        out["request_id"] = Json::Int64(*res.requestId);
    } else {
        out["error"] = res.message;
    }

    callback(jsonResponse(res.ok ? 200 : 400, out));
}

void RequestApiController::markPaid(const drogon::HttpRequestPtr& req,
                                    std::function<void (const drogon::HttpResponsePtr&)>&& callback,
                                    long long requestId) {
    std::optional<double> priceOverride;

    // JSON опционален
    if (auto json = req->getJsonObject(); json) {
        if ((*json).isMember("price") && !(*json)["price"].isNull()) {
            priceOverride = (*json)["price"].asDouble();
        }
    }

    auto db = drogon::app().getDbClient("default");
    if (!db) {
        Json::Value out;
        out["ok"] = false;
        out["error"] = "Клиент БД не сконфигурирован.";
        return callback(jsonResponse(500, out));
    }

    RequestService svc(db);
    auto res = svc.markPaid(static_cast<long>(requestId), priceOverride);

    Json::Value out;
    out["ok"] = res.ok;
    if (!res.ok) out["error"] = res.message;

    callback(jsonResponse(res.ok ? 200 : 400, out));
}

void RequestApiController::list(
    const drogon::HttpRequestPtr& req,
    std::function<void (const drogon::HttpResponsePtr&)>&& callback
) {
    RequestFilter filter;

    auto qp = req->getParameters();

    if (qp.find("status") != qp.end())
        filter.status = qp.at("status");

    if (qp.find("date_from") != qp.end())
        filter.dateFrom = qp.at("date_from");

    if (qp.find("date_to") != qp.end())
        filter.dateTo = qp.at("date_to");

    if (qp.find("limit") != qp.end())
        filter.limit = std::min(100, std::stoi(qp.at("limit")));

    if (qp.find("offset") != qp.end())
        filter.offset = std::max(0, std::stoi(qp.at("offset")));

    auto db = drogon::app().getDbClient("default");
    RequestService svc(db);

    Json::Value out;
    out["ok"] = true;
    out["items"] = svc.list(filter);

    callback(jsonResponse(200, out));
}
