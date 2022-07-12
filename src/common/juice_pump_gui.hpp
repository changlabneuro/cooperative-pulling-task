#pragma once

namespace om {
struct PortDescriptor;
}

namespace om::gui {

struct JuicePumpGUIParams {
  const om::PortDescriptor* serial_ports;
  int num_ports;
  int num_pumps;
};

void render_juice_pump_gui(const JuicePumpGUIParams& params);

}