#ifndef _PORGI_PYTHON_SCRIPT_INTERFACE_H_
#define _PORGI_PYTHON_SCRIPT_INTERFACE_H_

#include "script_interface.h"
#include "http_server.h"

#include <boost/python.hpp>

class PythonScriptInterface : public ScriptInterface {
public:
    explicit PythonScriptInterface(const std::string& path);

    void load_script(HttpServer* server) override;

private:
    HttpServer* server;
    boost::python::object globals;

    void inject_namespace(boost::python::object& _namespace);

    void _py_register_route(const ByteBuffer& rule, const boost::python::object& f, const boost::python::list& methods);
    UrlMap::RequestHandler handler_wrapper(const boost::python::object& f);

    std::string get_error_string() const;
};

#endif
