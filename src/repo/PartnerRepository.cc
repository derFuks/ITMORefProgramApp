#include "PartnerRepository.h"

PartnerRepository::PartnerRepository(drogon::orm::DbClientPtr db)
    : db_(std::move(db)) {}


std::optional<long long> PartnerRepository::findPartnerIdByUserId(long long userId) {
    auto r = db_->execSqlSync(
        R"SQL(
            SELECT id
            FROM partners
            WHERE user_id = $1 AND is_deleted = FALSE
            LIMIT 1
        )SQL",
        userId
    );
    if (r.empty()) return std::nullopt;
    return r[0]["id"].as<long long>();
}

std::optional<long long> PartnerRepository::findPartnerIdByKevId(const std::string &kevId) {
    if (kevId.empty()) return std::nullopt;

    auto r = db_->execSqlSync(
        R"SQL(
            SELECT id
            FROM partners
            WHERE kev_id = $1 AND is_deleted = FALSE
            LIMIT 1
        )SQL",
        kevId
    );

    if (r.empty()) return std::nullopt;
    return r[0]["id"].as<long long>();
}


Json::Value PartnerRepository::getPartnerProfile(long long userId) {
    // NOTE: тут специально всё «в лоб». Потом можно будет сделать нормальную модельку.
    auto r = db_->execSqlSync(
        R"SQL(
            SELECT
                p.id,
                p.name,
                p.email,
                p.kev_id,
                pn.phone AS phone
            FROM partners p
            LEFT JOIN partner_phone_numbers pn
                ON pn.partner_id = p.id
               AND pn.unassigned_at IS NULL
               AND pn.is_active = TRUE
            WHERE p.user_id = $1
              AND p.is_deleted = FALSE
            ORDER BY pn.assigned_at DESC NULLS LAST
            LIMIT 1
        )SQL",
        userId
    );

    Json::Value out;
    if (r.empty()) {
        out["ok"] = false;
        out["error"] = "Partner profile not found";
        return out;
    }

    out["ok"] = true;
    out["partner_id"] = Json::Int64(r[0]["id"].as<long long>());
    out["name"] = r[0]["name"].as<std::string>();
    out["email"] = r[0]["email"].as<std::string>();
    out["kev_id"] = r[0]["kev_id"].isNull() ? "" : r[0]["kev_id"].as<std::string>();
    out["phone"] = r[0]["phone"].isNull() ? "" : r[0]["phone"].as<std::string>();
    return out;
}

Json::Value PartnerRepository::listPartnerRequests(long long partnerId, int limit, int offset) {
    auto r = db_->execSqlSync(
        R"SQL(
            SELECT pr.id, pr.phone, pr.status, pr.price, pr.created_at, pr.paid_at,
                   s.name AS service_name
            FROM patient_requests pr
            JOIN services s ON s.id = pr.service_id
            WHERE pr.partner_id = $1 AND pr.is_deleted = FALSE
            ORDER BY pr.created_at DESC
            LIMIT $2::int OFFSET $3::int
        )SQL",
        partnerId, limit, offset
    );


    Json::Value items(Json::arrayValue);
    for (const auto& row : r) {
        Json::Value it;
        it["id"] = Json::Int64(row["id"].as<long long>());
     it["phone"] = row["phone"].as<std::string>();
        it["status"] = row["status"].as<std::string>();
        it["service"] = row["service_name"].as<std::string>();
        it["price"] = row["price"].isNull() ? Json::Value(Json::nullValue) : Json::Value(row["price"].as<double>());
        it["created_at"] = row["created_at"].as<std::string>();
        it["paid_at"] = row["paid_at"].isNull() ? Json::Value(Json::nullValue) : Json::Value(row["paid_at"].as<std::string>());
        items.append(it);
    }
    return items;
}


Json::Value PartnerRepository::listPartnerRewards(long long partnerId, int limit, int offset) {
    auto r = db_->execSqlSync(
        R"SQL(
            SELECT id, patient_request_id, amount, reward_rate, created_at
            FROM partner_rewards
            WHERE partner_id = $1 AND is_deleted = FALSE
            ORDER BY created_at DESC
            LIMIT $2::int OFFSET $3::int
        )SQL",
        partnerId, limit, offset
    );

    Json::Value items(Json::arrayValue);
    for (const auto& row : r) {
        Json::Value it;
        it["id"] = Json::Int64(row["id"].as<long long>());
        it["patient_request_id"] = Json::Int64(row["patient_request_id"].as<long long>());
        it["amount"] = row["amount"].as<double>();
        it["reward_rate"] = row["reward_rate"].as<double>();
        it["created_at"] = row["created_at"].as<std::string>();
        items.append(it);
    }
    return items;
}