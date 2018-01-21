#include "python_script_interface.h"

#include <boost/python/suite/indexing/map_indexing_suite.hpp>

using namespace boost::python;

struct HttpResponseProxy {
    HttpResponse resp;

    HttpResponseProxy(int status, const dict& hdrs, const ByteBuffer& payload)
    {
        write_head(status, hdrs);
        write(payload);
    }

    void write_head(int status, const dict& hdrs)
    {
        resp.status_code = status;

        auto items = hdrs.items();
        for (int i = 0; i < len(items); ++i) {
            auto tup = items[0];
            auto key_buf = extract<char const*>(tup[0]);
            auto value_buf = extract<char const*>(tup[1]);
            ByteBuffer key(key_buf, (size_t) len(tup[0])), value(value_buf, (size_t) len(tup[1]));
            resp.headers[key] = value;
        }
    }

    void write(const ByteBuffer& payload)
    {
        resp.body.append(payload);
    }
};

namespace detail_converter {

struct http_method_to_python_str {
    static PyObject* convert(HttpMethod met)
    {
        std::string method_name(http_method_name(met));

        return boost::python::incref(boost::python::object(method_name).ptr());
    }
};

struct byte_buffer_from_python_str {

    byte_buffer_from_python_str()
    {
        boost::python::converter::registry::push_back(
            &convertible,
            &construct,
            boost::python::type_id<ByteBuffer>());
    }

    static void* convertible(PyObject* obj_ptr)
    {
        if (!PyUnicode_Check(obj_ptr)) return 0;
        return obj_ptr;
    }

    static void construct(
        PyObject* obj_ptr,
        boost::python::converter::rvalue_from_python_stage1_data* data)
    {
        const char* value = PyUnicode_AsUTF8(obj_ptr);
        if (value == 0) boost::python::throw_error_already_set();
        void* storage = (
            (boost::python::converter::rvalue_from_python_storage<ByteBuffer>*)
                data)->storage.bytes;
        new(storage) ByteBuffer(value);
        data->convertible = storage;
    }
};

void init_converters()
{
    boost::python::to_python_converter<
        HttpMethod,
        http_method_to_python_str>();

    byte_buffer_from_python_str();
}
}

PythonScriptInterface::PythonScriptInterface(const std::string& path) : ScriptInterface(path)
{
    Py_Initialize();

    detail_converter::init_converters();
}

void PythonScriptInterface::load_script(HttpServer* server)
{
    this->server = server;

    std::ifstream ifs(script_path);
    if (!ifs) {
        throw FileIOError("cannot open script file");
    }
    std::string source((std::istreambuf_iterator<char>(ifs)),
                 std::istreambuf_iterator<char>());

    try {
        object main_module((handle<>(borrowed(PyImport_AddModule("__main__")))));

        globals = main_module.attr("__dict__");
        inject_namespace(globals);

        exec(source.c_str(), globals, globals);
    } catch (error_already_set) {
        PyErr_Print();
        throw ScriptExecutionError(get_error_string());
    }
}

void PythonScriptInterface::inject_namespace(boost::python::object& _namespace)
{
    static const char* prelude =
#include "prelude.inc"
    ;

    _namespace["Porgi"] = class_<PythonScriptInterface>("Porgi", init<std::string>())
        .def("register_route", &PythonScriptInterface::_py_register_route);
    _namespace["porgi"] = ptr(this);

    _namespace["ByteBuffer"] = class_<ByteBuffer>("ByteBuffer")
        .def("__len__", &ByteBuffer::size)
        .def("__str__", &ByteBuffer::to_string);

    _namespace["HeaderMap"] = class_<HeaderMap>("HeaderMap")
        .def(map_indexing_suite<HeaderMap>());

    _namespace["HttpRequest"] = class_<HttpRequest>("HttpRequest")
        .add_property("uri", &HttpRequest::uri)
        .add_property("method", &HttpRequest::method)
        .add_property("http_major", &HttpRequest::http_major)
        .add_property("http_minor", &HttpRequest::http_minor)
        .add_property("headers", &HttpRequest::headers);

    _namespace["_HttpResponse"] = class_<HttpResponseProxy>("HttpResponse", init<int, dict, ByteBuffer>())
        .def("write_head", &HttpResponseProxy::write_head)
        .def("write", &HttpResponseProxy::write);

    exec(prelude, _namespace, _namespace);
}

void PythonScriptInterface::_py_register_route(const ByteBuffer& rule, const object& f, const list& methods)
{
    std::vector<HttpMethod> methods_v;
    for (int i = 0; i < len(methods); ++i) {
        char const* method = extract<char const*>(methods[i]);

        if (std::strcmp(method, "GET") == 0) {
            methods_v.push_back(HttpMethod::GET);
        } else {
            PyErr_SetString(PyExc_ValueError, "unknown HTTP method");
            return;
        }
    }

    server->register_url_rule(
        rule,
        handler_wrapper(f),
        methods_v
    );
}

UrlMap::RequestHandler PythonScriptInterface::handler_wrapper(const boost::python::object& f)
{
    return UrlMap::RequestHandler([this, f](const HttpRequest& request, const UrlMap::UrlPatternMap& pattern_map) {
        try {
            object obj;

            if (pattern_map.empty()) {
                obj = call<object>(f.ptr(), boost::ref(request));
            } else {
                obj = call<object>(f.ptr(), boost::ref(request), boost::ref(pattern_map));
            }

            /* returned str value */
            extract<char const*> ext(obj);
            if (ext.check()) {
                HttpResponse resp;

                resp.status_code = 200;
                resp.headers[ByteBuffer("Content-Type")] = ByteBuffer("text/plain");
                char const* body = ext;
                resp.body.append(body, std::strlen(body));

                return resp;
            }

            HttpResponseProxy proxy = extract<HttpResponseProxy&>(obj);
            return proxy.resp;
        } catch(error_already_set) {
            PyErr_Print();
            throw ScriptExecutionError(get_error_string());
        }
    });
}

std::string PythonScriptInterface::get_error_string() const
{
    PyObject *ptype, *pvalue, *ptraceback;
    PyErr_Fetch(&ptype, &pvalue, &ptraceback);

    if (pvalue) {
        return extract<std::string>(pvalue);
    }

    return "";
}

