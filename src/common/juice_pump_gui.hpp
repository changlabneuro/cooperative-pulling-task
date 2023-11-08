#pragma once

#include <optional>

namespace om {
struct PortDescriptor;
}

namespace om::gui {

struct JuicePumpGUIParams {
  const om::PortDescriptor* serial_ports;
  int num_ports;
  int num_pumps;
  bool allow_automated_run;
};

struct JuicePumpGUIResult {
  std::optional<bool> allow_automated_run;
  bool reward_triggered;
};

JuicePumpGUIResult render_juice_pump_gui(const JuicePumpGUIParams& params);

}