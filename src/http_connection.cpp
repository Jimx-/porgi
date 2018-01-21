#include "http_connection.h"
#include "http_parser.h"
#include "easylogging++.h"

#include <errno.h>
#include <unistd.h>
#include <stdexcept>

HttpConnection::HttpConnection(HttpServer* server, int epfd, int fd) : server(server), epfd(epfd), fd(fd)
{
}

void HttpConnection::handle_read_event()
{
    static const ByteBuffer connection_header("Connection", 10);
    char buffer[CHUNK_SIZE];
    req_buffer.clear();

    while (true) {
        ssize_t nread = read(fd, buffer, CHUNK_SIZE);

        if (nread == 0) break;

        if (nread < 0) {
            if (errno == EINTR) {
                continue;
            }

            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                throw std::runtime_error("io read error(" + std::to_string(nread) + ")");
            }
            break;
        }
        req_buffer.append(buffer, (size_t) nread);
    }

    try {
        http_parser.parse_http(req_buffer, request);
    } catch(...) {
        handle_bad_request();
    }

    keep_alive = false;
    auto it = request.headers.find(connection_header);
    if (it != request.headers.end()) {
        auto connection_str = it->second.to_string();
        if (connection_str == "keep-alive" || connection_str == "Keep-Alive") {
            keep_alive = true;
        }
    }

    try {
        server->dispatch_request(*this, request, [this](const HttpResponse& response) {
            this->handle_response(response);
        });
    } catch (UrlMap::UnmatchedUrl) {
        handle_unmatched_url();
    }
}

void HttpConnection::handle_write_event()
{
    if (response.headers.empty() && response.body.size() == 0) return;

    ssize_t nwritten;
    /* write head */
    while (true) {
        nwritten = write(fd, &resp_head[resp_head_offset], resp_head_rem);

        if (nwritten < 0) {
            if (errno == EINTR) {
                continue;
            } if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return;
            } else {
                /* TODO: handle error */
            }
        }

        if (nwritten == 0) break;

        resp_head_offset += nwritten;
        resp_head_rem -= nwritten;
    }

    /* write body */
    while (true) {
        nwritten = write(fd, &response.body[resp_body_offset], resp_body_rem);

        if (nwritten < 0) {
            if (errno == EINTR) {
                continue;
            } if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return;
            } else {
                /* TODO: handle error */
            }
        }

        if (nwritten == 0) break;

        resp_body_offset += nwritten;
        resp_body_rem -= nwritten;
    }

    if (keep_alive) {
        reset();
    } else if (resp_body_rem == 0) {
        close();
    }
}

void HttpConnection::handle_response(const HttpResponse& response)
{
    LOG(INFO) << '"' << http_method_name(request.method) << " " << request.uri
              << " HTTP/" << request.http_major << '.' << request.http_minor << "\" " << response.status_code;

    this->response = response;
    build_resp_head(response, resp_head);
    resp_head_offset = 0;
    resp_head_rem = resp_head.size();
    resp_body_offset = 0;
    resp_body_rem = response.body.size();

    handle_write_event();
}

void HttpConnection::build_resp_head(const HttpResponse& response, ByteBuffer& buf)
{
    buf.append("HTTP/1.1 ", 9);
    buf.append(ByteBuffer(std::to_string(response.status_code)));
    buf.append(" \r\nServer: Porgi\r\nConnection: ", 30);
    if (keep_alive) {
        buf.append("keep-alive", 10);
    } else {
        buf.append("close", 5);
    }
    buf.append("\r\nContent-length: ", 18);
    buf.append(ByteBuffer(std::to_string(response.body.size())));
    buf.append("\r\n", 2);

    for (auto& it : response.headers) {
        buf.append(it.first);
        buf.append(": ", 2);
        buf.append(it.second);
        buf.append("\r\n", 2);
    }
    buf.append("\r\n", 2);
}

void HttpConnection::close()
{
    ::close(fd);
}

void HttpConnection::reset()
{
    resp_head.clear();
    resp_head_offset = 0;
    resp_body_offset = 0;
    resp_head_rem = 0;
    resp_body_rem = 0;
}

void HttpConnection::handle_bad_request()
{
    HttpResponse response;
    response.status_code = 400;
    response.headers[ByteBuffer("Content-Type")] = ByteBuffer("text/html");
    response.body = ByteBuffer(
#include "templates/400.inc"
    );
    keep_alive = false;
    handle_response(response);
}

void HttpConnection::handle_unmatched_url()
{
    HttpResponse response;
    response.status_code = 404;
    response.headers[ByteBuffer("Content-Type")] = ByteBuffer("text/html");
    response.body = ByteBuffer(
#include "templates/404.inc"
    );
    keep_alive = false;
    handle_response(response);
}

void HttpConnection::handle_internal_error()
{
    HttpResponse response;
    response.status_code = 500;
    response.headers[ByteBuffer("Content-Type")] = ByteBuffer("text/html");
    response.body = ByteBuffer(
#include "templates/500.inc"
    );
    keep_alive = false;
    handle_response(response);
}
