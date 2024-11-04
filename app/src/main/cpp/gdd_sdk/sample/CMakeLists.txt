link_directories(${CMAKE_BINARY_DIR}/src/core)
link_directories(${CMAKE_BINARY_DIR}/src/register)
set(Ascend_path /usr/local/Ascend/ascend-toolkit/5.13/arm64-lmixlinux200/runtime/)
# set(Ascend_path /usr/local/Ascend/ascend-toolkit/6.3/aarch64-linux/runtime/)
link_directories(/usr/local/Ascend_3403/lib/npu)
link_directories(/opt/aarch64-mix210-linux/aarch64-mix210-linux/aarch64-linux-gnu/lib64/)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/CommandParser SAMPLE_SRC_COMMON)

if(BUILD_TARGET_CHIP STREQUAL "ts")
    link_directories("/usr/local/ts_tx5368/libs/")
    link_directories("/usr/local/ts_tx5368/thirdparty/libs/")
    include_directories("/usr/local/ts_tx5368/thirdparty/include/")
    # message("##################boost: ")
    #------build sample_runner_pic--------
    link_directories(/usr/local/ts_tx5368/thirdparty/spdlog/lib/)
endif()
link_directories(${FFMPEG_LIBRARY_DIRS})
add_executable(sample_runner_pic sample_runner_pic.cpp ${SAMPLE_SRC_COMMON})

# GDDEPLOY_LIBS link libraries
set(GDDEPLOY_LIBS gddeploy_app gddeploy_api gddeploy_core gddeploy_register gddeploy_common)

if(BUILD_TARGET_CHIP STREQUAL "ts")
    target_link_libraries(sample_runner_pic gddeploy_app gddeploy_api gddeploy_core gddeploy_register spdlog ${Boost_LIBRARIES} boost_atomic mpi drm omx omxc_gpu vpu_jsw vpu_vsw vpu_comm 3a actuator_ms41929 buf_mgr calibration_imx327 cjson cmd_if csi_ar0239 csi_imx327 actuator_ms41929_aux csi_ov5647 frm_mgr isp isp_dpc isp_hal isp_lsh isp_mvd isp_osif isp_top isp_vin json_convert niss pcap sensor sensor_imx327 sensor_imx415 cmdr isp_tpg isp_submod_common sen_modules csi2d_imx327 aecd awbd afcd tuning_imx327_normal aec1d awb1d afc1d ui_uart csi2d_imx415 tuning_imx415_normal calibration_imx415 tuning_imx415_dol2 sensor_os04a10 csi2d_os04a10 tuning_os04a10_normal calibration_os04a10 omxc_gpu_stitch omx GAL)
elseif (BUILD_TARGET_CHIP STREQUAL "ascend3403")
    # set(Ascend_libs ${Ascend_path}/lib64/libascendcl.so msprofiler ge_executor runtime alog drvdevdrv drv_dfx ge_common graph error_manager c_sec /usr/local/Ascend_3403/lib/npu/libmmpa.so ascend_protobuf.so.3.13.0.0 register slog aicpu_scheduler adump aicpu_processer aicpu_prof msprof pthread rt dl)
    set(Ascend_libs ${Ascend_path}/lib64/stub/libascendcl.so )
    target_link_libraries(sample_runner_pic PUBLIC ${Ascend_libs} gddeploy_app gddeploy_api gddeploy_core gddeploy_register gddeploy_common ${Boost_LIBRARIES})
    # target_link_libraries(sample_runner_pic PUBLIC gddeploy_app gddeploy_api gddeploy_core gddeploy_register gddeploy_common ${Boost_LIBRARIES})
elseif(BUILD_TARGET_CHIP MATCHES "rk" AND BUILD_SYSTEM_NAME MATCHES "linux")
    target_link_libraries(sample_runner_pic PUBLIC ${GDDEPLOY_LIBS} rknnrt rga dl pthread)
elseif(BUILD_TARGET_CHIP MATCHES "rk" AND BUILD_SYSTEM_NAME MATCHES "android")
    target_link_libraries(sample_runner_pic PUBLIC ${GDDEPLOY_LIBS} rknnrt rga dl)
else()
    target_link_libraries(sample_runner_pic PUBLIC ${GDDEPLOY_LIBS} )
endif()

install(TARGETS sample_runner_pic LIBRARY DESTINATION bin)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/sample_runner_pic.cpp DESTINATION sample)
message( ${CMAKE_CURRENT_SOURCE_DIR}/sample_runner_pic.cpp)

# # ------build sample_runner_video--------
add_executable(sample_runner_video sample_runner_video.cpp ${SAMPLE_SRC_COMMON})

if(BUILD_TARGET_CHIP STREQUAL "ts")
    target_link_libraries(sample_runner_video gddeploy_app gddeploy_api gddeploy_core gddeploy_register spdlog ${Boost_LIBRARIES} boost_atomic mpi drm omx omxc_gpu vpu_jsw vpu_vsw vpu_comm 3a actuator_ms41929 buf_mgr calibration_imx327 cjson cmd_if csi_ar0239 csi_imx327 actuator_ms41929_aux csi_ov5647 frm_mgr isp isp_dpc isp_hal isp_lsh isp_mvd isp_osif isp_top isp_vin json_convert niss pcap sensor sensor_imx327 sensor_imx415 cmdr isp_tpg isp_submod_common sen_modules csi2d_imx327 aecd awbd afcd tuning_imx327_normal aec1d awb1d afc1d ui_uart csi2d_imx415 tuning_imx415_normal calibration_imx415 tuning_imx415_dol2 sensor_os04a10 csi2d_os04a10 tuning_os04a10_normal calibration_os04a10 omxc_gpu_stitch omx GAL)
elseif(BUILD_TARGET_CHIP MATCHES "rk" AND BUILD_SYSTEM_NAME MATCHES "linux")
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src/app/video)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src/app/video/rk)
    target_link_libraries(sample_runner_video PUBLIC ${GDDEPLOY_LIBS}  rknnrt rga dl pthread)
elseif(BUILD_TARGET_CHIP MATCHES "rk" AND BUILD_SYSTEM_NAME MATCHES "android")
    target_link_libraries(sample_runner_video PUBLIC ${GDDEPLOY_LIBS}  rknnrt rga dl)
else()
    target_link_libraries(sample_runner_video PUBLIC ${GDDEPLOY_LIBS} )
endif()


install(TARGETS sample_runner_video LIBRARY DESTINATION bin)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/sample_runner_video.cpp DESTINATION sample)


# # #build sample_procesor_api
# # # add_executable(sample_procesor_api sample_processor_api.cpp)
# # # target_link_libraries(sample_procesor_api gddeploy_app gddeploy_api gddeploy_core gddeploy_register)

# # # install(TARGETS sample_procesor_api LIBRARY DESTINATION bin)
# # # install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/sample_procesor_api.cpp DESTINATION sample)

#------build sample_runner_dataset--------
add_executable(sample_runner_dataset sample_runner_dataset.cpp ${SAMPLE_SRC_COMMON})
if(BUILD_TARGET_CHIP STREQUAL "ts")
    target_link_libraries(sample_runner_dataset gddeploy_app gddeploy_api gddeploy_core gddeploy_register spdlog ${Boost_LIBRARIES} boost_atomic mpi drm omx omxc_gpu vpu_jsw vpu_vsw vpu_comm 3a actuator_ms41929 buf_mgr calibration_imx327 cjson cmd_if csi_ar0239 csi_imx327 actuator_ms41929_aux csi_ov5647 frm_mgr isp isp_dpc isp_hal isp_lsh isp_mvd isp_osif isp_top isp_vin json_convert niss pcap sensor sensor_imx327 sensor_imx415 cmdr isp_tpg isp_submod_common sen_modules csi2d_imx327 aecd awbd afcd tuning_imx327_normal aec1d awb1d afc1d ui_uart csi2d_imx415 tuning_imx415_normal calibration_imx415 tuning_imx415_dol2 sensor_os04a10 csi2d_os04a10 tuning_os04a10_normal calibration_os04a10 omxc_gpu_stitch omx GAL)
elseif (BUILD_TARGET_CHIP STREQUAL "ascend3403")
    # set(Ascend_libs ${Ascend_path}/lib64/libascendcl.so msprofiler ge_executor runtime alog drvdevdrv drv_dfx ge_common graph error_manager c_sec /usr/local/Ascend_3403/lib/npu/libmmpa.so ascend_protobuf.so.3.13.0.0 register slog aicpu_scheduler adump aicpu_processer aicpu_prof msprof pthread rt dl)
    set(Ascend_libs ${Ascend_path}/lib64/stub/libascendcl.so )
    target_link_libraries(sample_runner_dataset PUBLIC ${Ascend_libs} gddeploy_app gddeploy_api gddeploy_core gddeploy_register gddeploy_common ${Boost_LIBRARIES})
elseif(BUILD_TARGET_CHIP MATCHES "rk" AND BUILD_SYSTEM_NAME MATCHES "linux")
    target_link_libraries(sample_runner_dataset PUBLIC ${GDDEPLOY_LIBS}  rknnrt rga dl pthread)
elseif(BUILD_TARGET_CHIP MATCHES "rk" AND BUILD_SYSTEM_NAME MATCHES "android")
    target_link_libraries(sample_runner_dataset PUBLIC ${GDDEPLOY_LIBS}  rknnrt rga dl)
else()
    target_link_libraries(sample_runner_dataset PUBLIC ${GDDEPLOY_LIBS} )
endif()

install(TARGETS sample_runner_dataset LIBRARY DESTINATION bin)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/sample_runner_dataset.cpp DESTINATION sample)

# install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt DESTINATION sample)
# install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/README.txt DESTINATION docs)

# add_executable(test_only_infer test_only_infer.cpp)
# target_link_libraries(test_only_infer gddeploy_api gddeploy_core gddeploy_register)
# install(TARGETS test_only_infer LIBRARY DESTINATION bin)

# link_directories(${CMAKE_SOURCE_DIR})
# add_executable(test_nvjpeg test_nvjpeg.cpp)
# target_link_libraries(test_nvjpeg PRIVATE gddeploy_app ${Boost_LIBRARIES})
# install(TARGETS test_nvjpeg LIBRARY DESTINATION bin)

add_executable(sample_runner_pic_c sample_runner_pic_c.c)
target_link_libraries(sample_runner_pic_c gddeploy_capi)
if (BUILD_TARGET_CHIP STREQUAL "ascend3403")
    link_directories(/opt/aarch64-mix210-linux/aarch64-mix210-linux/aarch64-linux-gnu/lib64/)
    # set(Ascend_libs ${Ascend_path}/lib64/libascendcl.so msprofiler ge_executor runtime alog drvdevdrv drv_dfx ge_common graph error_manager c_sec /usr/local/Ascend_3403/lib/npu/libmmpa.so ascend_protobuf.so.3.13.0.0 register slog aicpu_scheduler adump aicpu_processer aicpu_prof msprof pthread rt dl)
    set(Ascend_path /usr/local/Ascend/ascend-toolkit/5.13/arm64-lmixlinux200/runtime/)
    set(Ascend_libs ${Ascend_path}/lib64/stub/libascendcl.so )
    target_link_libraries(sample_runner_pic_c  ${Ascend_libs} stdc++)
endif()
install(TARGETS sample_runner_pic_c LIBRARY DESTINATION bin)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/sample_runner_pic_c.c DESTINATION sample)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/CommandParser DESTINATION sample)

add_executable(sample_api_sync_only_infer sample_api_sync_only_infer.cpp ${SAMPLE_SRC_COMMON})
target_link_libraries(sample_api_sync_only_infer gddeploy_capi)
install(TARGETS sample_api_sync_only_infer LIBRARY DESTINATION bin)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/sample_api_sync_only_infer.cpp DESTINATION sample)


if (BUILD_TARGET_CHIP MATCHES "rk" AND BUILD_SYSTEM_NAME MATCHES "android")
    add_executable(sample_android sample_android.cpp ${SAMPLE_SRC_COMMON})
    target_link_libraries(sample_android gddeploy_capi ${GDDEPLOY_LIBS} ${OpenCV_LIBS} spdlog rknnrt rga dl c++_shared -Wl,--no-undefined)
    install(TARGETS sample_android LIBRARY DESTINATION bin)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/sample_android.cpp DESTINATION sample)
endif()
