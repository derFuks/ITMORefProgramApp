#include "PatientRequestRepository.h"
#include <drogon/orm/Exception.h>
#include <json/json.h>

PatientRequestRepository::PatientRequestRepository(drogon::orm::DbClientPtr db)
    : db_(std::move(db)) {}

CreateRequestResult PatientRequestRepository::createRequest(
    const std::string& phone,
    long serviceId,
    std::optional<long> doctorId,
    const std::string& visitAtIso,
    std::optional<std::string> fullName,
    std::optional<long> partnerId,
    std::optional<std::string> partnerCode
) {
    try {
        auto r = db_->execSqlSync(
            R"SQL(
                INSERT INTO patient_requests
                    (phone, service_id, doctor_id, visit_at, full_name, partner_id, partner_code)
                VALUES
                    ($1,    $2,         $3,        $4::timestamptz, $5,       $6,        $7)
                RETURNING id
            )SQL",
            phone,
            serviceId,
            doctorId.has_value() ? std::optional<long>(*doctorId) : std::nullopt,
            visitAtIso,
            fullName.has_value() ? std::optional<std::string>(*fullName) : std::nullopt,
            partnerId.has_value() ? std::optional<long>(*partnerId) : std::nullopt,
            partnerCode.has_value() ? std::optional<std::string>(*partnerCode) : std::nullopt
        );

        if (r.size() != 1) {
            return CreateRequestResult::Fail("Unexpected DB response on INSERT");
        }

        const auto id = r[0]["id"].as<long long>();
        return CreateRequestResult::Success(id);

    } catch (const drogon::orm::DrogonDbException &e) {
        return CreateRequestResult::Fail(std::string("DB error: ") + e.base().what());
    } catch (const std::exception &e) {
        return CreateRequestResult::Fail(std::string("Error: ") + e.what());
    }
}


OperationResult PatientRequestRepository::markPaid(long requestId, std::optional<double> priceOverride) {
    try {
        auto trans = db_->newTransaction();

        // 1) Блокируем заявку FOR UPDATE, проверяем статус new
        // 2) Берём service base_price/reward_rate
        // 3) Обновляем заявку -> paid, paid_at, price
        // 4) Начисляем reward (если partner_id != NULL), защита от дубля по UNIQUE(patient_request_id)
        auto r = trans->execSqlSync(
            R"SQL(
                WITH req AS (
                    SELECT pr.id, pr.status, pr.partner_id, pr.service_id
                    FROM patient_requests pr
                    WHERE pr.id = $1 AND pr.is_deleted = FALSE
                    FOR UPDATE
                ),
                svc AS (
                    SELECT s.id, s.base_price, s.reward_rate
                    FROM services s
                    JOIN req ON req.service_id = s.id
                    WHERE s.is_deleted = FALSE AND s.is_active = TRUE
                ),
                upd AS (
                    UPDATE patient_requests pr
                    SET
                        status = 'paid',
                        paid_at = now(),
                        price = COALESCE($2, (SELECT base_price FROM svc))
                    WHERE pr.id = (SELECT id FROM req)
                      AND pr.status = 'new'
                    RETURNING pr.id, pr.partner_id, pr.service_id, pr.price
                ),
                ins AS (
                    INSERT INTO partner_rewards (partner_id, patient_request_id, amount, reward_rate)
                    SELECT
                        upd.partner_id,
                        upd.id,
                        ROUND(upd.price * svc.reward_rate, 2),
                        svc.reward_rate
                    FROM upd
                    JOIN svc ON svc.id = upd.service_id
                    WHERE upd.partner_id IS NOT NULL
                    ON CONFLICT (patient_request_id) DO NOTHING
                    RETURNING 1
                )
                SELECT
                    (SELECT COUNT(*) FROM upd) AS updated_requests,
                    (SELECT COUNT(*) FROM ins) AS inserted_rewards
            )SQL",
            requestId,
            priceOverride.has_value() ? std::optional<double>(*priceOverride) : std::nullopt
        );

        const auto updated = r[0]["updated_requests"].as<long long>();
        if (updated != 1) {
            return OperationResult::Fail(
                "Request not updated (not found, deleted, not in 'new' status, or service inactive)"
            );

        }
        // trans->commit();
        return OperationResult::Success("Request marked as paid");
    } catch (const drogon::orm::DrogonDbException &e) {
        return OperationResult::Fail(std::string("DB error: ") + e.base().what());
    } catch (const std::exception &e) {
        return OperationResult::Fail(std::string("Error: ") + e.what());
    }
}

OperationResult PatientRequestRepository::markCancelled(long requestId) {
    try {
        auto trans = db_->newTransaction();

        // Для простоты: лочу заявку FOR UPDATE и меняю статус из 'new' >> 'cancelled'
        auto r = trans->execSqlSync(
            R"SQL(
                WITH req AS (
                    SELECT id, status
                    FROM patient_requests
                    WHERE id = $1 AND is_deleted = FALSE
                    FOR UPDATE
                ),
                upd AS (
                    UPDATE patient_requests
                    SET status = 'cancelled'
                    WHERE id = (SELECT id FROM req)
                      AND status = 'new'
                    RETURNING 1
                )
                SELECT (SELECT COUNT(*) FROM upd) AS updated_requests
            )SQL",
            requestId
        );

        if (!r.empty()) {
            const auto updated = r[0]["updated_requests"].as<long long>();
            if (updated == 0) {
                return OperationResult::Fail("Апдей не прошел (вероятно запрос уже проходил)");
            }
        }

        return OperationResult::Success("Request cancelled");
    } catch (const drogon::orm::DrogonDbException &e) {
        return OperationResult::Fail(std::string("DB error: ") + e.base().what());
    } catch (const std::exception &e) {
        return OperationResult::Fail(std::string("Error: ") + e.what());
    }
}

Json::Value PatientRequestRepository::list(const RequestFilter& filter) {
    // Фиксированный запрос с опциональными фильтрами
    // если параметр NULL -> фильтр не применяется
    const std::string sql = R"SQL(
        SELECT
            pr.id,
            pr.phone,
            pr.status,
            pr.price,
            pr.created_at,
            pr.paid_at,
            s.name AS service_name
        FROM patient_requests pr
        JOIN services s ON s.id = pr.service_id
        WHERE pr.is_deleted = FALSE
          AND ($1::text IS NULL OR pr.status = $1)
          AND ($2::timestamptz IS NULL OR pr.created_at >= $2)
          AND ($3::timestamptz IS NULL OR pr.created_at <= $3)
        ORDER BY pr.created_at DESC
        LIMIT $4::int OFFSET $5::int
    )SQL";

    try {
        // Важно: здесь мы передаём фиксированный набор параметров.
        // std::optional<std::string> используется для NULL.
        auto r = db_->execSqlSync(
            sql,
            filter.status.has_value() ? std::optional<std::string>(*filter.status) : std::nullopt,
            filter.dateFrom.has_value() ? std::optional<std::string>(*filter.dateFrom) : std::nullopt,
            filter.dateTo.has_value() ? std::optional<std::string>(*filter.dateTo) : std::nullopt,
            filter.limit,
            filter.offset
        );

        Json::Value items(Json::arrayValue);

        for (const auto& row : r) {
            Json::Value item;
            item["id"] = Json::Int64(row["id"].as<long long>());
            item["phone"] = row["phone"].as<std::string>();
            item["status"] = row["status"].as<std::string>();
            item["service"] = row["service_name"].as<std::string>();

            // JsonCpp: null надо задавать как Json::Value(Json::nullValue)
            item["price"] = row["price"].isNull()
                ? Json::Value(Json::nullValue)
                : Json::Value(row["price"].as<double>());

            item["created_at"] = row["created_at"].as<std::string>();

            item["paid_at"] = row["paid_at"].isNull()
                ? Json::Value(Json::nullValue)
                : Json::Value(row["paid_at"].as<std::string>());

            items.append(item);
        }

        return items;
    } catch (const drogon::orm::DrogonDbException &e) {
        // Для MVP пока пустой список; позже надо прикрутить нормальную ошибку наверх
        return Json::Value(Json::arrayValue);
    }
}
