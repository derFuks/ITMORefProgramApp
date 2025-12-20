#include "RequestService.h"

RequestService::RequestService(drogon::orm::DbClientPtr db)
    : repo_(std::move(db)) {}

CreateRequestResult RequestService::createRequest(
    const std::string& phone,
    long serviceId,
    std::optional<long> doctorId,
    const std::string& visitAtIso,
    std::optional<std::string> fullName,
    std::optional<long> partnerId,
    std::optional<std::string> partnerCode
) {
    // В MVP пока пусть будет банальная валидация до БД (длина 11), но и CHECK в БД добавил.
    return repo_.createRequest(phone, serviceId, doctorId, visitAtIso, fullName, partnerId, partnerCode);
}

OperationResult RequestService::markPaid(long requestId, std::optional<double> priceOverride) {
    return repo_.markPaid(requestId, priceOverride);
}

OperationResult RequestService::markCancelled(long requestId) {
    return repo_.markCancelled(requestId);
}

Json::Value RequestService::list(const RequestFilter& filter) {
    return repo_.list(filter);
}
