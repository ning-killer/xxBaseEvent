/*
* vzes
* Copyright 2013 - 2018, Vzenith Inc.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*  1. Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*  2. Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
*  3. The name of the author may not be used to endorse or promote products
*     derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
* EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <stdio.h>
#include <stdlib.h>
#include "VzDeviceSDK.h"

#ifndef WIN32
int VZ_DeviceSDK_BlockDevice_Partition(char *BlockDevPath,
                                       unsigned int PartNum) {
  return 0;
}

VzBlockDeviceInfo* VZ_DeviceSDK_BlockDevice_GetInfo() {
  return NULL;
}

int VZ_DeviceSDK_BlockDevice_Format(char *PartDevPath) {
  return 0;
}

int VZ_DeviceSDK_BlockDevice_UnMount(char *PartDevPath) {
  return 0;
}

int VZ_DeviceSDK_BlockDevice_Mount(char *pDevPath,
                                   char *pMountPath) {
  return 0;
}

int VZ_DeviceSDK_BlockDevice_GetPartitionSize(
  char *partition, PartitionInfo *info) {
  return 0;
}

int VZ_DeviceSDK_BlockDevice_Partition_By_Handle(
  int PartNum) {
  return 0;
}

int VZ_DeviceSDK_Nand_Format_Media(int partIndex) {
  return 0;
}

int VZ_DeviceSDK_BlockDevice_SetAbnormityStatus(
  unsigned int BlockDevSerial,int err_flag) {
  return 0;
}

int VZ_DeviceSDK_Watchdog_init(unsigned int BoardVersion) {
  return 0;
}

int VZ_DeviceSDK_Watchdog_SetTimeOut(unsigned int TimeOut) {
  return 0;
}

int VZ_DeviceSDK_Watchdog_Feed(void) {
  return 0;
}

int VZ_DeviceSDK_Watchdog_Release(void) {
  return 0;
}

int VzDeviceSDK_Init(int SDK_Version) {
  return 0;
}

int VzDeviceSDK_Release(void) {
  return 0;
}


#endif
