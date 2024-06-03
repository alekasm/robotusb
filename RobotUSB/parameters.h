#pragma once
#include <Windows.h>
namespace Parameters
{
  int origin_x = 0;
  int origin_y = 0;
  unsigned int screen_width = 0;
  unsigned int screen_height = 0;
  unsigned int loop_count = 1;
  unsigned int delay_each_action = 0;
  DWORD loop_bind = NULL;
  DWORD loop_bind_toggle = NULL;
  DWORD end_script = VK_F5;
}

void ResetParameters()
{
  Parameters::origin_x = 0;
  Parameters::origin_y = 0;
  Parameters::screen_width = 0;
  Parameters::screen_height = 0;
  Parameters::loop_count = 1;
  Parameters::delay_each_action = 0;
  Parameters::loop_bind = NULL;
  Parameters::loop_bind_toggle = NULL;
  Parameters::end_script = VK_F5;
}
