VzDeviceSDK.h，用于引用SDK的接口定义，编译基础库singlelibrary.a，要实时和驱动文件保持一致。
libVzDeviceSDK_Dumy.a，用于HISIV300等测试程序link，但功能无效。

编译命令：
arm-hisiv300-linux-g++ -c VzDeviceSDK_Dumy.cpp
arm-hisiv300-linux-ar cr libVzDeviceSDK_Dumy.a VzDeviceSDK_Dumy.o