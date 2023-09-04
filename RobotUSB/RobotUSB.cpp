#include <iostream>
#include <conio.h>
#include <Windows.h>
#include "ScriptParse.h"
#include "Action.h"
#include <thread>
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <algorithm>
#include <map>
#include <setupapi.h>
#include <devguid.h>
#define SERIAL_BUFFER_SIZE 16
#define MOUSE_SPEED 2
#pragma comment (lib, "SetupAPI")

typedef bool(*ArgParseFunc)(const std::vector<std::string>&);
bool ParseWindowOrigin(const std::vector<std::string>&);

namespace
{
  volatile bool running = true;
  volatile bool interrupt = false;
  volatile bool loop_bind_toggled = true;
}

void ResetProgram()
{  
  running = true;
  interrupt = false;
  loop_bind_toggled = true;
  action_vector.clear();
  ResetParameters();
}

void ThreadCOMReader(const HANDLE&);
void ThreadCOMWriter(const HANDLE&);
void InterruptThread();
bool ProcessScriptFile(const std::string&, std::vector<IOAction*>&);
std::vector<std::string> split_string(char, std::string);

int main()
{
  printf("RobotUSB Version 4 | Aleksander Krimsky - www.krimsky.net\n");

  //Adapted from Jeffreys on StackOverflow
  //https://stackoverflow.com/questions/58030553/

  HDEVINFO hDevInfo;
  SP_DEVINFO_DATA DeviceInfoData = { sizeof(DeviceInfoData) };
  // get device class information handle
  hDevInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_PORTS, 0, 0, DIGCF_PRESENT);
  if (hDevInfo == INVALID_HANDLE_VALUE)
  {
    printf("Failed to find COM ports.\n");
    return 0;
  }

  for (int i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData); i++)
  {
    DWORD DataT;
    char friendly_name[64] = { 0 };

    if (!SetupDiGetDeviceRegistryPropertyA(hDevInfo, &DeviceInfoData,
      SPDRP_FRIENDLYNAME, &DataT, (PBYTE)friendly_name, sizeof(friendly_name), NULL))
    {
      continue;
    }

    friendly_name[63] = 0;
    printf("%s\n", friendly_name);
  }

  enter_port:
  printf("\nPlease enter the COM port: ");
  int port;
  std::cin >> port;
  printf("\n");
  if (std::cin.fail() || port < 0 || port > 256)
  {
    printf("Invalid COM port: %d\n", port);
    goto enter_port;
  }
  std::wstring pcCommPort = L"\\\\.\\COM";
  pcCommPort += std::to_wstring(port);
  HANDLE hCom = INVALID_HANDLE_VALUE;
  printf("Waiting for port to open on COM%d...\n", port);
  for(int i = 0; i < 6; ++i)
  {
    Sleep(500);
    hCom = CreateFileW(pcCommPort.c_str(),
      GENERIC_READ | GENERIC_WRITE,
      0, NULL, OPEN_EXISTING, 0, NULL);
    if (hCom != INVALID_HANDLE_VALUE)
    {
      break;
    }
  }

  if (hCom == INVALID_HANDLE_VALUE)
  {
    printf("Failed to open port on COM%d\n", port);
    goto enter_port;
  }

  printf("Opened port on COM%d, setting CommState.\n", port);

  DCB dcb;
  SecureZeroMemory(&dcb, sizeof(DCB));
  dcb.DCBlength = sizeof(DCB);
  if (!GetCommState(hCom, &dcb))
  {
    printf("GetCommState failed with error %d.\n", GetLastError());
    return 2;
  }

  dcb.BaudRate = CBR_115200;
  dcb.ByteSize = 8;
  dcb.Parity = NOPARITY;
  dcb.StopBits = ONESTOPBIT;

  if (!SetCommState(hCom, &dcb))
  {
    printf("SetCommState failed with error %d.\n", GetLastError());
    return 3;
  }

  printf("Communication has been established!\n");

program_main:
  ResetProgram();
  std::string filename;
  
  do
  {
    printf("\nPlease enter your RobotUSB script: ");
    filename.clear();
    std::cin >> filename;
  } while (!ProcessScriptFile(filename));


  std::thread interrupt_thread(InterruptThread);
  interrupt_thread.detach();

  while (!interrupt)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (!running)
    {
      continue;
    }
    for (unsigned int i = 0; i < Parameters::loop_count; i++)
    {
      if (Parameters::loop_count > 1)
      {
        if (!running || interrupt)
        {
          printf("\n");
          break;
        }
        printf("\rIterations remaining: %d", Parameters::loop_count - i);
      }
      for (IOAction* action : action_vector)
      {
        if (!running || interrupt)
          break;

        if (action->type == IOAction::TYPE::SERIAL)
        {
          SerialAction* serial_action = static_cast<SerialAction*>(action);
          LPDWORD bytes_written = 0;
          unsigned int sleep_time = 1 + Parameters::delay_each_action;
          if (serial_action->serial_type == SerialAction::SERIAL_TYPE::MOUSE)
          {
            MouseAction* mouse_action = static_cast<MouseAction*>(serial_action);
            if (!mouse_action->relative)
            {
              sleep_time += mouse_action->GetDestinationDistance() * MOUSE_SPEED;
            }
          }
          std::string com_string = serial_action->GetCOMString();
          WriteFile(hCom, com_string.c_str(), SERIAL_BUFFER_SIZE, bytes_written, NULL);
          std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
        }
        else if (action->type == IOAction::TYPE::DELAY)
        {
          DelayAction* delay_action = static_cast<DelayAction*>(action);
          printf("sleep %d\n", delay_action->time_ms);
          std::this_thread::sleep_for(std::chrono::milliseconds(delay_action->time_ms));
        }
      }
    }
  }

  printf("(R)un new script or (E)xit program?: ");
  std::string input_command;
  std::cin >> input_command;
  if (!std::cin.fail() && input_command.size())
  {
    char input = input_command.at(0);
    if (input == 0x52 || input == 0x72)
    {
      goto program_main;
    }
  }

  CloseHandle(hCom);
  printf("Program has ended!\n");
}

void InterruptThread()
{
  while (!interrupt)
  {
    if (Parameters::loop_bind)
    {
      running = loop_bind_toggled && GetAsyncKeyState(Parameters::loop_bind);
      if (!running && GetAsyncKeyState(Parameters::loop_bind_toggle))
      {
        loop_bind_toggled ^= true;
        printf("Script is toggled: %d\n", loop_bind_toggled);
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
    }
    if (GetAsyncKeyState(Parameters::end_script))
    {
      interrupt = true;
      printf("Interrupting execution...\n");
    }
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
}