#pragma once
#include <optional>
#include <string>
#include <drogon/orm/DbClient.h>

#include "domain/CreateRequestResult.h"
#include "domain/OperationResult.h"
#include "repo/PatientRequestRepository.h"

#include "repo/PartnerRepository.h"


class RequestService {
public:
    explicit RequestService(drogon::orm::DbClientPtr db);

    CreateRequestResult createRequest(
        const std::string& phone,
        long serviceId,
        std::optional<long> doctorId,
        const std::string& visitAtIso,
        std::optional<std::string> fullName,
        std::optional<std::string> kevId,
        std::optional<long> partnerIdLegacy,
        std::optional<std::string> partnerCodeLegacy
    );

    OperationResult markPaid(long requestId, std::optional<double> priceOverride);

    OperationResult markCancelled(long requestId);

    Json::Value list(const RequestFilter& filter);

private:
    PatientRequestRepository repo_;
    PartnerRepository partnerRepo_;
};