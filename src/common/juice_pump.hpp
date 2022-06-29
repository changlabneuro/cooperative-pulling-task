#pragma once

#include "identifier.hpp"
#include <string>

namespace om::pump {

struct PumpHandle {
  OM_INTEGER_IDENTIFIER_EQUALITY(PumpHandle, index)
  uint32_t index;
};

void initialize_pump_system(int num_pumps);
void terminate_pump_system();

bool open_connection(const std::string& port);
bool is_connection_open();
bool close_connection();

}