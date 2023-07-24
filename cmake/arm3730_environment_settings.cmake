include(CMakeForceCompiler)
# this one is important
SET(CMAKE_SYSTEM_NAME Linux)
set( CMAKE_SYSTEM_PROCESSOR arm ) 
#this one not so much
SET(CMAKE_SYSTEM_VERSION 1)

# specify the cross compiler
SET(CMAKE_C_COMPILER   /home/davinci/dm3730/dvsdk4_03/linux-devkit/arm-arago-linux-gnueabi/bin/gcc)
SET(CMAKE_CXX_COMPILER /home/davinci/dm3730/dvsdk4_03/linux-devkit/arm-arago-linux-gnueabi/bin/g++)
SET(CMAKE_STRIP /home/davinci/dm3730/dvsdk4_03/linux-devkit/arm-arago-linux-gnueabi/bin/strip)

#CMAKE_FORCE_C_COMPILER(arm-arago-linux-gnueabi-gcc GNU)
#CMAKE_FORCE_CXX_COMPILER(arm-arago-linux-gnueabi-g++ GNU)
# where is the target environment 
#SET(CMAKE_FIND_ROOT_PATH  "/mnt/hgfs/Share/cmake project/libvznet")

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

SET(VZ_S_BASE_DIR "/mnt/hgfs/work/vz_s_baseclass")
SET(BOOST_1_53_DIR "/mnt/hgfs/cwork/boost_1_53_0")
#SET(OSA "/mnt/hgfs/share/vz_s_baseclass/osa/inc")
#SET(INTERFACE_INCLUDE_DIR "/mnt/hgfs/share/IPNC_Build_Env/pub/interface_newhwinfo/include")
#SET(INTERFACE_LIBRARY_DIR "/mnt/hgfs/share/IPNC_Build_Env/pub/interface_newhwinfo/lib/arm3730/lib")
SET(INTERFACE_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/src/third_part/interface/inc)
SET(INTERFACE_LIBRARY_DIR ${PROJECT_SOURCE_DIR}/src/third_part/interface/lib)
SET(CMEM_DIR "/home/davinci/dm3730/dvsdk4_03/linuxutils_2_26_02_05/packages/ti/sdo/linuxutils/cmem")

SET(VZPROJECT_INCLUDE_DIR
	# SystemServer修改系统Admin账号密码的库
	${PROJECT_SOURCE_DIR}/src/third_part/passwd/include
	# BusinessServer里面使用的ZMQ库
	${PROJECT_SOURCE_DIR}/src/third_part/libzmq/include
	# BOOST库
	${BOOST_1_53_DIR}
	# Add Json DIR
	#${VZ_S_BASE_DIR}/app_headers
	${PROJECT_SOURCE_DIR}/src/lib
	# 很多地方使用的Interface库
	${PROJECT_SOURCE_DIR}/src/third_part/interface/inc
    # ${GLOG_DIR}/builds/arm6446
    # ${GLOG_DIR}/src
	# 虚拟机系统里面直接就有
    /opt/parted-3.1/include	
	
	${PROJECT_SOURCE_DIR}/src/third_part/libcurl
	
	${PROJECT_SOURCE_DIR}/src/third_part
	
	
	${PROJECT_SOURCE_DIR}/src/third_part/vzbase/src/lib
	${PROJECT_SOURCE_DIR}/src/third_part/vzbase/src/third_part/boost
	${PROJECT_SOURCE_DIR}/src/third_part/vzbase/src/third_part/libevent-2.1.8/arm3730/include
	${PROJECT_SOURCE_DIR}/src/third_part/vzbase/src/third_part
)

MESSAGE(STATUS "       Add LIBRARY PATH AND NAME")
# Linux下面，不需要care Debug版本
IF(CMAKE_BUILD_TYPE MATCHES Debug)
	MESSAGE(STATUS "       Debug Mode")
	SET(VZPROJECT_LIBRARY_DIR
		)
	SET(VZPROJECT_LINK_LIB
		)
ELSEIF(CMAKE_BUILD_TYPE MATCHES Release)
	MESSAGE(STATUS "       RELEASE MODE")
	MESSAGE(STATUS "       UNIX")
	SET(VZPROJECT_LIBRARY_DIR
		${BOOST_1_53_DIR}/stage/lib
		# Add Json library
		${PROJECT_SOURCE_DIR}/src/third_part/libzmq/lib/arm3730
		${PROJECT_SOURCE_DIR}/lib/arm3730
		/home/davinci/dm3730/dvsdk4_03/linux-devkit/arm-arago-linux-gnueabi/usr/include
		${PROJECT_SOURCE_DIR}/src/third_part/interface/lib
		${PROJECT_SOURCE_DIR}/src/third_part/libcurl/lib/arm3730
		
		${PROJECT_SOURCE_DIR}/src/third_part/vzbase/lib/arm3730
		${PROJECT_SOURCE_DIR}/src/third_part/vzbase/src/third_part/libevent-2.1.8/arm3730/lib
		)
	SET(VZPROJECT_LINK_LIB
		#/opt/parted-3.1/libparted/.libs/libparted.so
		libvzlogging.a
		# Add Json library
		libboost_thread.a
		libboost_system.a
        libboost_chrono.a
		libboost_random.a
		libjsoncpp.so
        ${INTERFACE_LIBRARY_DIR}/vz_hwi_sharemem.a
        ${INTERFACE_LIBRARY_DIR}/vz_sharemem.a
        ${INTERFACE_LIBRARY_DIR}/getSN.a        		
        ${INTERFACE_LIBRARY_DIR}/dm3730_gpio.a
        ${INTERFACE_LIBRARY_DIR}/ApproDrvMsg.a 
        ${INTERFACE_LIBRARY_DIR}/file_msg_drv.a
        ${INTERFACE_LIBRARY_DIR}/sem_util.a
        ${INTERFACE_LIBRARY_DIR}/msg_util.a
        ${INTERFACE_LIBRARY_DIR}/share_mem.a
        ${INTERFACE_LIBRARY_DIR}/onvif_state.a
		${INTERFACE_LIBRARY_DIR}/net_config.a
        ${CMEM_DIR}/lib/cmem.a470MV
		${INTERFACE_LIBRARY_DIR}/dm3730_fs8816.a
		${INTERFACE_LIBRARY_DIR}/dm3730_FS8816V2.88.a
        -lcrypto
        -Wl,-rpath=.
		pthread
		rt
		)
ENDIF()

MESSAGE(STATUS "SETP 4 : ADD CODE SOURCE")

ADD_DEFINITIONS(-DGOOGLE_GLOG_DLL_DECL=
	-DO3
    -D_LINUX
    -DDM3730
    -DONVIF
	-DNEW_ONVIF
    -DONVIF_SEPARATE_CFG
    -DONVIF_USB_LAN
    -DLINK_8816
    -DWITH_DOM
    -DGENERIC_RELAY
    -DSVN_REVISION=$(SVN_REVISION)
	)
    
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY  "${PROJECT_SOURCE_DIR}/bin/arm3730")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY  "${PROJECT_SOURCE_DIR}/lib/arm3730")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY  "${PROJECT_SOURCE_DIR}/lib/arm3730")

SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG  "${PROJECT_SOURCE_DIR}/bin/arm3730")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG  "${PROJECT_SOURCE_DIR}/lib/arm3730")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG  "${PROJECT_SOURCE_DIR}/lib/arm3730")

# With Release properties
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE  "${PROJECT_SOURCE_DIR}/bin/arm3730")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE  "${PROJECT_SOURCE_DIR}/lib/arm3730")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE  "${PROJECT_SOURCE_DIR}/lib/arm3730")


