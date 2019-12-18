cmake_minimum_required(VERSION 3.9)
include(cmake/hobot_util.cmake)
project(xbusiness)

# copy config to build file for ci
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/conf
            DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)
            file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/start.sh
                        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)

#for xsearch
set(CMAKE_BUILD_TYPE Release)

if (${PLATFORM_MAC})
    add_definitions(-DHR_POSIX)
    add_definitions(-DHR_MAC)
elseif (${PLATFORM_LINUX})
    add_definitions(-DHR_POSIX)
    add_definitions(-DHR_LINUX)
    if(NOT(${arch} STREQUAL "default") AND NOT(${arch} STREQUAL "x86"))
        if(${arch} STREQUAL "arm_hi3519")
            list(APPEND CMAKE_C_FLAGS " -mcpu=cortex-a17.cortex-a7 -mfloat-abi=softfp -mfpu=neon-vfpv4 ")
            add_definitions(-DHR_HI3519)
        elseif(${arch} STREQUAL "armhf")
            list(APPEND CMAKE_C_FLAGS " -marm -mfloat-abi=hard -mfpu=neon ")
        else()
            list(APPEND CMAKE_C_FLAGS " -marm -mfloat-abi=softfp -mfpu=neon ")
        endif()
    else()   # linux on x86?
        list(APPEND CMAKE_C_FLAGS " -msse4.2 ")
    endif()
elseif (${PLATFORM_ANDROID})
    set(ENABLE_NEON "true")
    add_definitions(-DENABLE_OMP)
    add_definitions(-DHR_POSIX)
    add_definitions(-DHR_ANDROID)
elseif (${PLATFORM_WIN})
    set(CMAKE_CXX_FLAGS_RELEASE "/MD")
    set(CMAKE_CXX_FLAGS_DEBUG "/MDd")
    add_definitions(-DHR_WIN)
    add_definitions(-D_MBCS)
    add_definitions(-DHOBOT_EXPORTS)
else ()
    message(FATAL_ERROR "invoke cmake with -DPLATFORM_MAC=TRUE|-DPLATFORM_LINUX=TRUE|-DPLATFORM_ANDROID=TRUE|-DPLATFORM_WIN=TRUE")
    return()
endif()
message("CMAKE_C_FLAGS: ${CMAKE_C_FLAGS}" )

option(USE_ENCRYPT "whether to use hobot aes encrypt" OFF)
if (NOT ${USE_ENCRYPT})
    add_definitions(-DNO_ENCRYPT)
    message("not use hobot aes encrypt")
else ()
    message("use hobot aes encrypt")
endif()

option(USE_OMP "whether to use omp" OFF)
if (NOT ${USE_OMP})
    add_definitions(-DNO_OMP)
    message("not use omp..")
else ()
    message("use omp..")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp" )
endif ()

option(IPC_VER "whether to use omp" OFF)
if (NOT ${IPC_VER})
    add_definitions(-DNOIPC_VER)
    message("not ipc ver..")
else ()
    message("ipc ver..")
    add_definitions(-DUSE_ZLOG)
    message("use zlog....")
    add_definitions(-DNO_OMP)
    message("not use omp....")
endif ()

list(APPEND CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -Wno-reorder -Wno-sign-compare")
string(REGEX REPLACE ";" " " CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
string(REGEX REPLACE ";" " " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})

if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC -DNOIPC_VER -Ofast -O3")
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -DEBUG -g -DNOIPC_VER -O0 -fPIC")
endif()
#end for xsearch


add_definitions(-DXPERF)
message("using perf debug")

include_directories(
        include
        include/common
        include/common/redis
        include/log
        include/package
        include/frame
        include/conf
        include/data
        include/message
        include/package/consumer
        include/package/transponder
        Depend/include
)

link_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/Depend/lib
)

include_directories(SYSTEM /usr/include/python2.7)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

set(CMAKE_CXX_STANDARD 11)

#set(Boost_ADDITIONAL_VERSIONS "1.66" "1.66.0")
#set(BOOST_ROOT "/home/yanxinwang/.gradle/caches/modules-2/files-2.1/third_party/boost/1.67.0_linux_centos_gcc-4.8.5/files/boost")
set(BOOST_COMPONENTS system filesystem thread)
# Late 2017 TODO: remove the following checks and always use std::regex
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
#        set(BOOST_COMPONENTS ${BOOST_COMPONENTS} regex)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_BOOST_REGEX")
    endif()
endif()

#find_package(Boost 1.67.0 COMPONENTS ${BOOST_COMPONENTS} REQUIRED)
#include_directories(SYSTEM ${Boost_INCLUDE_DIR})

message("add src files ...")
set(SOURCE_FILES
        src/log/log_module.cpp
        src/common/com_func.cpp
        src/common/kafka_consumer_imp.cpp
        src/common/kafka_consumer_imp_cb.cpp
	src/common/ThreadSAP.cpp
        src/common/redis/RedisClient.cpp
        src/common/redis/RedisSubscriber.cpp
        src/common/mongo/mongo_client_pool.cpp
        src/message/session.cpp
        src/package/conf/conf_manager.cpp
        src/package/transponder/transponder.cpp
        src/package/workflow/flow_manager.cpp
        src/package/workflow/http_module_wrapper.cpp
        src/package/workflow/module_factory.cpp
        src/package/workflow/module/module_download.cpp
        src/package/workflow/module/module_kafka_output.cpp
        src/package/workflow/module/module_picture_waiting.cpp
        src/package/workflow/module/module_business_output.cpp
        src/package/workflow/module/module_trace_space.cpp 
        src/package/workflow/module/module_get_plugin.cpp
        src/package/workflow/module/module_generate_url.cpp
	src/package/consumer/MsgConsumer.cpp
        src/package/consumer/ConsumeProcess.cpp
        src/package/consumer/PartitionManager.cpp 
        src/package/consumer/XBusinessHandler.cpp
        src/handler/pre_handler.cpp
        src/handler/pre_handler_http_request.cpp
        src/handler/pre_handler_http_response.cpp
        src/data/base_data.cpp
        src/data/business_data.cpp
        src/data/capture_data.cpp
        src/data/xiashun_business_data.cpp
        src/data/xiashun_capture_data.cpp
        src/data/screenshot_data.cpp
        )

set(SOURCE_FILES_SERVICE
        src/service/main.cpp
        ${SOURCE_FILES}
        )

add_executable(xbusiness  ${SOURCE_FILES_SERVICE})
target_link_libraries(xbusiness xflow hobotlog  passenger_flow boost_system boost_thread mongocxx bsoncxx mercuriusd logmodule zkclientcxx thrift thriftnb event rdkafka rdkafka++ aws-cpp-sdk-s3 aws-cpp-sdk-core jsoncpp configparser cpp_redis tacopie pthread dl )
target_link_libraries(xbusiness ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(xbusiness  xsearch protobuf z  zip tracktimer)


