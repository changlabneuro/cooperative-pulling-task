#pragma once

#include "serial_lever.hpp"
#include "lever_system.hpp"
#include <array>

namespace om {

struct App {
  virtual ~App() = default;
  virtual void gui_update() {}
  virtual void update() {}
  virtual void setup() {}
  int run();

  std::vector<om::PortDescriptor> ports;
  std::array<om::lever::SerialLeverHandle, 2> levers{};
};

}