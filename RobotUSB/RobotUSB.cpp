#include <iostream>
#include "Action.h"
#include <thread>
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <algorithm>
#include <map>
#include <Windows.h>
#include "ScriptParse.h"
extern "C"
{
#include "usb.h"
}

#define SERIAL_BUFFER_SIZE 16
#define MOUSE_SPEED 2

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

void InterruptThread();
bool ProcessScriptFile(const std::string&, std::vector<IOAction*>&);
std::vector<std::string> split_string(char, std::string);

int main()
{
  printf("RobotUSB Version 5 | Aleksander Krimsky - www.krimsky.net\n");

  struct SerialById* ids = new struct SerialById[MAX_SERIAL_DEVICES];
  size_t count = get_devices(ids);
  for (size_t i = 0; i < count; ++i)
  {
    printf("[%02d] %32s = %s\n", i, ids[i].id, ids[i].path);
  }


  enter_port:
  printf("\nPlease enter your port selection: ");
  int port;
  std::cin >> port;
  printf("\n");
  if (std::cin.fail() || port < 0 || port > count)
  {
    printf("Invalid selection: %d\n", port);
    goto enter_port;
  }
  std::string cCommPort = "\\\\.\\";
  cCommPort.append(std::string(ids[port].path));

  //std::wstring pcCommPort = L"\\\\.\\COM";
 // pcCommPort += std::to_wstring(ids[i]);

  int device_id = -1;
  printf("Waiting for port to open on %s...\n", cCommPort.c_str());
  for (int i = 0; i < 6; ++i)
  {
    Sleep(500);
    device_id = sd_open(cCommPort.c_str());
    if ((int)device_id > -1)
      break;
  }

  if ((int)device_id == -1)
  {
    printf("Failed to open port %s\n", ids[port].path);
    goto enter_port;
  }

  printf("Opened port %s, configuring...\n", ids[port].path);
  struct SerialSettings settings = { 0 };
  if (sd_config(device_id, settings) != 0)
    return 1;

  printf("Port has been configured\n");

program_main:
  ResetProgram();
  std::string filename;
  
  do
  {
    printf("\nPlease enter your RobotUSB script: ");
    filename.clear();
    std::cin >> filename;
  } while (!ProcessScriptFile(filename));


#ifdef _WIN32
  std::thread interrupt_thread(InterruptThread);
  interrupt_thread.detach();
#endif

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
          sd_write(device_id, com_string.c_str(), SERIAL_BUFFER_SIZE);
          //WriteFile(hCom, com_string.c_str(), SERIAL_BUFFER_SIZE, bytes_written, NULL);
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

  sd_close(device_id);
  printf("Program has ended!\n");
}

#ifdef _WIN32
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
#endif