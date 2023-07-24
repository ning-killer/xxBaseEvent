include(CMakeForceCompiler)
SET(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm ) 
SET(CMAKE_SYSTEM_VERSION 1)

SET(CMAKE_C_COMPILER   arm-himix100-linux-gcc)
SET(CMAKE_CXX_COMPILER arm-himix100-linux-g++)
SET(CMAKE_STRIP arm-himix100-linux-strip)

SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

SET(FTP_DIR "${PROJECT_SOURCE_DIR}/ftp/hisi")	
SET(BUILD_AR arm-himix100-linux-ar)

SET(HiSiSDK_DIR ${PROJECT_SOURCE_DIR}/src/third_part/C-camera/Hi3516CV300_SDK_V1.0.3.0)

SET(VZPROJECT_INCLUDE_DIR
	${PROJECT_SOURCE_DIR}/src/lib
	${PROJECT_SOURCE_DIR}/src/third_part
	${PROJECT_SOURCE_DIR}/src/third_part/boost
	#Liteos defined include dirs
	${HiSiSDK_DIR}/mpp/include
	${HiSiSDK_DIR}/drv/extdrv/DeviceSDK_LiteOS
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/platform/bsp/hi3516cv300 
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/platform/cpu/arm/include 
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/platform/bsp/hi3516cv300/config 
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/platform/bsp/hi3516cv300/mmu 
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/platform/bsp/hi3516cv300/include 
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/platform/bsp/common 
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/compat/posix/include 
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/compat/posix/src 
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/compat/linux/include 
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/net/lwip_sack/include 
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/net/lwip_sack/include/ipv4 
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/net/mac 
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/lib/include 
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/lib/libc/include
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/lib/libsec/include 
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/lib/libm/include 
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/lib/zlib/include 
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/compat/posix/include 
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/compat/posix/src 
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/compat/linux/include 
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/lib/libc/kernel/uapi/asm-arm 
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/lib/libc/arch-arm/include
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/lib/include
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/lib/libc/include
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/lib/libsec/include 
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/lib/libm/include
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/lib/zlib/include
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/platform/bsp/hi3516cv300 
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/platform/cpu/arm/include 
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/platform/bsp/hi3516cv300/config 
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/platform/bsp/hi3516cv300/mmu
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/platform/bsp/hi3516cv300/include
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/platform/bsp/common
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/lib/cxxstl/c++/6.3.0 
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/lib/cxxstl/c++/6.3.0/ext 
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/lib/cxxstl/c++/6.3.0/backward
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/compat/posix/include 
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/lib/libm/include
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/lib/libc/include 
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/fs/include 
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/kernel/include 
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/lib/libc/kernel/uapi/asm-arm
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/lib/libc/arch-arm/include
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/lib/include
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/lib/libc/include 
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/lib/libc/kernel/uapi
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/lib/libsec/include
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/lib/cxxstl/gccinclude 
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/lib/cxxstl/gdbinclude 
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/lib/cxxstl/c++/6.3.0/arm-linux-androideabi 
	${HiSiSDK_DIR}/osdrv/opensource/liteos/liteos/kernel/base/include
)

SET(VZPROJECT_LIBRARY_DIR
)

# Liteos SDK defined C&C++ flags
#set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -mcpu=arm926ej-s -fno-aggressive-loop-optimizations -fno-builtin -nostdinc -nostdlib -mno-unaligned-access -fstack-protector -Wnonnull -std=c99 -Wpointer-arith -Wstrict-prototypes -Wno-write-strings -mthumb-interwork -ffunction-sections -fdata-sections -fno-exceptions -fno-short-enums -fno-omit-frame-pointer --param ssp-buffer-size=4 ")
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -mcpu=arm926ej-s -std=c++11 -nostdlib -nostdinc -nostdinc++ -fexceptions -fpermissive -fno-use-cxa-atexit -fno-builtin -frtti")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os -fno-aggressive-loop-optimizations -fno-builtin -nostdinc -mno-unaligned-access -fstack-protector -Wno-deprecated-declarations -Wnonnull -std=c99 -Wpointer-arith -Wstrict-prototypes -Wno-write-strings -fno-short-enums -fno-omit-frame-pointer -ffunction-sections -fdata-sections")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -std=c++11 -nostdinc -nostdinc++ -fexceptions -fpermissive -fno-use-cxa-atexit -fno-builtin -frtti -Wno-deprecated-declarations -ffunction-sections -fdata-sections")

MESSAGE(STATUS "       Add LIBRARY PATH AND NAME")
IF(CMAKE_BUILD_TYPE MATCHES Debug)
	MESSAGE(STATUS "       Debug Mode")
	SET(VZPROJECT_LIBRARY_DIR
		)
	SET(VZPROJECT_LINK_LIB
        -Wl,-rpath=.
		pthread
		rt
		)
ELSEIF(CMAKE_BUILD_TYPE MATCHES Release)
	MESSAGE(STATUS "       RELEASE MODE")
	MESSAGE(STATUS "       UNIX")
	SET(VZPROJECT_LIBRARY_DIR
		)
	SET(VZPROJECT_LINK_LIB
        -Wl,-rpath=.
		pthread
		rt
		)
ENDIF()

MESSAGE(STATUS "SETP 4 : ADD CODE SOURCE")

ADD_DEFINITIONS(
	# EventBase macros define
	-DO2
    -D_LINUX
    -DPOSIX
    -DGENERIC_RELAY
	-fpermissive
    -DLITEOS
	-DMULTI_PART_STORAGE
	# Liteos SDK macros define
	-D__COMPILER_HUAWEILITEOS__
	-D__LITEOS__
	-D__HuaweiLite__
	-D__KERNEL__
	-DHI3516CV300
	-DLOSCFG_ARM926
	-DLOSCFG_MEM_WATERLINE
	-DLOSCFG_LIB_LIBC
	-DLOSCFG_LIB_LIBM
	-DLOSCFG_FS_VFS
	-DLOSCFG_FS_FAT
	-DLOSCFG_FS_FAT_CACHE
	-DLOSCFG_FS_FAT_CHINESE
	-DLOSCFG_FS_RAMFS
	-DLOSCFG_FS_YAFFS
	-DLOSCFG_FS_PROC
	-DLOSCFG_FS_JFFS
	-DLOSCFG_NET_LWIP_SACK
	-DLWIP_BSD_API
	-DLWIP_DEBUG
	-DLOSCFG_NET_LWIP_SACK_TFTP
	-DLOSCFG_SHELL
	-DLOSCFG_SHELL_EXCINFO
	-DLOSCFG_NET_TELNET
	)

SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY  "${PROJECT_SOURCE_DIR}/bin/liteos")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY  "${PROJECT_SOURCE_DIR}/lib/liteos")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY  "${PROJECT_SOURCE_DIR}/lib/liteos")

SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG  "${PROJECT_SOURCE_DIR}/bin/liteos")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG  "${PROJECT_SOURCE_DIR}/lib/liteos")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG  "${PROJECT_SOURCE_DIR}/lib/liteos")

# With Release properties
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE  "${PROJECT_SOURCE_DIR}/bin/liteos")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE  "${PROJECT_SOURCE_DIR}/lib/liteos")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE  "${PROJECT_SOURCE_DIR}/lib/liteos")

