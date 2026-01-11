#include "RequestService.h"

RequestService::RequestService(drogon::orm::DbClientPtr db)
    : repo_(db), partnerRepo_(std::move(db)) {}

CreateRequestResult RequestService::createRequest(
    const std::string& phone,
    long serviceId,
    std::optional<long> doctorId,
    const std::string& visitAtIso,
    std::optional<std::string> fullName,
    std::optional<std::string> kevId,
    std::optional<long> partnerIdLegacy,
    std::optional<std::string> partnerCodeLegacy
) {
    // В MVP пока пусть будет банальная валидация до БД (длина 11), но и CHECK в БД добавил.
    //
    // kev_id — основной способ привязки заявки к партнеру.
    // partnerIdLegacy/partnerCodeLegacy оставил на всякий (совместимость), но если kev_id пришел — он главнее.
    
    std::optional<long> partnerId = partnerIdLegacy;

    if (kevId.has_value() && !kevId->empty()) {
        auto pid = partnerRepo_.findPartnerIdByKevId(*kevId);
        if (!pid.has_value()) {
            return CreateRequestResult::Fail("Invalid kev_id");
        }
        partnerId = static_cast<long>(*pid);
    }

    // partner_code больше не используем (legacy). kev_id вынесен в отдельное поле patient_requests.kev_id.
    (void)partnerCodeLegacy; // чтобы компилятор не ругался, пока поле еще живет.
    return repo_.createRequest(phone, serviceId, doctorId, visitAtIso, fullName, partnerId, std::nullopt, kevId);
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
