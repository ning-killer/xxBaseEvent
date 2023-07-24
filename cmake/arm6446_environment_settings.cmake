include(CMakeForceCompiler)
# this one is important
SET(CMAKE_SYSTEM_NAME Linux)
set( CMAKE_SYSTEM_PROCESSOR arm ) 
#this one not so much
SET(CMAKE_SYSTEM_VERSION 1)

# specify the cross compiler
SET(CMAKE_C_COMPILER   arm_v5t_le-gcc)
SET(CMAKE_CXX_COMPILER arm_v5t_le-g++)
SET(CMAKE_STRIP arm_v5t_le-strip)
#CMAKE_FORCE_C_COMPILER(arm-arago-linux-gnueabi-gcc GNU)
#CMAKE_FORCE_CXX_COMPILER(arm-arago-linux-gnueabi-g++ GNU)
# where is the target environment 
#SET(CMAKE_FIND_ROOT_PATH  "/mnt/hgfs/Share/cmake project/libvznet")

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

############################################################################################
## Step 1 
SET(VZ_S_BASE_DIR "/mnt/hgfs/vz_s_base")
SET(BOOST_1_53_DIR "/mnt/hgfs/boost_1_53_0")
SET(VSCP "/mnt/hgfs/work/vscp/code/branches/sip")
SET(OSA "/mnt/hgfs/work/event_server/osa/_linux")
SET(INTERFACE_DIR "/mnt/hgfs/work/event_server/OrgCode/branch_star/ipnc_app/interface_arm6446/interface_arm6446")
SET(CMEM_DIR "/home/davinci/dvsdk_2_00_00_22/linuxutils_2_23_01/packages/ti/sdo/linuxutils/cmem")
SET(GLOG_DIR "/mnt/hgfs/vz_s_base/vz_log/arm6446/glog")

SET(VZPROJECT_INCLUDE_DIR
	${VZ_S_BASE_DIR}/vz_log/glog-0.3.3/src
	${VZ_S_BASE_DIR}/zeromq/libzmq/include
	${BOOST_1_53_DIR}
	# Add Json DIR
	${VZ_S_BASE_DIR}/app_headers
	${VZ_S_BASE_DIR}/jsoncpp/include
	${PROJECT_SOURCE_DIR}/src/lib
	#${VZ_S_BASE_DIR}/Vz_head
    ${INTERFACE_DIR}/inc
    ${GLOG_DIR}/builds/arm6446
    ${GLOG_DIR}/src
)
MESSAGE(STATUS "       Add library path and name")
if(CMAKE_BUILD_TYPE MATCHES Debug)
	MESSAGE(STATUS "       Debug Mode")
	SET(VZPROJECT_LIBRARY_DIR
        ${GLOG_DIR}/builds/arm6446
		${BOOST_1_53_DIR}/stage_arm6446/lib
		${VZ_S_BASE_DIR}/jsoncpp/lib/arm6446
		${PROJECT_SOURCE_DIR}/lib/arm6446
		# Add Json library
		${VZ_S_BASE_DIR}/zeromq/libzmq/lib/arm6446
	    ${PROJECT_SOURCE_DIR}/src/osa/_linux/lib/arm6446
		)
	SET(VZPROJECT_LINK_LIB
        -lunwind
		libglog.a
		# Add Json library
		libboost_thread.a
		libboost_system.a
		libboost_random.a
        libosa.a
		libsqlite3.a
        libnetsqlite.a
        libzmq-static.a
		libjsoncpp.a
        ${INTERFACE_DIR}/lib/vz_hwi_sharemem.a
        ${INTERFACE_DIR}/lib/vz_sharemem.a
        ${INTERFACE_DIR}/lib/getSN.a
        ${INTERFACE_DIR}/lib/dm3730_fs8816.a
        ${INTERFACE_DIR}/lib/dm3730_gpio.a
        ${INTERFACE_DIR}/lib/ApproDrvMsg.a
        ${INTERFACE_DIR}/lib/dm3730_FS8816V2.88.a
        ${INTERFACE_DIR}/lib/sem_util.a
        ${INTERFACE_DIR}/lib/msg_util.a
        ${INTERFACE_DIR}/lib/file_msg_drv.a
        ${INTERFACE_DIR}/lib/share_mem.a
        ${INTERFACE_DIR}/lib/onvif_state.a
        ${CMEM_DIR}/lib/cmem.a470MV
        -lcrypto
        -Wl,-rpath=.
		pthread
		rt
        -lgcc
		)
elseif(CMAKE_BUILD_TYPE MATCHES Release)
	MESSAGE(STATUS "       Release Mode")
	SET(VZPROJECT_LIBRARY_DIR
        ${GLOG_DIR}/builds/arm6446
        ${BOOST_1_53_DIR}/stage_arm6446/lib
        ${VZ_S_BASE_DIR}/jsoncpp/lib/arm6446
        ${PROJECT_SOURCE_DIR}/lib/arm6446
		)
	SET(VZPROJECT_LINK_LIB
        -lunwind
        libglog.a
        libsqlite3.a
        libjsoncpp.a
        libboost_thread.a
        libboost_system.a
        libboost_chrono.a
        libboost_random.a
        -lcrypto
        -Wl,-rpath=.
		pthread
		rt
        -lgcc
		)
endif()

MESSAGE(STATUS "Step 4 : Add code source")
ADD_DEFINITIONS(-DGOOGLE_GLOG_DLL_DECL=
	-DGLOG_NO_ABBREVIATED_SEVERITIES
	-DO2
    -D_LINUX
    -DDM3730
    -DONVIF
    -DONVIF_SEPARATE_CFG
    -DONVIF_USB_LAN
    -DLINK_8816
    -DWITH_DOM
    -DGENERIC_RELAY
    -DSVN_REVISION=$(SVN_REVISION)
    -DHAVE_GLOG
	)
################################################################
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY  "${PROJECT_SOURCE_DIR}/bin/arm6446")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY  "${PROJECT_SOURCE_DIR}/lib/arm6446")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY  "${PROJECT_SOURCE_DIR}/lib/arm6446")

SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG  "${PROJECT_SOURCE_DIR}/bin/arm6446")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG  "${PROJECT_SOURCE_DIR}/lib/arm6446")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG  "${PROJECT_SOURCE_DIR}/lib/arm6446")

# With Release properties
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE  "${PROJECT_SOURCE_DIR}/bin/arm6446")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE  "${PROJECT_SOURCE_DIR}/lib/arm6446")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE  "${PROJECT_SOURCE_DIR}/lib/arm6446")