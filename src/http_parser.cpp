#include "http_parser.h"

#include <unordered_map>
#include <string>
#include <cctype>

static std::unordered_map<HttpMethod, std::string> method_strings =
    {
#define DEF_METHOD_STRING(name) { HttpMethod::name, #name }
     DEF_METHOD_STRING(GET),
    };

static bool is_uri_char(char ch)
{
    return isalpha(ch) || isdigit(ch) || (ch == '_') || (ch == '/') || (ch == '.');
}

HttpParser::HttpParser()
{
    state = RequestParseState::START_REQ;
}

void HttpParser::parse_http(const ByteBuffer& req_buf, HttpRequest& request)
{
    for (auto p = std::begin(req_buf); p != std::end(req_buf); ++p) {
        auto ch = *p;

        switch (state) {
        case RequestParseState::START_REQ:
            if (ch == '\n' || ch == '\r') {
                break;
            }
            if (ch < 'A' || ch > 'Z') {
                throw HttpParser::InvalidMethod("invalid method");
            }

            method = HttpMethod::UNKNOWN;
            switch (ch) {
            case 'G':
                method = HttpMethod::GET;
                break;
            default:
                throw HttpParser::InvalidMethod("invalid method");
            }
            method_index = 1;
            state = RequestParseState::METHOD;
            break;

        case RequestParseState::METHOD:
            {
                auto& match = method_strings[method];

                if (ch == ' ' && method_index == match.length()) {
                    state = RequestParseState::SPACES_BEFORE_URI;
                    request.method = method;
                } else if (ch == match[method_index]) {

                } else if (ch < 'A' || ch > 'Z') {
                    throw HttpParser::InvalidMethod("invalid method");
                }

                method_index++;
                break;
            }

        case RequestParseState::SPACES_BEFORE_URI:
            if (ch == ' ') break;

            if (ch == '/') {
                uri_buffer.clear();
                uri_buffer.append(p, 1);
                state = RequestParseState::PATH;
            } else {
                throw HttpParser::InvalidURI("invalid URI");
            }
            break;

        case RequestParseState::PATH:
            switch (ch) {
            case ' ':
                state = RequestParseState::HTTP_START;
                request.uri = std::move(uri_buffer);
                break;
            default:
                state = parse_uri_char(ch);
                if (state == RequestParseState::FINISH) {
                    throw HttpParser::InvalidURI("invalid URI");
                }

                uri_buffer.append(p, 1);
                break;
            }
            break;

        case RequestParseState::HTTP_START:
            switch (ch) {
            case 'H':
                state = RequestParseState::HTTP_H;
                break;
            case ' ':
                break;
            default:
                throw HttpParser::InvalidConstant("invalid constant");
            }
            break;

        case RequestParseState::HTTP_H:
            state = RequestParseState::HTTP_HT;
            break;

        case RequestParseState::HTTP_HT:
            state = RequestParseState::HTTP_HTT;
            break;

        case RequestParseState::HTTP_HTT:
            state = RequestParseState::HTTP_HTTP;
            break;

        case RequestParseState::HTTP_HTTP:
            state = RequestParseState::HTTP_MAJOR;
            break;

        case RequestParseState::HTTP_MAJOR:
            if (!isdigit(ch)) {
                throw HttpParser::InvalidVersion("invalid version");
            }

            request.http_major = ch - '0';
            state = RequestParseState::HTTP_DOT;
            break;

        case RequestParseState::HTTP_DOT:
            if (ch != '.') {
                throw HttpParser::InvalidVersion("invalid version");
            }

            state = RequestParseState::HTTP_MINOR;
            break;

        case RequestParseState::HTTP_MINOR:
            if (!isdigit(ch)) {
                throw HttpParser::InvalidVersion("invalid version");
            }

            request.http_minor = ch - '0';
            state = RequestParseState::HTTP_END;
            break;

        case RequestParseState::HTTP_END:
            if (ch == '\r')
                state = RequestParseState::REQ_LINE_ALMOST_DONE;
            else if (ch == '\n')
                state = RequestParseState::HEADER_FIELD_START;
            else
                throw HttpParser::InvalidVersion("invalid version");
            break;

        case RequestParseState::REQ_LINE_ALMOST_DONE:
            if (ch == '\n')
                state = RequestParseState::HEADER_FIELD_START;
            else
                throw HttpParser::LFExpected("LF character expected");
            break;

        case RequestParseState::HEADER_FIELD_START:
            switch (ch) {
            case '\r':
                state = RequestParseState::HEADERS_ALMOST_DONE;
                break;
            case '\n':
                state = RequestParseState::HEADERS_ALMOST_DONE;
                break;
            default:
                state = RequestParseState::HEADER_FIELD;
                last_header_name.clear();
                last_header_name.append(p, 1);
                break;
            }
            break;

        case RequestParseState::HEADER_FIELD:
            switch (ch) {
            case ':':
                state = RequestParseState::HEADER_VALUE_START;
                break;
            case '\r':
                state = RequestParseState::HEADER_ALMOST_DONE;
                break;
            default:
                last_header_name.append(p, 1);
                break;
            }
            break;

        case RequestParseState::HEADER_VALUE_START:
            if (ch != ' ') {
                throw InvalidHeader("invalid header");
            }

            state = RequestParseState::HEADER_SPACE_BEFORE_VALUE;
            break;

        case RequestParseState::HEADER_SPACE_BEFORE_VALUE:
            switch (ch) {
            case ' ':
                break;
            case '\r':
                state = RequestParseState::HEADER_ALMOST_DONE;
                break;
            default:
                last_header_value.clear();
                last_header_value.append(p, 1);
                state = RequestParseState::HEADER_VALUE;
                break;
            }
            break;

        case RequestParseState::HEADER_VALUE:
            switch (ch) {
            case '\r':
                state = RequestParseState::HEADER_ALMOST_DONE;
                break;
            case ' ':
                state = RequestParseState::HEADER_SPACE_AFTER_VALUE;
                break;
            default:
                last_header_value.append(p, 1);
                break;
            }
            break;

        case RequestParseState::HEADER_SPACE_AFTER_VALUE:
            switch (ch) {
            case ' ':
                break;
            case '\r':
                state = RequestParseState::HEADER_ALMOST_DONE;
                break;
            default:
                state = RequestParseState::HEADER_VALUE;
                last_header_value.append(" ", 1);
                last_header_value.append(p, 1);
                break;
            }
            break;

        case RequestParseState::HEADER_ALMOST_DONE:
            request.headers[last_header_name] = last_header_value;
            state = RequestParseState::HEADER_FIELD_START;
            break;

        case RequestParseState::HEADERS_ALMOST_DONE:
            state = RequestParseState::FINISH;
            break;

        case RequestParseState::FINISH:
            state = RequestParseState::START_REQ;
            return;
        }
    }
}

HttpParser::RequestParseState HttpParser::parse_uri_char(char ch)
{
    switch (state) {
    case RequestParseState::SPACES_BEFORE_URI:
        if (ch == '/') return RequestParseState::PATH;

        break;
    case RequestParseState::PATH:
        if (is_uri_char(ch)) {
            return state;
        }

        if (ch == '?') {
            return RequestParseState::QUERY_STRING_START;
        }
        break;
    }

    return RequestParseState::FINISH;
}
