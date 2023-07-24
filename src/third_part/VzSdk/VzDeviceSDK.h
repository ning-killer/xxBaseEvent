#ifndef _VZ_DEVICE_SDK_H_
#define _VZ_DEVICE_SDK_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#ifndef WIN32
#include <unistd.h>
#else
typedef unsigned int loff_t;
#endif
#if defined (__cplusplus)
extern "C" {
#endif

#ifndef __OS_LINUX
#define __OS_LINUX
#endif

/*#ifndef __OS_LITEOS
#define __OS_LITEOS
#endif*/

typedef enum _VZ_BOARD_VERSION
{
	VZ_BOARD_VERSION_C = 0x3007,
	VZ_BOARD_VERSION_RX_A = 0x3008,
	VZ_BOARD_VERSION_RX_B = 0x3048,
	VZ_BOARD_VERSION_S1L  = 0x3088,
}VZ_BOARD_VERSION;

//global init
int VzDeviceSDK_Init(int SDK_Version);
int VzDeviceSDK_Release(void);


/*@func_name:VZ_DeviceSDK_Sys_Reboot
  *@func_desp:device restart
  *@para:
  *	
  *@return:int
  */

#if defined (__OS_LITEOS) || defined (__OS_LINUX)
	int VZ_DeviceSDK_Sys_Reboot(void);
#endif
/*
获取硬件信息接口
*/
#define HW_INFO_WRITTED 0x27052011
#define OEM_INFO_SIZE 7
#define HW_VERSION_SIZE 4
#define BATCH_NUM_LEN 9
	
#define PLATE_IDENTIFY_VER 11

typedef struct fs_info_ex
{
	char reserved;
	unsigned char oem_info[OEM_INFO_SIZE];		///< �?位厂商的编号，后4位厂商简�?
	unsigned char hw_version[HW_VERSION_SIZE];	
	unsigned int hw_flag;						///< 硬件标识,新版改为保存HwType
    unsigned exdata_size;                       //0xaa res exdata_size 0x55
    unsigned board_version;
}fs_info_ex_t;

typedef struct HWInfo
{
	unsigned int uIndex;        //标签编号：在数据库中递增
	unsigned short usTypeCode;	//智能类型
	unsigned short usModelCode;	//设备型号
	unsigned char ucMAC[6];	    //MAC地址
	unsigned char ucSerial[8];	//序列�?
	unsigned char ucBatchNum[BATCH_NUM_LEN];	//YYMMDDIIX	//年月日智能序号次�?
	unsigned char ucCompanyCode;				//公司代码
	fs_info_ex_t tFSInfoEx;
	unsigned char unone[72];  /* 保留数据 满足128字节 */
}VZCG_HWInfo;

#define ENCRYPY
typedef struct HwStatus
{
	VZCG_HWInfo hwInfo;     //硬件信息    
	unsigned int uHcrc;		//头校验码
	unsigned int uState;    //当前烧写状�?
	unsigned int uVersion;  //uboot版本�?---->存放uboot镜像和硬件加密信�?
	unsigned int kVersion;  //not used 	C系列上没有kernel
	unsigned int rVersion;  //配置文件版本�?  <---->    	Config_file
	unsigned int appVersion; //app镜像	<---->App：分为liteos,app,alg一起的分区
	unsigned int algVersion; //not used
	unsigned int iHwMajorVer;    //硬件版本�?	
	unsigned int iSoftVerMajor;  //软件主版本号
	unsigned int iSoftVerMinor; //软件次版本号
	unsigned int iSoftPatchVer;  //系统版本�?
#ifdef ENCRYPY
	unsigned char  encryptdata[16]; 	//存放秘钥
	unsigned char  uNone[68];  //预留,凑足总包�?56Byte
#else
	unsigned char uNone[84];  //预留,凑足总包�?56Byte
#endif
}HwStatus;

HwStatus * Vz_DeviceSDK_GetSoftWareVer(void);

#if 1
/*
	RTC
*/

typedef enum
{
	UTC = 0,
	CST,
}VZ_TIME_ZONE;

typedef struct VzRtcTime
{
	VZ_TIME_ZONE zone;
	unsigned int year;
	unsigned int month;
	unsigned int day;
	unsigned int hour;
	unsigned int min;
	unsigned int sec;
} VzRtcTime;


int VZ_DeviceSDK_Set_TimeZone(int TimeMin);
int VZ_DeviceSDK_Get_TimeZone();

int VZ_DeviceSDK_RTC_GetTime(unsigned int BoardVersion , VzRtcTime *GetTime);
int VZ_DeviceSDK_RTC_SetTime(unsigned int BoardVersion , VzRtcTime *SetTime);

#if defined (__OS_LITEOS) || defined (__OS_LINUX)
int VZ_DeviceSDK_Set_Sys_Time(VzRtcTime time);
#endif

#endif

/*
	TTY 
*/

//tty max num
#define VZ_DEVICE_TTY_MAX 2

typedef enum
{
	VZ_RATE_300    = 0x01,
	VZ_RATE_600    = 0x02,
	VZ_RATE_1200   = 0x04,
	VZ_RATE_2400   = 0x08,
	VZ_RATE_4800   = 0x10,
	VZ_RATE_9600   = 0x20,
	VZ_RATE_19200  = 0x40,
	VZ_RATE_38400  = 0x80,
	VZ_RATE_57600  = 0x100,
	VZ_RATE_115200 = 0x200,	
}VZ_BAUD_RATE;

typedef enum
{
	VZ_TTY_RS232 = 1,
	VZ_TTY_RS485,
}VZ_TTY_TYPE;

typedef enum
{
#ifdef  __OS_LITEOS
	VZ_DATA_BIT_5 = 0x1,
#endif
	VZ_DATA_BIT_6 = 0x2,
	VZ_DATA_BIT_7 = 0x4,
	VZ_DATA_BIT_8 = 0x8,
}VZ_DATA_BITS;

typedef enum
{
	VZ_STOP_BIT_1 = 0x1,
#ifdef  __OS_LITEOS
	VZ_STOP_BIT_1P5 = 0x2,
#endif
	VZ_STOP_BIT_2 = 0x4
}VZ_STOP_BITS;

typedef enum
{
	VZ_PARITY_NONE = 0x1,
	VZ_PARITY_ODD  = 0x2,
	VZ_PARITY_EVEN = 0x4,
#ifdef  __OS_LITEOS
	VZ_PARITY_MARK = 0x8,
	VZ_PARITY_SPACE = 0x10
#endif
}VZ_PARITY;

typedef struct VZ_TTY_INFO
{
	unsigned int VzTTY_NUM;
	VZ_TTY_TYPE  TtyType[VZ_DEVICE_TTY_MAX];
	unsigned int TtyBaudRateSup[VZ_DEVICE_TTY_MAX];
	unsigned int TtyDataBitsSup[VZ_DEVICE_TTY_MAX];
	unsigned int TtyParitySup[VZ_DEVICE_TTY_MAX];
	unsigned int TtyStopBitSup[VZ_DEVICE_TTY_MAX];;
}VZ_TTY_INFO;

VZ_TTY_INFO * VZ_DeviceSDK_TTY_Init(unsigned int BoardVersion);
int VZ_DeviceSDK_TTY_GetFd(unsigned int TTY_Serial);
int VZ_Device_SDK_TTY_SetAttrSetAttr(unsigned int TTY_Serial, VZ_DATA_BITS databits, VZ_PARITY parity,
												   VZ_STOP_BITS stopbits, VZ_BAUD_RATE baudrate);

#if defined (__OS_LITEOS) || defined (__OS_LINUX)
int VZ_DeviceSDK_TTY_get_byte_to_read(unsigned int TTY_Serial,\
	unsigned int * byte_to_read);
#endif

int VZ_DeviceSDK_TTY_Release(void);

/*
	watchdog
*/

int VZ_DeviceSDK_Watchdog_init(unsigned int BoardVersion);
int VZ_DeviceSDK_Watchdog_SetTimeOut(unsigned int TimeOut);
int VZ_DeviceSDK_Watchdog_Feed(void);
int VZ_DeviceSDK_Watchdog_Release(void);

/*
	LED
*/

typedef enum
{
	VZ_RED      = 0x01,
	VZ_GREEN    = 0x02,
	VZ_BLUE     = 0x04,
	VZ_YELLOW   = 0x08,
	VZ_WHITE    = 0x10,
	VZ_PURPLE   = 0x20,
} LED_COLOR;

typedef enum
{
	LED_MODE_LEVEL=0x00,
	LED_MODE_PWM=0x01,
}LED_MODE;

#define VZ_DEVICE_LED_MAX 1

typedef struct VzDeviceLED
{
	unsigned int LedSupportNum;
	unsigned int LedMode;
	unsigned int LedLevelMax[VZ_DEVICE_LED_MAX];
	unsigned int LedColorSupp[VZ_DEVICE_LED_MAX];
}VzDeviceLED;


/*
LED
*/
VzDeviceLED *VZ_DeviceSDK_LED_Init(unsigned int BoardVersion);
int VZ_DeviceSDK_LED_GetLedStatus(unsigned int LedSerial, unsigned int *Level, unsigned int *Color);
int VZ_DeviceSDK_LED_SetLedLevel(unsigned int LedSerial,unsigned int Level);
int VZ_DeviceSDK_LED_SetLedColor(unsigned int LedSerial,unsigned int Color);
void VZ_DeviceSDK_LED_Release(void);

/*
Motor
*/
/*
focus and zoom
*/
typedef enum
{
	VZ_MOTOR_STOP = 0,
	VZ_MOTOR_FOCUS_A,
	VZ_MOTOR_FOCUS_B,
	VZ_MOTOR_ZOOM_A,
	VZ_MOTOR_ZOOM_B
}VZ_Motor_Type;


int VZ_DeviceSDK_Motor_init(unsigned int BoardVersion);
int VZ_DeviceSDK_Motor_Set(VZ_Motor_Type type,int Step);
int VZ_DeviceSDK_Motor_GetCurStep(int * focusStep,int *zoomStep);
int VZ_DeviceSDK_Motor_Release(void);



#if defined (__OS_LINUX) || defined (__OS_LITEOS)
/*
Flash
*/

typedef struct _FlashOption
{
	unsigned int mtd_num;
	loff_t		 offset;
	size_t		 size;
	void		 *buf; 
}FlashOption;


int VZ_DeviceSDK_Nand_Read(FlashOption *pOpt);
int VZ_DeviceSDK_Nand_Write(FlashOption *pOpt, unsigned int *NeedOffsetNextWrite);


/*@func_name:VZ_DeviceSDK_Nand_Format_Media
  *@func_desp:format media partition
  *@para: 			
  *@return:0: ok -1:error
  */
//int VZ_DeviceSDK_Nand_Format_Media(void);
int VZ_DeviceSDK_Nand_Format_Media(int);

int VZ_DeviceSDK_Nor_Read(FlashOption *pOpt);
int VZ_DeviceSDK_Nor_Write(FlashOption *pOpt);


/*@func_name:VZ_DeviceSDk_NandFlash_BadBlock_Record
  *@func_desp:get nand flash badblock 
  *@para: nand_badblock_info			
  * |		4 bytes			|		4byte		 |      4byte      |     ...		
  *	|total bad block count	|one bad block addr | 		........	|    ...
  *@return:void
  */
int VZ_DeviceSDk_NandFlash_BadBlock_Record(unsigned char *nand_badblock_info);

/*
eth
*/
/*int VZ_DeviceSDK_Eth_Init();
int VZ_DeviceSDK_Eth_Setip(unsigned char ip1, unsigned char ip2, unsigned char ip3, unsigned char ip4);
int VZ_DeviceSDK_Eth_SetNetMask(unsigned char nm1, unsigned char nm2, unsigned char nm3, unsigned char nm4);
int VZ_DeviceSDK_Eth_SetMAC(unsigned char *pMac);
int VZ_DeviceSDK_Eth_SetDNS(unsigned char dns1, unsigned char dns2, unsigned char dns3, unsigned char dns4);*/
#endif

/*********************************NET API START*****************************************/
#define VZ_ETH_DNS_MAX_COUNT	2

/*@func_name:VZ_DeviceSDK_Eth_Init
  *@func_desp:netinit
  *@para:
  *
  *@return:
  *	error:-1
  *	success:0
  */
int VZ_DeviceSDK_Eth_Init(void);

/*@func_name:VZ_DeviceSDK_Eth_SetIpconfig
  *@func_desp:set eth0 ip,netmask,gateway
  *@para:
  *	ipaddr:
  *	netmask:
  *	gateway:
  *@return:
  *	error:-1
  *	success:0
  */
int VZ_DeviceSDK_Eth_SetIpconfig(const char *ipaddr,const char *netmask,const char *gateway);


/*@func_name:VZ_DeviceSDK_Eth_GetIpconfig
  *@func_desp:get eth0 ip,netmask,gateway
  *@para:
  *	ipaddr:
  *	netmask:
  *	gateway:
  *@return:void
  */
void VZ_DeviceSDK_Eth_GetIpconfig(char *ipaddr,char *netmask,char *gateway);

/*@func_name:VZ_DeviceSDK_Eth_SetDNS
  *@func_desp:set dns
  *@para:
  *	serial_dns:0 or 1 
  *	dnsserver:
  *@return: 
  *	error:-1
  *	success:0
  */
int VZ_DeviceSDK_Eth_SetDNS(unsigned int serial_dns, const char *dnsserver);


/*@func_name:VZ_DeviceSDK_Eth_GetDNS
  *@func_desp:get dns
  *@para:
  *	serial_dns:0 or 1 
  *	dnsserver:
  *@return:void  
  */
void VZ_DeviceSDK_Eth_GetDNS(unsigned int serial_dns, char *dnsserver);


/*@func_name:VZ_DeviceSDK_Eth_SetHwaddr
  *@func_desp:set hard mac addr
  *@para:
  *	HwAddr:		for example:	f6:28:8a:77:78:6f
  *@return:
  *	error:-1
  *	success:0
  */
int VZ_DeviceSDK_Eth_SetHwaddr(const char *hwaddr);

/*@func_name:VZ_DeviceSDK_Eth_GetHwaddr
  *@func_desp:get hard mac addr
  *@para:
  *	HwAddr:		for example:	f6:28:8a:77:78:6f
  *@return:void
  */
void VZ_DeviceSDK_Eth_GetHwaddr(char *HwAddr,int *HwAddr_len);


/*@func_name:VZ_DeviceSDK_Eth_DhcpC_Start
  *@func_desp:start dhcp client service
  *@para:
  *	void
  *@return:int 0:ok
  				-1:fail
  */
int VZ_DeviceSDK_Eth_DhcpC_Start(void);



/*@func_name:VZ_DeviceSDK_Eth_DhcpC_is_getIp
  *@func_desp: dhcp client service get ip from other server
  *@para:
  *	void
  *@return:void
  */
int VZ_DeviceSDK_Eth_DhcpC_is_getIp(\
		unsigned char * getIp,\
		unsigned char * getNetmask,\
		unsigned char * getGateway);

/*@func_name:VZ_DeviceSDK_Eth_DhcpC_Start_stop
  *@func_desp:stop dhcp client service
  *@para:
  *	void
  *@return:void
  */
void VZ_DeviceSDK_Eth_DhcpC_stop(void);


/*@func_name:VZ_DeviceSDK_Eth_SNTP_Revice_Time
  *@func_desp:revice time from sntp
  *@para:
  *	void
  *@return:void
  */

#if 0
int VZ_DeviceSDK_Eth_SNTP_Revice_Time(struct timeval * );
#else
int VZ_DeviceSDK_Eth_SNTP_Revice_Time(VzRtcTime * Sntp_get_time, const char *Sntp_Server_ip);
#endif

/*@func_name:VZ_DeviceSDK_Eth_Link_Statu
  *@func_desp:check netif is link uo/down
  *@para:
  *	void
  *@return:1: link up
  *			0:link down
  */

typedef enum __Vz_Netif_Link_Status
{
	Vz_Netif_Link_Down = 0,
	Vz_Netif_Link_on	
}Vz_Netif_Link_Status;
Vz_Netif_Link_Status VZ_DeviceSDK_Eth_Link_Statu(void);
/*********************************NET API END*****************************************/


/*********************************GPIO API START*****************************************/
typedef struct VzDeviceGpio
{
	unsigned int GpioInNum;
	unsigned int GpioOutNum;
	unsigned int GpioTTLNum;
	unsigned int GpioFanNum;
	unsigned int GpioHardWareResetNum;
	unsigned int GpioSensorResetNum;
}VzDeviceGpio,*VzDeviceGpio_p;

typedef enum{
	VZ_GPIO_ERR = -1,
	VZ_GPIO_OFF,
	VZ_GPIO_ON,
}VzDeviceGpioStatus;

VzDeviceGpio* VZ_DeviceSDK_GPIO_Init(unsigned board_ver);

VzDeviceGpioStatus VZ_DeviceSDK_GPIO_GetIn(unsigned int );

int VZ_DeviceSDK_GPIO_SetOut(unsigned int,VzDeviceGpioStatus);

VzDeviceGpioStatus VZ_DeviceSDK_GPIO_GetHWReset(void);

int VZ_DeviceSDK_GPIO_SensorReset(VzDeviceGpioStatus);

void VZ_DeviceSDK_GPIO_DeInit(void);


/******gpio interrupt******/

//int  VZ_DeviceSDK_GPIO_Alarm_in_Irq_Register(void (*)(unsigned int Gpio_In_Serial_num,VzDeviceGpioStatus Gpio_status));

/*********************************GPIO API END*****************************************/

/*@func_name:VZ_DeviceSDK_LED_Status_LED
  *@func_desp:status led control
  *@para:status:1: light on
  *			false:0 off
  *
  *@return:
  *	error:-1
  *	success:0
  */
int VZ_DeviceSDK_LED_Status_LED(int status);


/*@func_name:VZ_DeviceSDK_Ac_Out_Enable
  *@func_desp: control ac enable
  *@para:status:	1:on 
  *					0:off
  *
  *@return:
  *	error:-1
  *	success:0
  */
int VZ_DeviceSDK_Ac_Out_Enable(int status);


#if 1
/*@func_name:VZ_DeviceSDK_Usb_Power_Enable
  *@func_desp: control usb enable
  *@para:status:	1:on 
  *					0:off
  *
  *@return:
  *	error:-1
  *	success:0
  */
int VZ_DeviceSDK_Usb_Power_Enable(int status);
#endif

/********************SD*******************/

/*
	SD  USB SCSI
*/

#define VZ_BLOCK_DEVICE_MAX				4
#define VZ_BLOCK_DEVICE_PATH_SIZE		24
#define VZ_BLOCK_DEVICE_PART_MAX		4

//Dev flags
#define BLOCK_DEVICE_SD             0x0
#define BLOCK_DEVICE_USB            0x1
#define BLOCK_DEVICE_ERROR          0x2
#define BLOCK_DEVICE_HARDDISK       0x3

#define PART_UNMOUNT                0x2
#define PART_MOUNTED                0x5
#define PART_FORMATING              0x3
#define PART_ERROR                  0x0

//part type
typedef enum
{
	FAT32 = 1,
	EXT2,
	EXT3,
	EXT4,
	NTFS,

	UNKNOW = 0xf,
}PART_FILESYS_TYPE;

typedef struct 
{
	unsigned long total;
	unsigned long used;
	unsigned long free;
}PartitionInfo;

typedef struct VzBlockDevPartInfo
{
	unsigned int PartTotleSize;		//MB
	unsigned int PartUsedSize;		//MB
	unsigned int PartFreeSize;		//MB
	PART_FILESYS_TYPE PartType;
	unsigned int PartMounted;
	unsigned int PartFlag;
	char PartDevPath[VZ_BLOCK_DEVICE_PATH_SIZE];
	char PartMountPath[VZ_BLOCK_DEVICE_PATH_SIZE];
} VzBlockDevPartInfo;

typedef struct VzBlockDevice
{
	unsigned int 			BlockDevFlag;
	unsigned int 			BlockDevTotleSize;		//MB
	char         			BlockDevPath[VZ_BLOCK_DEVICE_PATH_SIZE];
	unsigned int 			PartNum;
	VzBlockDevPartInfo		PartInfo[VZ_BLOCK_DEVICE_PART_MAX];
}VzBlockDevice;

//sd card is 0,usb mass is 1
typedef struct VzBlockDeviceInfo
{
	unsigned int BlockDevNum;
	VzBlockDevice	BlockDev[VZ_BLOCK_DEVICE_MAX];
} VzBlockDeviceInfo;

VzBlockDeviceInfo	*VZ_DeviceSDK_BlockDevice_GetInfo();
int VZ_DeviceSDK_BlockDevice_Format(char *PartDevPath);
int VZ_DeviceSDK_BlockDevice_UnMount(char *PartDevPath);
int VZ_DeviceSDK_BlockDevice_Mount(char *PartDevPath, char *pMountPath);
int VZ_DeviceSDK_BlockDevice_Partition(char *BlockDevPath, unsigned int PartNum);


/***********************设备加密**************************/
//设备efuse 解密接口
#define KEY_LEN 16 

typedef enum
{
	opt_zone0 = 0x1,
	opt_zone1,
	opt_zone2,
	opt_zone3
}Vz_opt_zone_num;

typedef struct encrypt_data
{
	char data[KEY_LEN];
}EncryptData,*EncryptData_p;


int VZ_DeviceSDK_Opt_Efuse_Decrypt(EncryptData * pDataSrc,EncryptData * pDataDst,Vz_opt_zone_num opt_num);


typedef enum _Vz_Partition_Status
{
	PARTITION_NOT_COMPLETE = 0x0,
	PARTITION_COMPLETE
}Vz_Partition_Status;

typedef struct _Vz_Partition_Complete_Flag
{
	Vz_Partition_Status Partition_Usr_Complete;				//usr partition complete flag
	Vz_Partition_Status Partition_UsrB_Complete;			//usr_backup partition complete flag
	Vz_Partition_Status Partition_ConfigF_Complete;			//Config_file partition complete flag
	Vz_Partition_Status Partition_Log_Complete;				//Log partition complete flag
	Vz_Partition_Status Partition_media_Complete;			//media partition complete flag
}Vz_Partition_Complete_Flag,*Vz_Partition_Complete_Flag_p;


Vz_Partition_Complete_Flag * VZ_DeviceSDK_Partition_Mount_Status_Get(void);

//IR-CUT
int VzDeviceSDK_Ircut_SetDay(unsigned int BoardVersion);
int VzDeviceSDK_Ircut_SetNight(unsigned int BoardVersion);



#if defined (__cplusplus)
}
#endif

#endif
