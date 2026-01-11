#pragma once

#include <optional>
#include <string>
#include <drogon/orm/DbClient.h>
#include "domain/OperationResult.h"
#include "domain/CreateRequestResult.h"
#include <json/json.h>
#include "domain/RequestFilter.h"

class PatientRequestRepository {
public:
    explicit PatientRequestRepository(drogon::orm::DbClientPtr db);

    // MVP: создание заявки (status=new)
    // fullName можно не передавать (NULL), phone обязателен (11 цифр уже проверяется CHECK-ом в БД)

    CreateRequestResult createRequest(
        const std::string& phone,
        long serviceId,
        std::optional<long> doctorId,
        const std::string& visitAtIso,
        std::optional<std::string> fullName,
        std::optional<long> partnerId,
        std::optional<std::string> partnerCode,
        std::optional<std::string> kevId
    );

    Json::Value list(const RequestFilter& filter);


    // MVP: отметить заявку как оплачено и начислить reward (если partner_id != NULL)
    // priceOverride может быть NULL -> берём services.base_price
    OperationResult markPaid(long requestId, std::optional<double> priceOverride);

    // отклонить оплату (перевести из new -> cancelled)
    OperationResult markCancelled(long requestId);

private:
    drogon::orm::DbClientPtr db_;
};
