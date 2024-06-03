#include "usb.h"
#include <Windows.h>
#include <stdio.h>
#include <setupapi.h>
#include <devguid.h>
#pragma comment (lib, "SetupAPI")

size_t get_devices(struct SerialById* ids)
{

  unsigned int device_count = 0;
  HDEVINFO hDevInfo;
  SP_DEVINFO_DATA DeviceInfoData = { sizeof(DeviceInfoData) };

  hDevInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_PORTS, 0, 0, DIGCF_PRESENT);
  if (hDevInfo == INVALID_HANDLE_VALUE)
  {
    printf("Failed to find COM ports.\n");
    return 0;
  }

  //SPDRP_DEVICEDESC = name
  //SPDRP_MFG = manufacturer
  //SPDRP_ENUMERATOR_NAME = type
  for (int i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData); i++)
  {

    HKEY hKey = SetupDiOpenDevRegKey(hDevInfo, &DeviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
    if (hKey == INVALID_HANDLE_VALUE)
      continue;

    DWORD query_type;
    CHAR query_buf[256] = { 0 };
    DWORD cbData = sizeof(query_buf) - 1;
    LSTATUS status = RegQueryValueExA(hKey, "PortName", NULL, &query_type,
      (LPBYTE)query_buf, &cbData);

    if (status != ERROR_SUCCESS)
      continue;

    RegCloseKey(hKey);

    if (query_type != REG_SZ)
      continue; //expected REG_SZ for PortName

    DWORD DataT;
    char buffer[64] = { 0 };
    if (!SetupDiGetDeviceRegistryPropertyA(hDevInfo, &DeviceInfoData,
      SPDRP_DEVICEDESC, &DataT, (PBYTE)buffer, sizeof(buffer), NULL))
    {
      continue;
    }
    buffer[63] = 0;

    size_t path_len = cbData + 1;
    size_t id_len = strlen(buffer) + 1;
    const char* path = (const char*)malloc(path_len);
    const char* id = (const char*)malloc(id_len);
    if (path == NULL || id == NULL)
      continue;
    memset(path, 0, path_len);
    memset(id, 0, id_len);
    memcpy(path, query_buf, path_len);
    memcpy(id, buffer, id_len);
    ids[device_count].path = path;
    ids[device_count].id = id;
    ++device_count;
    //printf("%s:%s\n", buffer, query_buf);
  }
  return device_count;
}

void* sd_open(const char* device)
{
  HANDLE hCom = INVALID_HANDLE_VALUE;
  hCom = CreateFileA(device,
    GENERIC_READ | GENERIC_WRITE,
    0, NULL, OPEN_EXISTING, 0, NULL);
  if (hCom == INVALID_HANDLE_VALUE)
    return (void*) -1;
  return hCom;
}

int sd_config(void* device_id, struct SerialSettings settings)
{
  HANDLE hCom = (HANDLE)device_id;
  DCB dcb;
  SecureZeroMemory(&dcb, sizeof(DCB));
  dcb.DCBlength = sizeof(DCB);
  if (!GetCommState(hCom, &dcb))
  {
    printf("GetCommState failed with error %d.\n", GetLastError());
    return 1;
  }

  dcb.BaudRate = CBR_115200;
  dcb.ByteSize = 8;
  dcb.Parity = NOPARITY;
  dcb.StopBits = ONESTOPBIT;

  if (!SetCommState(hCom, &dcb))
  {
    printf("SetCommState failed with error %d.\n", GetLastError());
    return 1;
  }

  return 0;
}

int sd_write(void* device, char* buf, size_t size)
{
  DWORD written;
  WriteFile(device, buf, size, &written, NULL);
  return written;
}

void sd_close(void* device)
{
  if ((int)device > -1)
  {
    CloseHandle((HANDLE)device);
  }
}