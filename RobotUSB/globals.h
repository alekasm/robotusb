#pragma once
#include <vector>
struct IOAction;
extern std::vector<IOAction*> action_vector;
volatile extern bool running;
volatile extern bool interrupt;
volatile extern bool loop_bind_toggled;