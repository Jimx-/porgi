#include "http_server.h"
#include "python_script_interface.h"

#include "cxxopts/include/cxxopts.hpp"
#include "easylogging++.h"
INITIALIZE_EASYLOGGINGPP

uint16_t port;
std::string script_path;
int ncpus;

static void print_help(const char* program)
{
    std::cerr << "Usage: " << program << " [option...] <script>" << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "\t-p,--port <port>    The port that Porgi listens on. Default is 8080" << std::endl;
    std::cerr << "\t-n,--ncpus <ncpus>  Number of worker threads. Default is 1" << std::endl;
    std::cerr << "\t-h,--help           Print this help information" << std::endl;

    exit(1);
}

static void parse_arg(int argc, char* argv[])
{
    cxxopts::Options options(argv[0], " - Porgi server");

    options.add_options()
        ("p,port", "", cxxopts::value<uint16_t>(port)->default_value("8080"), "PORT")
        ("n,ncpus", "", cxxopts::value<int>(ncpus)->default_value("1"), "NCPUS")
        ("script", "", cxxopts::value<std::string>(script_path), "SCRIPT");

    options.parse_positional({"script"});
    auto result = options.parse(argc, argv);

    if (result.count("script") != 1) {
        print_help(argv[0]);
    }
}

inline bool ends_with(const std::string& value, const std::string& ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

ScriptInterface* get_script_interface(const std::string& script_path)
{
    if (ends_with(script_path, ".py")) {
        return new PythonScriptInterface(script_path);
    }
}

int main(int argc, char** argv)
{
    parse_arg(argc, argv);

    ScriptInterface* script_interface = get_script_interface(script_path);

    HttpServer server("127.0.0.1", port, script_interface, ncpus);
    server.start_main_loop();

    return 0;
}

