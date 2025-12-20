#pragma once

#include <drogon/HttpFilter.h>

// Фильтр на роль partner.

class PartnerOnlyFilter : public drogon::HttpFilter<PartnerOnlyFilter> {
public:
    void doFilter(const drogon::HttpRequestPtr& req,
                  drogon::FilterCallback&& fcb,
                  drogon::FilterChainCallback&& fccb) override;
};
