#pragma once
#include <string>
#include <Windows.h>
#include <cmath>
#include "parameters.h"
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
  int32_t x = 0;
  int32_t y = 0;
  uint32_t click = 0;
  bool use_color = false;
  Color color;
  bool relative = false;
  MouseAction(int32_t x, int32_t y, unsigned int click,
    bool relative = false)
  {
    this->x = x + Parameters::origin_x;
    this->y = y + Parameters::origin_y;
    this->click = click;
    this->serial_type = SerialAction::SERIAL_TYPE::MOUSE;
    this->type = IOAction::TYPE::SERIAL;
    this->relative = relative;
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
#ifdef _WIN32
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
#endif
    {
      dx = relative ? x : x - lpPoint.x;
      dy = relative ? y : y - lpPoint.y;
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
  for (int x = Parameters::origin_x; x < Parameters::origin_x + (int)Parameters::screen_width; x++)
  {
    for (int y = Parameters::origin_y; y < Parameters::origin_y + (int)Parameters::screen_height; y++)
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