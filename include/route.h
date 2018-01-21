#ifndef _PORGI_ROUTING_H_
#define _PORGI_ROUTING_H_

#include "http_request.h"
#include "exceptions.h"

#include <unordered_map>
#include <functional>
#include <vector>

class UrlMap {
public:
    PORGI_DEF_ERROR(InvalidUrlRule);
    PORGI_DEF_ERROR(UnmatchedUrl);

    using UrlPatternMap = std::map<ByteBuffer, ByteBuffer>;
    using RequestHandler = std::function<HttpResponse(const HttpRequest& request, const UrlPatternMap& pattern_map)>;

    void register_rule(const ByteBuffer& rule, RequestHandler&& handler, const std::vector<HttpMethod>& methods);
    RequestHandler match_url(const ByteBuffer& url, HttpMethod method, UrlPatternMap& pattern_map);

private:

    struct UrlEntry {
        std::unordered_map<HttpMethod, RequestHandler> handlers;

        ByteBuffer pattern_name;
        std::unique_ptr<UrlEntry> pattern_entry;

        std::unordered_map<ByteBuffer, std::unique_ptr<UrlEntry> > children;
    };

    UrlEntry root;
};

#endif
