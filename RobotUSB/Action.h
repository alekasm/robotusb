#pragma once
#include <stdint.h>
#include <string.h>
#include "parameters.h"

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
  MouseAction(int32_t x, int32_t y, unsigned int click)
  {
    this->x = x + Parameters::origin_x;
    this->y = y + Parameters::origin_y;
    this->click = click;
    this->serial_type = SerialAction::SERIAL_TYPE::MOUSE;
    this->type = IOAction::TYPE::SERIAL;
  }

  virtual std::string GetCOMString() override
  {
    std::string serial_write = "0,";
    serial_write += std::to_string(x) + ",";
    serial_write += std::to_string(y) + ",";
    serial_write += std::to_string(click);
    return serial_write;
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