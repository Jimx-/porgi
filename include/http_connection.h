#ifndef _PORGI_HTTP_CONNECTION_H_
#define _PORGI_HTTP_CONNECTION_H_

#include "byte_buffer.h"
#include "http_request.h"
#include "http_parser.h"
#include "http_server.h"

#include <cstddef>

class HttpConnection {
public:
    HttpConnection(HttpServer* server, int epfd, int fd);

    int get_fd() const { return fd; }

    void handle_read_event();
    void handle_write_event();
    void handle_response(const HttpResponse& response);
    void handle_bad_request();
    void handle_internal_error();

    void close();

private:
    int epfd, fd;
    HttpServer* server;
    ByteBuffer req_buffer;
    HttpRequest request;
    HttpResponse response;
    HttpParser http_parser;
    bool keep_alive;

    ByteBuffer resp_head;
    size_t resp_body_offset, resp_body_rem;
    size_t resp_head_offset, resp_head_rem;

    static const size_t CHUNK_SIZE = 4096;

    void build_resp_head(const HttpResponse& response, ByteBuffer& buf);
    void reset();

    void handle_unmatched_url();
};

#endif
