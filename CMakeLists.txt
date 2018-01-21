cmake_minimum_required(VERSION 3.5)
project(orgi)

set(CMAKE_CXX_STANDARD 14)

# find boost.python
FIND_PACKAGE(Boost)
FIND_PACKAGE(PythonLibs 3.5)
IF(Boost_FOUND)
    INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR} ${PYTHON_INCLUDE_DIR})
    SET(Boost_USE_STATIC_LIBS OFF)
    SET(Boost_USE_MULTITHREADED ON)
    SET(Boost_USE_STATIC_RUNTIME OFF)
    FIND_PACKAGE(Boost COMPONENTS python-py35)

ELSEIF(NOT Boost_FOUND)
    MESSAGE(FATAL_ERROR "Unable to find correct Boost version. Did you set BOOST_ROOT?")
ENDIF()

include_directories(3rdparty)
include_directories(3rdparty/easyloggingpp/src/)
include_directories(include)
set(LIBRARIES ${PYTHON_LIBRARY} ${Boost_LIBRARIES} pthread)

set(SOURCE_FILES src/main.cpp src/byte_buffer.cpp src/http_server.cpp src/http_connection.cpp
        src/http_parser.cpp src/route.cpp src/python_script_interface.cpp)
set(HEADER_FILES include/byte_buffer.h include/http_server.h include/http_connection.h include/http_parser.h
        include/route.h include/script_interface.h include/python_script_interface.h include/exceptions.h)
set(EXT_SOURCE_FILES 3rdparty/easyloggingpp/src/easylogging++.cc)
add_executable(porgi ${SOURCE_FILES} ${HEADER_FILES} ${EXT_SOURCE_FILES})
target_link_libraries(porgi ${LIBRARIES})
