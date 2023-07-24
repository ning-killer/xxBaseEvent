include(CMakeForceCompiler)
# this one is important
SET(CMAKE_SYSTEM_NAME Linux)
set( CMAKE_SYSTEM_PROCESSOR arm ) 
#this one not so much
SET(CMAKE_SYSTEM_VERSION 1)

# specify the cross compiler
SET(CMAKE_C_COMPILER   arm-arago-linux-gnueabi-gcc)
SET(CMAKE_CXX_COMPILER arm-arago-linux-gnueabi-g++)
SET(CMAKE_STRIP arm-arago-linux-gnueabi-strip)

#CMAKE_FORCE_C_COMPILER(arm-arago-linux-gnueabi-gcc GNU)
#CMAKE_FORCE_CXX_COMPILER(arm-arago-linux-gnueabi-g++ GNU)
# where is the target environment 
#SET(CMAKE_FIND_ROOT_PATH  "/mnt/hgfs/Share/cmake project/libvznet")

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

OPTION(BULID_SELF_8127 "Build self 8127" OFF)

SET(VZPROJECT_INCLUDE_DIR
	${PROJECT_SOURCE_DIR}/src/lib
	${PROJECT_SOURCE_DIR}/src/third_part/vzbase/src/lib
	${PROJECT_SOURCE_DIR}/src/third_part/vzbase/src/third_part/boost
	${PROJECT_SOURCE_DIR}/src/third_part/vzbase/src/third_part
	${PROJECT_SOURCE_DIR}/src/third_part/vzbase/src/third_part/libevent-2.1.8/arm3730/include
	${PROJECT_SOURCE_DIR}/src/third_part/vzbase/src/third_part/libcurl-7.54.0/arm3730/include
	)

MESSAGE(STATUS "       Add LIBRARY PATH AND NAME")
IF(CMAKE_BUILD_TYPE MATCHES Debug)
	MESSAGE(STATUS "       Debug Mode")
	SET(VZPROJECT_LIBRARY_DIR
		${PROJECT_SOURCE_DIR}/src/third_part/vzbase/src/third_part/libevent-2.1.8/arm3730/lib
		${PROJECT_SOURCE_DIR}/src/third_part/vzbase/src/third_part/libcurl-7.54.0/arm3730/lib
		${PROJECT_SOURCE_DIR}/lib/arm8127
		)
	SET(VZPROJECT_LINK_LIB
        -lcrypto
        -Wl,-rpath=.
		pthread
		rt
		)
ELSEIF(CMAKE_BUILD_TYPE MATCHES Release)
	MESSAGE(STATUS "       RELEASE MODE")
	MESSAGE(STATUS "       UNIX")
	SET(VZPROJECT_LIBRARY_DIR
		${PROJECT_SOURCE_DIR}/src/third_part/vzbase/src/third_part/libevent-2.1.8/arm3730/lib
		${PROJECT_SOURCE_DIR}/src/third_part/vzbase/src/third_part/libcurl-7.54.0/arm3730/lib
		${PROJECT_SOURCE_DIR}/lib/arm8127
		)
	SET(VZPROJECT_LINK_LIB
        -Wl,-rpath=.
		pthread
		rt
		)
ENDIF()

MESSAGE(STATUS "SETP 4 : ADD CODE SOURCE")

ADD_DEFINITIONS(
	-DO2
    -D_LINUX
    -DDM8127
    -DGENERIC_RELAY
	)
   
IF (BULID_SELF_8127) 
MESSAGE(STATUS "Define BULID_SELF_8127")
	ADD_DEFINITIONS(-DDEFINE_SELF_8127)
	SET(FTP_DIR "/mnt/hgfs/vscp/code/branches/self_8127_ftp")
ELSE()
	SET(FTP_DIR "/mnt/hgfs/vscp/code/branches/8127_ftp")
ENDIF()

SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY  "${PROJECT_SOURCE_DIR}/bin/arm8127")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY  "${PROJECT_SOURCE_DIR}/lib/arm8127")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY  "${PROJECT_SOURCE_DIR}/lib/arm8127")

SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG  "${PROJECT_SOURCE_DIR}/bin/arm8127")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG  "${PROJECT_SOURCE_DIR}/lib/arm8127")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG  "${PROJECT_SOURCE_DIR}/lib/arm8127")

# With Release properties
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE  "${PROJECT_SOURCE_DIR}/bin/arm8127")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE  "${PROJECT_SOURCE_DIR}/lib/arm8127")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE  "${PROJECT_SOURCE_DIR}/lib/arm8127")


