#ifndef _PORGI_HTTP_SERVER_H_
#define _PORGI_HTTP_SERVER_H_

#include "route.h"
#include "script_interface.h"

#include "ctpl/ctpl.h"

#include <string>

class HttpConnection;

class HttpServer {
public:
    typedef uint16_t Port;

    static const size_t MAX_CONNECTIONS = 1024;
    static const size_t MAX_EVENTS = 1024;

    HttpServer(const std::string& host, Port port, ScriptInterface* script_interface, int ncpus, int backlog = 1024);

    void start_main_loop();

    using RequestCallback = std::function<void(const HttpResponse&)>;
    void dispatch_request(HttpConnection& conn, HttpRequest request, RequestCallback&& callback);

    void register_url_rule(const ByteBuffer& rule, UrlMap::RequestHandler&& handler, const std::vector<HttpMethod>& methods);

private:
    std::string host;
    Port port;
    int backlog;
    ScriptInterface* script_interface;
    ctpl::thread_pool thread_pool;

    UrlMap url_map;

    static const int EPOLL_FLAGS = 0;

    int open_listenfd();
    int make_socket_non_blocking(int sfd);
    int epoll_add(int epfd, int fd, struct epoll_event* event);
};

#endif
