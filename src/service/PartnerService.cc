#include "PartnerService.h"
#include <stdexcept>

PartnerService::PartnerService(drogon::orm::DbClientPtr db)
    : repo_(std::move(db)) {}

Json::Value PartnerService::getMyProfile(long long userId) {
    return repo_.getPartnerProfile(userId);
}

Json::Value PartnerService::getMyRequests(long long userId, int limit, int offset) {
    auto pid = repo_.findPartnerIdByUserId(userId);
    if (!pid) throw std::runtime_error("Partner profile not found for user");
    return repo_.listPartnerRequests(*pid, limit, offset);
}
Json::Value PartnerService::getMyRewards(long long userId, int limit, int offset) {
    auto pid = repo_.findPartnerIdByUserId(userId);
    if (!pid) throw std::runtime_error("Partner profile not found for user");
    return repo_.listPartnerRewards(*pid, limit, offset);
}