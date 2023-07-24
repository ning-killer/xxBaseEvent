# -G "Visual Studio 9 2008"
# First: cd in bulids/type/ (eg: cd builds/arm3730/
# Then, use the below command
#build with arm3730:  cmake ../.. -DCMAKE_TOOLCHAIN_FILE=../../arm_make.cmake -DCMAKE_BUILD_TYPE=Release
#build with vs2013 :  cmake ../.. -DCMAKE_BUILD_TYPE=Debug -G "Visual Studio 12 2013"
#build with vs2008 :  cmake ../.. -DCMAKE_BUILD_TYPE=Debug -G "Visual Studio 9 2008"
#bjam --toolset=msvc-9.0 --build-type=complete link=static threading=multi runtime-link=static --with-thread
# http://blog.csdn.net/jwybobo2007/article/details/7242307
# ./b2 link=static threading=multi runtime-link=static --with-thread
# CMAKE_BUILD_PLATFORM=HISI
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
##########################################################
## Step 1 
SET(VZ_S_BASE_DIR "F:/share/vz_s_baseclass")
SET(BOOST_1_56_DIR "F:/share/boost_1_53_0")
SET(INTERFACE_INCLUDE_DIR "F:/share/IPNC_Build_Env/pub/interface_newhwinfo/include")
SET(INTERFACE_LIBRARY_DIR "F:/share/IPNC_Build_Env/pub/interface_newhwinfo/lib/arm3730/lib")

##1. INCLUDE_DIRECTORES
SET(VZPROJECT_INCLUDE_DIR
	${VZ_S_BASE_DIR}/vz_log/glog-0.3.3/src/windows
	${VZ_S_BASE_DIR}/zeromq/libzmq/include
	${PROJECT_SOURCE_DIR}/src/third_part/googletest/googletest/include
	${PROJECT_SOURCE_DIR}/src/third_part/googletest/googlemock/include
	${BOOST_1_56_DIR}
	# Add Json DIR
	# ${VZ_S_BASE_DIR}/app_headers
	${VZ_S_BASE_DIR}/jsoncpp/include
	${PROJECT_SOURCE_DIR}/src/lib
	${INTERFACE_INCLUDE_DIR}
)

MESSAGE(STATUS "       Add library path and name")
if(CMAKE_BUILD_TYPE MATCHES Debug)
	MESSAGE(STATUS "       Debug Mode")
	MESSAGE(STATUS "       MSVC12")
	SET(VZPROJECT_LIBRARY_DIR
		${VZ_S_BASE_DIR}/vz_log/glog-0.3.3/lib/vs2008/
		${VZ_S_BASE_DIR}/zeromq/libzmq/lib/vs2008
		${BOOST_1_56_DIR}/stage/lib
		# Add Json library
		${VZ_S_BASE_DIR}/jsoncpp/lib/vs2008
		${PROJECT_SOURCE_DIR}/lib/vs2008
		${GOOGLE_PROTOBUF}/lib/vs2008/Debug
		)
	SET(VZPROJECT_LINK_LIB
		libglog_staticd.lib
		json_vc71_libmtd.lib
		libsqlite3.lib
		ws2_32.lib
		libkvdb.lib
		libvzconn.lib
		libzmq_d.lib
		)
elseif(CMAKE_BUILD_TYPE MATCHES Release)
	MESSAGE(STATUS "       Release Mode")
	MESSAGE(STATUS "       MSVC12")
	SET(VZPROJECT_LIBRARY_DIR
		${VZ_S_BASE_DIR}/glog-0.3.3/lib/vs2008/
		${VZ_S_BASE_DIR}/zeromq/libzmq/lib/vs2008
		${BOOST_1_56_DIR}/stage/lib
		# Add Json library
		${VZ_S_BASE_DIR}/jsoncpp/lib/vs2008
		${PROJECT_SOURCE_DIR}/lib/vs2008
		${GOOGLE_PROTOBUF}/lib/vs2008/Release
		)
	SET(VZPROJECT_LINK_LIB
		libzmq.lib
		libglog_static.lib
		libsqlite3.lib
		json_vc71_libmtd.lib
		ws2_32.lib
		)
endif()

MESSAGE(STATUS "Step 4 : Add code source")
ADD_DEFINITIONS(-DGOOGLE_GLOG_DLL_DECL=
	-DGLOG_NO_ABBREVIATED_SEVERITIES
	-D_CRT_SECURE_NO_WARNINGS
	-D_WINSOCK_DEPRECATED_NO_WARNINGS
	-D_WIN32_WINNT=0x0502
	-D_SCL_SECURE_NO_WARNINGS
	-DZMQ_STATIC
    -DHAVE_GLOG
)

#####################################################################
# Step 3 :Set visual studio runtime type
set(CompilerFlags
		CMAKE_CXX_FLAGS
		CMAKE_CXX_FLAGS_DEBUG
		CMAKE_CXX_FLAGS_RELEASE
		CMAKE_C_FLAGS
		CMAKE_C_FLAGS_DEBUG
		CMAKE_C_FLAGS_RELEASE
		)
foreach(CompilerFlag ${CompilerFlags})
  string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
  string(REPLACE "/MDd" "/MTd" ${CompilerFlag} "${${CompilerFlag}}")
  #string(REPLACE "/EDITANDCONTINUE" "/SAFESEH" ${CompilerFlag} "${${CompilerFlag}}")
endforeach()

SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY  "${PROJECT_SOURCE_DIR}/bin/vs2008")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY  "${PROJECT_SOURCE_DIR}/lib/vs2008")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY  "${PROJECT_SOURCE_DIR}/lib/vs2008")

SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG  "${PROJECT_SOURCE_DIR}/bin/vs2008")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG  "${PROJECT_SOURCE_DIR}/lib/vs2008")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG  "${PROJECT_SOURCE_DIR}/lib/vs2008")

# With Release properties
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE  "${PROJECT_SOURCE_DIR}/bin/vs2008")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE  "${PROJECT_SOURCE_DIR}/lib/vs2008")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE  "${PROJECT_SOURCE_DIR}/lib/vs2008")