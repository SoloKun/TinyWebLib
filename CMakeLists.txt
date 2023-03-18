cmake_minimum_required(VERSION 3.0)

project(tiny_network C CXX)


include_directories(
    ${PROJECT_SOURCE_DIR}/src/base
    ${PROJECT_SOURCE_DIR}/src/net
    ${PROJECT_SOURCE_DIR}/src/net/poller
    ${PROJECT_SOURCE_DIR}/src/time
    ${PROJECT_SOURCE_DIR}/src/log
    ${PROJECT_SOURCE_DIR}/src/mysql
    ${PROJECT_SOURCE_DIR}/src/http
)

#aux_source_directory是一个宏，用于将指定目录下的所有源文件名存入指定变量
aux_source_directory(${PROJECT_SOURCE_DIR}/src/base SRC_BASE)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/net SRC_NET)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/net/poller SRC_POLLER)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/time SRC_TIME)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/log SRC_LOG)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/mysql SRC_MYSQL)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/http SRC_HTTP)

set(CXX_FLAGS
    -g
    -Wall
    -std=c++11
)

#add_library是一个宏，用于将指定的源文件生成链接文件，第一个参数是链接文件的名字，第二个参数是链接文件的类型，第三个参数是源文件列表
add_library(tiny_network SHARED
    ${SRC_BASE}
    ${SRC_NET}
    ${SRC_POLLER}
    ${SRC_TIME}
    ${SRC_LOG}
    ${SRC_MYSQL}
    ${SRC_HTTP}
)

# target_link_libraries是一个宏，用于将指定的链接库链接到可执行文件上
target_link_libraries(tiny_network
    pthread
    mysqlclient
)

#LINK_DIRECTORIES是一个宏，用于指定链接库的搜索路径
#CMAKE_INSTALL_PREFIX是一个变量，用于指定安装路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})

install(DIRECTORY ${PROJECT_SOURCE_DIR}/src/ 
        DESTINATION include/tiny_network FILES_MATCHING PATTERN "*.h")

