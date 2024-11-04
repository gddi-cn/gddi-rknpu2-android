include_directories(${FFMPEG_INCLUDE_DIRS})

link_directories(${CMAKE_BINARY_DIR}/src/api)
link_directories(${CMAKE_BINARY_DIR}/src/core)
link_directories(${CMAKE_BINARY_DIR}/src/common)
link_directories(${CMAKE_BINARY_DIR}/src/register)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../common)

if(BUILD_TARGET_CHIP STREQUAL "ts")
link_directories("/usr/local/ts_tx5368/libs/")
link_directories("/usr/local/ts_tx5368/thirdparty/libs/")
include_directories("/usr/local/ts_tx5368/thirdparty/include/")
endif()

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/video APP_SRC_FILES)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/pic APP_SRC_FILES)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} APP_SRC_FILES)
if(${BUILD_TARGET_CHIP} STREQUAL "rk3588")
    aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/video/rk APP_SRC_FILES)
endif()

add_library(gddeploy_app SHARED ${APP_SRC_FILES})
if(BUILD_TARGET_CHIP MATCHES "rk" AND BUILD_SYSTEM_NAME MATCHES "linux")
    target_link_libraries(gddeploy_app PRIVATE gddeploy_api gddeploy_core gddeploy_register gddeploy_common ${OpenCV_LIBS} ${APP_LIBS} ${CONAN_LIBS} rockchip_mpp rockchip_vpu pthread)
elseif(BUILD_TARGET_CHIP MATCHES "rk" AND BUILD_SYSTEM_NAME MATCHES "android")
    target_link_libraries(gddeploy_app PRIVATE gddeploy_api gddeploy_core gddeploy_register gddeploy_common ${OpenCV_LIBS} ${APP_LIBS} ${CONAN_LIBS} mpp vpu)
else()
    target_link_libraries(gddeploy_app PRIVATE gddeploy_api gddeploy_core gddeploy_register gddeploy_common ${OpenCV_LIBS} ${APP_LIBS} ${CONAN_LIBS} pthread)
endif()

# 检查 PROJECT_APP_VERSION 是否已经设置，如果没有，则默认为 0
if(NOT DEFINED PROJECT_APP_VERSION)
  set(PROJECT_APP_VERSION "0")
endif()

set_target_properties(gddeploy_app PROPERTIES VERSION ${PROJECT_APP_VERSION} SOVERSION 1) 

install(TARGETS gddeploy_app LIBRARY DESTINATION lib)

if(BUILD_ALL_LIB)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/app DESTINATION sample)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/app DESTINATION include)
endif()