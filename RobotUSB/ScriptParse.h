#pragma once
#include <vector>
#include <string>
#include <map>
#include <fstream>
#include "Action.h"
#include "parameters.h"

#ifdef _WIN32
#include <Windows.h>
#include "WindowsAction.h"
#endif


namespace
{
  std::vector<IOAction*> action_vector;
}

std::vector<std::string> split_string(char delim, std::string input_string)
{
  std::vector<std::string> vector;
  std::string splitter(input_string);
  while (splitter.find(delim) != std::string::npos)
  {
    auto sfind = splitter.find(delim);
    vector.push_back(splitter.substr(0, sfind));
    splitter.erase(0, sfind + 1);
  }
  vector.push_back(splitter);
  return vector;
}

unsigned int GetMouseClickAction(const std::string& value)
{
  if (value.compare("NO_CLICK") == 0)
  {
    return 0;
  }
  if ((value).compare("LEFT_CLICK") == 0)
  {
    return 1;
  }
  if (value.compare("RIGHT_CLICK") == 0)
  {
    return 2;
  }
  printf("Invalid Mouse Action: %s\n", (value).c_str());
  printf("Valid Mouse Actions: <LEFT_CLICK|RIGHT_CLICK|NO_CLICK>\n");
  return 3;
}

typedef bool(*ArgParseFunc)(const std::vector<std::string>&);

#ifdef _WIN32
bool ParseWindowOrigin(const std::vector<std::string>& tokens)
{
  if (tokens.size() != 2)
  {
    printf("Expected format: \"WINDOW_ORIGIN,Window Title Name\"\n");
    return false;
  }
  std::wstring window_title = std::wstring(tokens.at(1).begin(), tokens.at(1).end());
  HWND window = FindWindowW(NULL, window_title.c_str());
  if (window != NULL)
  {
    RECT lpRect;
    GetWindowRect(window, &lpRect);
    Parameters::origin_x = lpRect.left;
    Parameters::origin_y = lpRect.top;
    printf("Found Window: %d, %d\n", Parameters::origin_x, Parameters::origin_y);
    return true;
  }
  else
  {
    printf("Failed to find window with name: %s\n", tokens.at(1).c_str());
    return false;
  }
}
#endif

bool ParseOffsetOrigin(const std::vector<std::string>& tokens)
{
  if (tokens.size() != 3)
  {
    printf("Expected format: \"OFFSET_ORIGIN,x,y\"\n");
    return false;
  }
  Parameters::origin_x += atoi(tokens.at(1).c_str());
  Parameters::origin_y += atoi(tokens.at(2).c_str());
  return true;
}
#ifdef _WIN32
bool ParseMouse(const std::vector<std::string>& tokens)
{
  if (tokens.size() != 4)
  {
    printf("Expected format: \"MOUSE,x,y,<LEFT_CLICK|RIGHT_CLICK|NO_CLICK>\"\n");
    return false;
  }

  unsigned int click = GetMouseClickAction(tokens.at(3));
  if (click == 3)
  {
    return false;
  }

  int x = atoi(tokens.at(1).c_str());
  int y = atoi(tokens.at(2).c_str());
  if (x < 0 || y < 0)
  {
    printf("Mouse x/y values cannot be negative, use MOUSE_RELATIVE\n");
    return false;
  }
  unsigned int ux = static_cast<unsigned int>(x);
  unsigned int uy = static_cast<unsigned int>(y);
  action_vector.push_back(new MouseAbsoluteAction(ux, uy, click));
  return true;
}
#endif

bool ParseDelay(const std::vector<std::string>& tokens)
{
  if (tokens.size() != 2)
  {
    printf("Expected format: \"DELAY,time_ms\"\n");
    return false;
  }
  int time_ms = atoi(tokens.at(1).c_str());
  if (time_ms < 0)
  {
    printf("Delay time cannot be negative.\n");
    return false;
  }
  unsigned int utime_ms = static_cast<unsigned int>(time_ms);
  if (tokens.at(0).compare("DELAY") == 0)
    action_vector.push_back(new DelayAction(utime_ms));
  else //DELAY_EACH_ACTION
    Parameters::delay_each_action = utime_ms;
  return true;
}

bool ParseLoopCount(const std::vector<std::string>& tokens)
{
  if (tokens.size() != 2)
  {
    printf("Expected format: \"LOOP_COUNT,amount\"\n");
    return false;
  }
  int count = atoi(tokens.at(1).c_str());
  if (count < 0)
  {
    printf("Loop count cannot be negative.\n");
    return false;
  }
  Parameters::loop_count = static_cast<unsigned int>(count);
  return true;
}

#ifdef _WIN32
bool ParseScreenSize(const std::vector<std::string>& tokens)
{
  if (tokens.size() != 3)
  {
    printf("Expected format: \"SCREEN_SIZE,width,height\"\n");
    return false;
  }
  int width = atoi(tokens.at(1).c_str());
  int height = atoi(tokens.at(2).c_str());
  if (width < 0 || height < 0)
  {
    printf("Screen width/height cannot be negative!.\n");
    return false;
  }
  Parameters::screen_width = static_cast<unsigned int>(width);
  Parameters::screen_height = static_cast<unsigned int>(height);
  return true;
}
#endif

#ifdef _WIN32
bool ParseMouseColor(const std::vector<std::string>& tokens)
{
  if (tokens.size() != 5)
  {
    printf("Expected format: \"MOUSE_COLOR,R,G,B,<LEFT_CLICK|RIGHT_CLICK|NO_CLICK>\"\n");
    return false;
  }

  int r = atoi(tokens.at(1).c_str());
  int g = atoi(tokens.at(2).c_str());
  int b = atoi(tokens.at(3).c_str());
  if (r < 0 || g < 0 || b < 0 || r > 255 || g > 255 || b > 255)
  {
    printf("Valid RGB values are from 0 to 255\n");
    return false;
  }
  unsigned int click = GetMouseClickAction(tokens.at(4));
  if (click == 3)
  {
    return false;
  }
  action_vector.push_back(new MouseColorAction(Color(r, g, b), click));
  return true;
}
#endif

bool ParseMouseRelative(const std::vector<std::string>& tokens)
{
  if (tokens.size() != 4)
  {
    printf("Expected format: \"MOUSE_RELATIVE,x,y,<LEFT_CLICK|RIGHT_CLICK|NO_CLICK>\"\n");
    return false;
  }

  unsigned int click = GetMouseClickAction(tokens.at(3));
  if (click == 3)
  {
    return false;
  }

  int x = atoi(tokens.at(1).c_str());
  int y = atoi(tokens.at(2).c_str());
  unsigned int ux = static_cast<unsigned int>(x);
  unsigned int uy = static_cast<unsigned int>(y);
  action_vector.push_back(new MouseAction(ux, uy, click));
  return true;
}

#ifdef _WIN32
bool ParseLoopBind(const std::vector<std::string>& tokens)
{
  if (tokens.size() != 2)
  {
    printf("Expected format: \"LOOP_BIND,<Virtual Key Code>\"\n");
    return false;
  }
  Parameters::loop_bind = std::stoi(tokens.at(1), 0, 16);
  return true;
}
#endif

#ifdef _WIN32
bool ParseLoopBindToggle(const std::vector<std::string>& tokens)
{
  if (tokens.size() != 2)
  {
    printf("Expected format: \"LOOP_BIND_TOGGLE,<Virtual Key Code>\"\n");
    return false;
  }
  Parameters::loop_bind_toggle = std::stoi(tokens.at(1), 0, 16);
  return true;
}
#endif 

#ifdef _WIN32
bool ParseEndScript(const std::vector<std::string>& tokens)
{
  if (tokens.size() != 2)
  {
    printf("Expected format: \"END_SCRIPT,<Virtual Key Code>\"\n");
    return false;
  }
  Parameters::end_script = std::stoi(tokens.at(1), 0, 16);
  return true;
}
#endif

std::map<std::string, ArgParseFunc> script_args =
{
#ifdef _WIN32
  {"WINDOW_ORIGIN", ParseWindowOrigin}, //WIN32
  {"SCREEN_SIZE", ParseScreenSize}, //WIN32
  {"END_SCRIPT", ParseEndScript}, //WIN32
  {"LOOP_BIND", ParseLoopBind}, //WIN32
  {"LOOP_COUNT", ParseLoopCount}, //WIN32
  {"MOUSE_COLOR", ParseMouseColor}, //WIN32
  {"LOOP_BIND_TOGGLE", ParseLoopBindToggle}, //WIN32
  {"MOUSE", ParseMouse},
#endif
  {"OFFSET_ORIGIN", ParseOffsetOrigin},
  {"MOUSE_RELATIVE", ParseMouseRelative},
  {"DELAY", ParseDelay},
  {"DELAY_EACH_ACTION", ParseDelay},
};

bool ProcessScriptFile(const std::string& filename)
{

  std::ifstream file_stream(filename);
  if (!file_stream.good())
  {
    perror("test1");
    printf("Could not open: %s\n", filename.c_str());
    return false;
  }
  std::string line;
  while (std::getline(file_stream, line))
  {
    std::vector<std::string> tokens = split_string(',', line);
    if (tokens.empty())
    {
      continue;
    }
    std::string key = tokens.at(0);
    std::map<std::string, ArgParseFunc>::iterator it = script_args.find(key);
    if (it != script_args.end())
    {
      it->second(tokens);
    }
    else
    {
      printf("Skipping token \"%s\"\n", key.c_str());
    }
  }
  printf("Added %lu actions\n", action_vector.size());
  /*
  printf("Calculated origin at: %d, %d\n", Parameters::origin_x, Parameters::origin_y);
  printf("Looping %d times\n", Parameters::loop_count);
  printf("Screen Width: %d, Screen Height: %d\n", Parameters::screen_width, Parameters::screen_height);
  printf("Script is running, end using Virtual Key 0x%x\n", Parameters::end_script);
  if (Parameters::loop_bind)
  {
    printf("Script loop is binded to Virtual Key: 0x%x\n", Parameters::loop_bind);
    if (Parameters::loop_bind_toggle)
    {
      printf("Script loop bind can be toggled with Virtual Key: 0x%x\n", Parameters::loop_bind_toggle);
    }
  }
  */
  return !action_vector.empty();
}