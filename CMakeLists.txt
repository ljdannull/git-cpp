cmake_minimum_required(VERSION 3.13)
project(git-starter-cpp)
set(CMAKE_CXX_STANDARD 20) # Enable the C++20 standard

find_package(ZLIB)
find_package(OpenSSL)

set(SOURCE_FILES src/Server.cpp)

add_executable(server ${SOURCE_FILES})

target_link_libraries(server ZLIB::ZLIB)
target_link_libraries(server ${OPENSSL_LIBRARIES})