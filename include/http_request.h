#ifndef _PORGI_HTTP_REQUEST_H_
#define _PORGI_HTTP_REQUEST_H_

#include "byte_buffer.h"

#include <cstdint>
#include <map>

enum class HttpMethod {
    UNKNOWN = 0,
    GET = 1,
};

static inline char const* http_method_name(HttpMethod met)
{
    switch (met) {
    case HttpMethod::UNKNOWN:
        return "UNKNOWN";
    case HttpMethod::GET:
        return "GET";
    }
}

using HeaderMap = std::map<ByteBuffer, ByteBuffer>;

struct HttpRequest {
    HttpMethod method;
    ByteBuffer uri;

    uint16_t http_major;
    uint16_t http_minor;

    HeaderMap headers;
};

struct HttpResponse {
    int status_code;
    HeaderMap headers;
    ByteBuffer body;
};

namespace std {

template <>
struct hash<HttpMethod>
{
    std::size_t operator()(HttpMethod met) const
    {
        return (size_t) met;
    }
};
}

#endif
