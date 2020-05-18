#include <iostream>
#include <conio.h>
#include <Windows.h>
#include <thread>
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#define SERIAL_BUFFER_SIZE 16
#define MOUSE_SPEED 2
unsigned int GetMouseClickAction(const std::string&);

namespace
{
  volatile bool running = true;
  int origin_x = 0;
  int origin_y = 0;
  unsigned int screen_width = 0;
  unsigned int screen_height = 0;
  unsigned int loop_count = 100;
}

struct Color
{
  unsigned char r, g, b;
  Color()
  {
    Color(0, 0, 0);
  }
  Color(COLORREF color)
  {
    r = GetRValue(color);
    g = GetGValue(color);
    b = GetBValue(color);
  }
  Color(unsigned char r, unsigned char g, unsigned char b)
  {
    this->r = r;
    this->g = g;
    this->b = b;
  }
  bool operator==(Color const& c)
  {
    return c.r == r && c.g == g && c.b == b;
  }
};

BOOL GetColorLocation(Color, LPPOINT);

struct IOAction
{
  enum class TYPE { SERIAL, DELAY };
  TYPE type;
};

struct SerialAction : IOAction
{
  enum class SERIAL_TYPE { MOUSE, KEYBOARD };
  SERIAL_TYPE serial_type;
  virtual std::string GetCOMString() = 0;
};

struct MouseAction : public SerialAction
{
  unsigned int x = 0;
  unsigned int y = 0;
  unsigned int click = 0;
  bool use_color = false;
  Color color;
  MouseAction(unsigned int x, unsigned int y, unsigned int click)
  {
    this->x = x + origin_x;
    this->y = y + origin_y;
    this->click = click;
    this->serial_type = SerialAction::SERIAL_TYPE::MOUSE;
    this->type = IOAction::TYPE::SERIAL;
  }

  MouseAction(Color color, unsigned int click)
  {
    this->color = color;
    this->click = click;
    use_color = true;
    this->serial_type = SerialAction::SERIAL_TYPE::MOUSE;
    this->type = IOAction::TYPE::SERIAL;
  }

  std::string GetCOMString()
  {
    POINT lpPoint;
    GetCursorPos(&lpPoint);
    std::string serial_write = "0,";
    int dx, dy;
    int dclick = click;
    if (use_color)
    {
      POINT point;
      if (!GetColorLocation(color, &point))
      {
        printf("Failed to find color value, doing nothing\n");
        dx = 0;
        dy = 0;
        dclick = 0;
      }
      else
      {
        printf("Found color value at: %d, %d\n", point.x, point.y);
        dx = point.x - lpPoint.x;
        dy = point.y - lpPoint.y;
      }
    }
    else
    {
      dx = x - lpPoint.x; //accept absolute position then 
      dy = y - lpPoint.y; //translate to relative
    }
    serial_write += std::to_string(dx) + ",";
    serial_write += std::to_string(dy) + ",";
    serial_write += std::to_string(dclick);
    return serial_write;
  }

  unsigned int GetDestinationDistance()
  {
    POINT lpPoint;
    GetCursorPos(&lpPoint);
    int dx = x - lpPoint.x;
    int dy = y - lpPoint.y;
    double dist = std::sqrt(std::pow(dx, 2) + std::pow(dy, 2));
    return static_cast<unsigned int>(std::abs(dist));
  }
};

struct DelayAction : public IOAction
{
  unsigned int time_ms;
  DelayAction(unsigned int time_ms)
  {
    this->time_ms = time_ms;
    this->type = IOAction::TYPE::DELAY;
  }
};


void ThreadCOMReader(const HANDLE&);
void ThreadCOMWriter(const HANDLE&);
void InterruptThread();
bool ProcessScriptFile(const std::string&, std::vector<IOAction*>&);
std::vector<std::string> split_string(char, std::string);


int main()
{
  printf("RobotUSB by Aleksander Krimsky\n");
  printf("Version 2 - www.krimsky.net\n\n");
  printf("Please enter your RobotUSB script: ");
  std::string filename;
  std::cin >> filename;
  std::vector<IOAction*> action_vector;
  if (!ProcessScriptFile(filename, action_vector))
  {
    printf("Press any key to quit.\n");
    _getch();
    return 1;
  }
  printf("Please enter the COM port: ");
  int port;
  std::cin >> port;
  printf("\n");
  if (std::cin.fail() || port < 0 || port > 256)
  {
    printf("Invalid COM port!\n");
    return 1;
  }
  std::wstring pcCommPort = L"\\\\.\\COM";
  pcCommPort += std::to_wstring(port);
  HANDLE hCom = INVALID_HANDLE_VALUE;
  printf("Waiting for port to open on COM%d...\n", port);
  while (hCom == INVALID_HANDLE_VALUE)
  {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    hCom = CreateFileW(pcCommPort.c_str(),
      GENERIC_READ | GENERIC_WRITE,
      0, NULL, OPEN_EXISTING, 0, NULL);
    if (hCom == INVALID_HANDLE_VALUE)
    {
      printf("Code: %d\n", GetLastError());
    }
  }
  printf("Opened port on COM%d. Setting CommState...\n", port);

  DCB dcb;
  SecureZeroMemory(&dcb, sizeof(DCB));
  dcb.DCBlength = sizeof(DCB);
  if (!GetCommState(hCom, &dcb))
  {
    printf("GetCommState failed with error %d.\n", GetLastError());
    return 2;
  }

  dcb.BaudRate = CBR_9600;
  dcb.ByteSize = 8;
  dcb.Parity = NOPARITY;
  dcb.StopBits = ONESTOPBIT;

  if (!SetCommState(hCom, &dcb))
  {
    printf("SetCommState failed with error %d.\n", GetLastError());
    return 3;
  }

  printf("Communication has been established. Press any key to start.\n");
  _getch();

  std::thread interrupt_thread(InterruptThread);
  interrupt_thread.detach();

  for (unsigned int i = 0; i < loop_count; i++)
  {
    printf("\nIterations remaining: %d\n", loop_count - i);
    for (IOAction* action : action_vector)
    {
      if (!running)
      {
        break;
      }
      if (action->type == IOAction::TYPE::SERIAL)
      {
        SerialAction* serial_action = static_cast<SerialAction*>(action);
        LPDWORD bytes_written = 0;
        //arduino loop is at 500ms, account for serial/process time
        unsigned int sleep_time = 1;
        if (serial_action->serial_type == SerialAction::SERIAL_TYPE::MOUSE)
        {
          MouseAction* mouse_action = static_cast<MouseAction*>(serial_action);
          sleep_time += mouse_action->GetDestinationDistance() * MOUSE_SPEED;
        }
        std::string com_string = serial_action->GetCOMString();
        WriteFile(hCom, com_string.c_str(), SERIAL_BUFFER_SIZE, bytes_written, NULL);
        printf("Action: %s, Sleep: %d\n", com_string.c_str(), sleep_time);
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

  CloseHandle(hCom);
  //for (IOAction* action : action_vector)
  //{
  //    delete action;
  //}
  printf("Program has ended!\n");
}

void InterruptThread()
{
  while (running)
  {
    if (GetAsyncKeyState(VK_F4))
    {
      running = false;
      printf("Interrupting execution...\n");
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
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

bool ProcessScriptFile(const std::string& filename, std::vector<IOAction*>& action_vector)
{
  RECT screen;
  GetWindowRect(GetDesktopWindow(), &screen);
  screen_width = screen.right;
  screen_height = screen.bottom;
  std::ifstream file_stream(filename);
  if (!file_stream.good())
  {
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
    if (key.compare("WINDOW_ORIGIN") == 0)
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
        origin_x = lpRect.left;
        origin_y = lpRect.top;
        printf("Found Window: %d, %d\n", origin_x, origin_y);
      }
      else
      {
        printf("Failed to find window with name: %s\n", tokens.at(1).c_str());
        return false;
      }
    }
    else if (key.compare("OFFSET_ORIGIN") == 0)
    {
      if (tokens.size() != 3)
      {
        printf("Expected format: \"OFFSET_ORIGIN,x,y\"\n");
        return false;
      }
      origin_x += atoi(tokens.at(1).c_str());
      origin_y += atoi(tokens.at(2).c_str());
    }
    else if (key.compare("MOUSE") == 0)
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
        printf("Mouse x/y values cannot be negative.\n");
        return false;
      }
      unsigned int ux = static_cast<unsigned int>(x);
      unsigned int uy = static_cast<unsigned int>(y);
      action_vector.push_back(new MouseAction(ux, uy, click));
    }
    else if (key.compare("DELAY") == 0)
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
      action_vector.push_back(new DelayAction(utime_ms));
    }
    else if (key.compare("LOOP_COUNT") == 0)
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
      loop_count = static_cast<unsigned int>(count);
    }
    else if (key.compare("SCREEN_SIZE") == 0)
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
      screen_width = static_cast<unsigned int>(width);
      screen_height = static_cast<unsigned int>(height);
    }
    else if (key.compare("MOUSE_COLOR") == 0)
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
      action_vector.push_back(new MouseAction(Color(r, g, b), click));
    }
    else
    {
      printf("Unrecognized command: %s\n", key.c_str());
      return false;
    }

  }
  printf("Added %d actions\n", action_vector.size());
  printf("Calculated origin at: %d, %d\n", origin_x, origin_y);
  printf("Looping %d times\n", loop_count);
  printf("Screen Width: %d, Screen Height: %d\n", screen_width, screen_height);
  return true;
}

BOOL GetColorLocation(Color color, LPPOINT point)
{
  POINT a;
  POINT b;
  a.x = 0;
  a.y = 0;
  RECT screenRect;
  GetClientRect(GetDesktopWindow(), &screenRect);
  b.x = screenRect.right;
  b.y = screenRect.bottom;
  HDC     hScreen = GetDC(NULL);
  HDC     hDC = CreateCompatibleDC(hScreen);
  HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, abs(b.x - a.x), abs(b.y - a.y));
  HGDIOBJ old_obj = SelectObject(hDC, hBitmap);
  BOOL    bRet = BitBlt(hDC, 0, 0, abs(b.x - a.x), abs(b.y - a.y), hScreen, a.x, a.y, SRCCOPY);
  for (int x = origin_x; x < origin_x + (int)screen_width; x++)
  {
    for (int y = origin_y; y < origin_y + (int)screen_height; y++)
    {
      if (Color(GetPixel(hDC, x, y)) == color)
      {
        point->x = x;
        point->y = y;
        return TRUE;
      }
    }
  }
  return FALSE;
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