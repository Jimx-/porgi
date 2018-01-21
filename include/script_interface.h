#ifndef _PORGI_SCRIPT_INTERFACE_H_
#define _PORGI_SCRIPT_INTERFACE_H_

#include "exceptions.h"

#include <string>

class HttpServer;

class ScriptInterface {
public:
    PORGI_DEF_ERROR(ScriptExecutionError);

    explicit ScriptInterface(const std::string& path) : script_path(path) { }

    virtual void load_script(HttpServer* server) = 0;

protected:
    std::string script_path;
};

#endif
