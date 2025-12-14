#pragma once
#include <json/json.h>
#include <drogon/orm/DbClient.h>
#include "repo/PartnerRepository.h"

class PartnerService {
public:
    explicit PartnerService(drogon::orm::DbClientPtr db);

    Json::Value getMyRequests(long long userId, int limit, int offset);
    Json::Value getMyRewards(long long userId, int limit, int offset);

private:
    PartnerRepository repo_;
};