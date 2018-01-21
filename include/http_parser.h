#ifndef _PORGI_HTTP_PARSER_H_
#define _PORGI_HTTP_PARSER_H_

#include "http_request.h"
#include "exceptions.h"

#include <string>

class HttpParser {
public:
    PORGI_DEF_ERROR(InvalidMethod);
    PORGI_DEF_ERROR(InvalidURI);
    PORGI_DEF_ERROR(InvalidConstant);
    PORGI_DEF_ERROR(InvalidVersion);
    PORGI_DEF_ERROR(LFExpected);
    PORGI_DEF_ERROR(InvalidHeader);

    HttpParser();
    void parse_http(const ByteBuffer& req_buf, HttpRequest& request);

private:
    enum class RequestParseState {
        START_REQ,
        METHOD,
        SPACES_BEFORE_URI,
        PATH,
        QUERY_STRING_START,
        HTTP_START,
        HTTP_H,
        HTTP_HT,
        HTTP_HTT,
        HTTP_HTTP,
        HTTP_MAJOR,
        HTTP_DOT,
        HTTP_MINOR,
        HTTP_END,
        REQ_LINE_ALMOST_DONE,
        HEADER_FIELD_START,
        HEADER_FIELD,
        HEADER_VALUE_START,
        HEADER_SPACE_BEFORE_VALUE,
        HEADER_VALUE,
        HEADER_SPACE_AFTER_VALUE,
        HEADER_ALMOST_DONE,
        HEADERS_ALMOST_DONE,
        FINISH,
    };

    RequestParseState state;
    HttpMethod method;
    size_t method_index;
    ByteBuffer uri_buffer;
    ByteBuffer last_header_name;
    ByteBuffer last_header_value;

    RequestParseState parse_uri_char(char ch);
};

#endif
