aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} API_SRC_FILES)

add_library(gddeploy_api SHARED ${API_SRC_FILES}) # ${REGISTER_SRC_FILES})
target_link_libraries(gddeploy_api ${CONAN_LIBS} gddeploy_core) 

set_target_properties(gddeploy_api PROPERTIES VERSION ${PROJECT_API_VERSION} SOVERSION 1) 

set_target_properties(gddeploy_api PROPERTIES VERSION ${PROJECT_API_VERSION} SOVERSION 1) 

install(TARGETS gddeploy_api LIBRARY DESTINATION lib)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/api DESTINATION sample)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/api DESTINATION include)