aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} COMMON_SRC_FILES)

# find_package(Boost 1.81 REQUIRED COMPONENTS locale)

add_library(gddeploy_common SHARED ${COMMON_SRC_FILES})
target_link_libraries(gddeploy_common spdlog ${FFMPEG_LIBRARIES} opencv_imgproc opencv_core)
include_directories(${CMAKE_SOURCE_DIR}/include/common)

if(${BUILD_TARGET_CHIP} STREQUAL "nvidia")
target_link_libraries(gddeploy_common cudart)
endif()

set_target_properties(gddeploy_common PROPERTIES VERSION ${PROJECT_COMMON_VERSION} SOVERSION 1) 

install(TARGETS gddeploy_common LIBRARY DESTINATION lib)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/common DESTINATION sample)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/common DESTINATION include)

if(${BUILD_TARGET_CHIP} STREQUAL "rk3588")
    install(FILES ${CMAKE_SOURCE_DIR}/include/common/rk_rga_mem.h DESTINATION include/common)
endif()