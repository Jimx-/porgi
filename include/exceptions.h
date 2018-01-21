#ifndef _PORGI_EXCEPTIONS_H_
#define _PORGI_EXCEPTIONS_H_

#include <stdexcept>

#define PORGI_DEF_ERROR(name) \
    class name : public std::runtime_error { \
    public:                                                             \
        explicit name (const std::string& what_arg) : std::runtime_error(what_arg) { } \
    }

PORGI_DEF_ERROR(FileIOError);

#endif
