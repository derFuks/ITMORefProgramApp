#pragma once
#include <optional>
#include <json/json.h>
#include <drogon/orm/DbClient.h>


class PartnerRepository {
public:
    explicit PartnerRepository(drogon::orm::DbClientPtr db);

    std::optional<long long> findPartnerIdByUserId(long long userId);
    std::optional<long long> findPartnerIdByKevId(const std::string &kevId);
    
    Json::Value getPartnerProfile(long long userId);
    Json::Value listPartnerRequests(long long partnerId, int limit, int offset);
    Json::Value listPartnerRewards(long long partnerId, int limit, int offset);

private:
    drogon::orm::DbClientPtr db_;
};

