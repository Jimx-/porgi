#include "http_server.h"
#include "http_connection.h"
#include "easylogging++.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <cstring>
#include <stdexcept>

HttpServer::HttpServer(const std::string& host, Port port, ScriptInterface* script_interface, int ncpus, int backlog)
    : host(host), port(port), script_interface(script_interface), backlog(backlog), thread_pool(ncpus)
{
    script_interface->load_script(this);
}

void HttpServer::start_main_loop()
{
    int listen_fd = open_listenfd();
    if (listen_fd == -1) {
        throw std::runtime_error("cannot open listen socket");
    }

    if (make_socket_non_blocking(listen_fd) == -1) {
        throw std::runtime_error("failed to make socket non-blocking");
    }

    int epfd;
    epfd = epoll_create1(EPOLL_FLAGS);
    if (epfd == -1) {
        throw std::runtime_error("failed to create epoll");
    }

    auto http_conn = new HttpConnection(this, epfd, listen_fd);

    struct epoll_event ep_event;
    ep_event.events = EPOLLIN | EPOLLET;
    ep_event.data.ptr = reinterpret_cast<void*>(http_conn);
    epoll_add(epfd, listen_fd, &ep_event);

    LOG(INFO) << "Running on http://" << host << ":" << port << "/";

    auto events = std::make_unique<struct epoll_event[]>(MAX_EVENTS);

    while(true) {
        int nready = epoll_wait(epfd, events.get(), MAX_EVENTS, -1);

        if (nready < 0) {
            if (errno == EINTR) {
                continue;
            } else {
                throw std::runtime_error("epoll_wait failed");
            }
        }

        for (int i = 0; i < nready; ++i) {
            auto conn = reinterpret_cast<HttpConnection*>(events[i].data.ptr);
            int fd = conn->get_fd();

            if (listen_fd == fd) {
                /* handle incoming connections */
                socklen_t clientlen;
                struct sockaddr_in clientaddr;

                clientlen = sizeof(clientaddr);
                int conn_fd = accept(listen_fd, reinterpret_cast<struct sockaddr*>(&clientaddr), &clientlen);
                if (conn_fd < 0) {
                    throw std::runtime_error("accept error");
                }

                LOG(DEBUG) << "Accepting new connection, fd = " << conn_fd;
                if (make_socket_non_blocking(conn_fd) == -1) {
                    throw std::runtime_error("failed to make socket non-blocking");
                }

                auto new_conn = new HttpConnection(this, epfd, conn_fd);
                struct epoll_event new_event;
                new_event.events = EPOLLIN | EPOLLOUT | EPOLLET;
                new_event.data.ptr = reinterpret_cast<void*>(new_conn);
                epoll_add(epfd, conn_fd, &new_event);
            } else {
                if (events[i].events & EPOLLIN) {
                    conn->handle_read_event();
                } else {
                    conn->handle_write_event();
                }
            }
        }
    }
}

int HttpServer::open_listenfd()
{
    int sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sfd == -1) {
        return -1;
    }

    int opt = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = inet_addr(host.c_str());
    if (bind(sfd, (struct sockaddr*)&sa, sizeof(struct sockaddr)) == -1) {
        return -1;
    }

    if (listen(sfd, backlog) == -1) {
        close(sfd);
        return -1;
    }

    return sfd;
}

int HttpServer::make_socket_non_blocking(int sfd)
{
    int flags = fcntl (sfd, F_GETFL, 0);
    if (flags == -1) {
        return -1;
    }
    if(fcntl (sfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        return -1;
    }
    return 0;
}

int HttpServer::epoll_add(int epfd, int fd, struct epoll_event* event)
{
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, event) == -1) {
        throw std::runtime_error("cannot add epoll event");
    }
}

void
HttpServer::register_url_rule(const ByteBuffer& rule, UrlMap::RequestHandler&& handler, const std::vector<HttpMethod>& methods)
{
    url_map.register_rule(rule, std::move(handler), methods);
}

void HttpServer::dispatch_request(HttpConnection& conn, HttpRequest request, RequestCallback&& callback)
{
    UrlMap::UrlPatternMap pattern_map;
    auto handler = url_map.match_url(request.uri, request.method, pattern_map);

    thread_pool.push([handler, &conn, request = std::move(request), callback = std::move(callback), pattern_map = std::move(pattern_map)](int){
        try {
            auto resp = handler(request, pattern_map);
            callback(resp);
        } catch (...) {
            conn.handle_internal_error();
        }
    });
}
