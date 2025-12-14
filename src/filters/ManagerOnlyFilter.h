#pragma once
#include <drogon/HttpFilter.h>


class ManagerOnlyFilter : public drogon::HttpFilter<ManagerOnlyFilter> {
public:
    void doFilter(const drogon::HttpRequestPtr& req,
                  drogon::FilterCallback&& fcb,
                  drogon::FilterChainCallback&& fccb) override;
};