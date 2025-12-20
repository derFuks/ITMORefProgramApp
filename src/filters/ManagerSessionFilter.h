#pragma once
#include <drogon/HttpFilter.h>

// Фильтр для SSR менеджера. Достает cookie сессию и прокидывает user_id/role в req->attributes()
class ManagerSessionFilter : public drogon::HttpFilter<ManagerSessionFilter> {
public:
    void doFilter(const drogon::HttpRequestPtr& req,
                  drogon::FilterCallback&& fcb,
                  drogon::FilterChainCallback&& fccb) override;
};