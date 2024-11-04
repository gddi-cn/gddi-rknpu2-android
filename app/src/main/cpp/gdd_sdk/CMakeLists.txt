cmake_minimum_required (VERSION 3.10)
project(gddeploy)
set(CMAKE_VERBOSE_MAKEFILE ON)

file(GLOB CMakeFiles "${CMAKE_SOURCE_DIR}/cmake/*.cmake")
foreach (cmake_module_file IN LISTS CMakeFiles)
    # get_filename_component(CMakeName ${cmake_module_file} NAME_WLE)
    get_filename_component(CMakePath ${cmake_module_file} DIRECTORY)
    message("LoadCMake: ${CMakeName}")
    include(${cmake_module_file})
endforeach (cmake_module_file)

message("CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
message("CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")

include_directories(${CMAKE_SOURCE_DIR}/include)
link_directories(${CMAKE_SOURCE_DIR}/lib)

include_directories("${CMAKE_SOURCE_DIR}/sample/")

# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/sample/app)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/sample/CommandParser SAMPLE_SRC_COMMON)

set(GDDEPLOY_LIBS gddeploy_app gddeploy_api gddeploy_core gddeploy_register gddeploy_common)

message("CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message("COMMON_LIB: ${COMMON_LIB}")
set(LD_FLAGS "-v -Wl,--no-undefined")
 
#------build sample_runner_pic--------
add_executable(sample_runner_pic ${CMAKE_SOURCE_DIR}/sample/sample_runner_pic.cpp ${SAMPLE_SRC_COMMON})
target_link_libraries(sample_runner_pic PRIVATE ${LD_FLAGS} ${GDDEPLOY_LIBS} ${COMMON_LIB} opencv_imgcodecs opencv_imgproc opencv_core)
install(TARGETS sample_runner_pic RUNTIME DESTINATION bin)

#------build sample_runner_video--------
add_executable(sample_runner_video ${CMAKE_SOURCE_DIR}/sample/sample_runner_video.cpp ${SAMPLE_SRC_COMMON})
target_link_libraries(sample_runner_video PRIVATE ${LD_FLAGS} ${GDDEPLOY_LIBS} ${COMMON_LIB})
install(TARGETS sample_runner_video RUNTIME DESTINATION bin)

#------build sample_runner_dataset--------
add_executable(sample_runner_dataset ${CMAKE_SOURCE_DIR}/sample/sample_runner_dataset.cpp ${SAMPLE_SRC_COMMON})
target_link_libraries(sample_runner_dataset PRIVATE ${LD_FLAGS} ${GDDEPLOY_LIBS} ${COMMON_LIB})
install(TARGETS sample_runner_dataset RUNTIME DESTINATION bin)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/lib/ DESTINATION lib2)

