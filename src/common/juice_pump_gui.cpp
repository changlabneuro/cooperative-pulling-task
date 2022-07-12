#include "juice_pump_gui.hpp"
#include "juice_pump.hpp"
#include "serial.hpp"
#include <imgui/imgui.h>

namespace om {

void gui::render_juice_pump_gui(const JuicePumpGUIParams& params) {
  for (int i = 0; i < params.num_ports; i++) {
    if (ImGui::Button(params.serial_ports[i].port.c_str())) {
      om::pump::initialize_pump_system(params.serial_ports[i].port, params.num_pumps);
    }
  }

  if (ImGui::Button("TerminateSystem")) {
    om::pump::terminate_pump_system();
  }

  for (int i = 0; i < om::pump::num_initialized_pumps(); i++) {
    auto pump_handle = om::pump::ith_pump(i);

    std::string handle_label{"Pump"};
    handle_label += std::to_string(pump_handle.index);

    if (ImGui::TreeNode(handle_label.c_str())) {
      auto desired_pump_state = om::pump::read_desired_pump_state(pump_handle);
      if (ImGui::InputInt("Address", &desired_pump_state.address)) {
        if (desired_pump_state.address >= 0) {
          om::pump::set_address(pump_handle, desired_pump_state.address);
        }
      }
      if (ImGui::InputInt("Rate", &desired_pump_state.rate)) {
        if (desired_pump_state.rate >= 0) {
          om::pump::set_pump_rate(
            pump_handle, desired_pump_state.rate, desired_pump_state.rate_units);
        }
      }

      auto canonical_pump_state = om::pump::read_canonical_pump_state(pump_handle);
      if (canonical_pump_state.connection_open) {
        if (ImGui::Button("Run")) {
          om::pump::run_dispense_program(pump_handle);
        }
        if (ImGui::Button("Stop")) {
          om::pump::stop_dispense_program(pump_handle);
        }
      } else {
        ImGui::Text("Connection is closed.");
      }

      ImGui::TreePop();
    }
  }
}

}
