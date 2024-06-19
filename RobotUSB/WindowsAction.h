#pragma once
#ifdef _WIN32
#include <Windows.h>
#include "Action.h"

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

struct MouseColorAction : public MouseAction
{
    Color color;
    MouseColorAction(Color color, unsigned int click)
    {
        this->color = color;
        this->click = click;
        this->serial_type = SerialAction::SERIAL_TYPE::MOUSE;
        this->type = IOAction::TYPE::SERIAL;
    }

    virtual std::string GetCOMString() override
    {
        std::string serial_write = "0,";
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
        serial_write += std::to_string(dx) + ",";
        serial_write += std::to_string(dy) + ",";
        serial_write += std::to_string(dclick);
        return serial_write;
    }
}
struct MouseAbsoluteAction : public MouseAction
{
    MouseAbsoluteAction(int32_t x, int32_t y, unsigned int click)
    {
        this->x = x + Parameters::origin_x;
        this->y = y + Parameters::origin_y;
        this->click = click;
        this->serial_type = SerialAction::SERIAL_TYPE::MOUSE;
        this->type = IOAction::TYPE::SERIAL;
    }
    
    virtual std::string GetCOMString() override
    {
        POINT lpPoint;
        GetCursorPos(&lpPoint);
        std::string serial_write = "0,";
        int dx, dy;
        int dclick = click;
        dx = x - lpPoint.x;
        dy = y - lpPoint.y;
        serial_write += std::to_string(dx) + ",";
        serial_write += std::to_string(dy) + ",";
        serial_write += std::to_string(dclick);
        return serial_write;

}
#endif